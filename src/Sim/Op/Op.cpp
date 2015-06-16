// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Ryota Shioya.
// Copyright (c) 2005-2015 Masahiro Goshima.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
// 
// 


#include <pch.h>


#include "Sim/Op/Op.h"
#include "Sim/Dumper/Dumper.h"

#include "Sim/Foundation/SimPC.h"
#include "Sim/Dependency/MemDependency/MemDependency.h"
#include "Sim/Register/RegisterFile.h"
#include "Sim/Foundation/TimeWheel/TimeWheelBase.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Memory/MemOrderManager/MemOrderManager.h"
#include "Sim/Foundation/Checkpoint/CheckpointMaster.h"
#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/Op/OpInitArgs.h"
#include "Sim/Thread/Thread.h"
#include "Sim/Core/Core.h"
#include "Sim/Foundation/SimPC.h"
#include "Sim/Dependency/PhyReg/PhyReg.h"
#include "Sim/ExecUnit/ExecUnitIF.h"

using namespace std;
using namespace Onikiri;


Op::Op(OpIterator iterator) :
    m_opInfo     (0),
    m_opClass    (0),
    m_no         (0),
    m_localTID   (TID_INVALID),
    m_serialID   (0),
    m_globalSerialID     (0),
    m_retireID   (0),
    m_thread     (0),
    m_core       (0),
    m_inorderList(0),
    m_beforeCheckpoint (0), 
    m_afterCheckpoint (0), 
    m_taken      (false),
    m_latPredResult(),
    m_issueState (),
    m_scheduler  (0),
    m_regFile    (0),
    m_srcNum     (0),
    m_dstNum     (0),
    m_iterator   (iterator)
{
}

Op::~Op()
{
}

void Op::Initialize(const OpInitArgs& args)
{
    m_pc            = *args.pc;
    m_opInfo        = args.opInfo;
    m_opClass       = &m_opInfo->GetOpClass();
    m_no            = args.no;
    m_localTID      = args.thread->GetLocalThreadID();
    m_serialID      = args.serialID;
    m_retireID      = args.retireID;
    m_globalSerialID = args.globalSerialID;
    m_core          = args.core;
    m_thread        = args.thread;
    m_inorderList   = args.thread->GetInorderList();
    m_status        = OpStatus();
    m_beforeCheckpoint  = 0; 
    m_afterCheckpoint   = 0; 
    m_takenPC       = PC(args.pc->pid, args.pc->tid, args.pc->address+4); 
    m_taken         = false;
    m_latPredResult = LatPredResult();
    m_issueState    = IssueState();
    m_reserveExecUnit = false;
    m_scheduler     = 0;

    m_memAccess.address = Addr();
    m_memAccess.size    = 0;
    m_memAccess.sign    = false;
    m_memAccess.value   = 0;

    m_exception = Exception();
    m_cacheAccessResult = CacheAccessResult();

    m_regFile = m_core->GetRegisterFile();
    m_srcNum = m_opInfo->GetSrcNum();
    m_dstNum = m_opInfo->GetDstNum();

    SetPID( args.pc->pid );
    SetTID( args.pc->tid );

    //m_id            = id;
    // m_event �� retire/flush ���ɏ���������Ă���
    // m_event.clear();
    // m_event.reserve(8);

    for (int i = 0; i < SimISAInfo::MAX_DST_REG_COUNT; i++) {
        m_dstReg[i] = UNSET_REG;
        m_dstPhyReg[i] = 0;
        m_dstResultValue[i] = 0;
    }

    for (int i = 0; i < SimISAInfo::MAX_SRC_REG_COUNT; i++) {
        m_srcReg[i] = UNSET_REG;
        m_srcPhyReg[i] = 0;
    }

    for (int i = 0; i < MAX_SRC_MEM_NUM; i++) {
        m_srcMem[i].reset();
    }

    for (int i = 0; i < MAX_DST_MEM_NUM; i++) {
        m_dstMem[i].reset();
    }

}


PhyReg* Op::GetPhyReg(int phyRegNo)
{
    return m_regFile->GetPhyReg(phyRegNo);
}

const u64 Op::GetSrc(const int index) const
{
    ASSERT(m_srcReg[index] != UNSET_REG,
        "physical register not set at index %d.", index);

    return m_srcPhyReg[index]->GetVal();
}

const u64 Op::GetDst(const int index) const
{
    ASSERT(m_dstReg[index] != UNSET_REG,
        "physical register not set at index %d.", index);

    return m_dstPhyReg[index]->GetVal();
}

void Op::SetDst(const int index, const u64 value)
{
    ASSERT(m_dstReg[index] != UNSET_REG,
        "physical register not set at index %d.", index);

//  m_dstPhyReg[index]->SetVal(value);
    m_dstResultValue[index] = value;
}

void Op::SetTaken(const bool taken)
{
    m_taken = taken;
}

bool Op::GetTaken() const
{
    return m_taken;
}

void Op::SetTakenPC(const PC takenPC)
{
    m_takenPC = takenPC;
}

PC Op::GetTakenPC() const
{
    return m_takenPC;
}

PC Op::GetNextPC()
{
    if( m_taken ) {  
        return m_takenPC;
    }else {
        SimPC nextPC(m_pc);
        nextPC++;
        return nextPC;
    }
}


void Op::Read( MemAccess* access )
{
    // Must set a correct TID because this method is called by
    // the emulator, which does not set a TID.
    access->address.tid = GetTID();
    m_thread->GetMemOrderManager()->Read( m_iterator, access );
}

void Op::Write( MemAccess* access )
{
    // Must set a correct TID because this method is called by
    // the emulator, which does not set a TID.
    access->address.tid = GetTID();
    m_thread->GetMemOrderManager()->Write( m_iterator, access );
}

// Returned whether this op is ready to select in an 'index'-th scheduler.
// 'newDeps' is a set of dependencies that will be satisfied.
// You can test whether an op can be selected if 'newDeps' dependencies are 
// satisfied. 'newDeps' argument can be NULL.
bool Op::IsSrcReady( int index, const DependencySet* newDeps ) const
{
    ASSERT( index == m_scheduler->GetIndex() );

    for( int i = 0; i < m_srcNum; ++i ){
        Dependency* dep = m_srcPhyReg[i];
        if( dep->GetReadiness( index ) == false ){
            if( newDeps ){
                if( !newDeps->IsIncluded( dep ) ){
                    return false;
                }
            }
            else{
                return false;
            }
        }
    }

    for(int i = 0; i < MAX_SRC_MEM_NUM; ++i) {
        Dependency* dep = m_srcMem[i].get();
        if( dep && dep->GetReadiness(index) == false) {
            if( newDeps ){
                if( !newDeps->IsIncluded( dep ) ){
                    return false;
                }
            }
            else{
                return false;
            }
        }
    }

    return true;
}

void Op::AddEvent(
    const EventPtr& evnt, 
    TimeWheelBase* timeWheel, 
    int time, 
    EventMask mask
)
{
    timeWheel->AddEvent( evnt, time );
    if( !evnt->IsUpdated() ){
        m_event.AddEvent( evnt, mask );
    }
}

void Op::CancelEvent( EventMask mask )
{
    m_event.Cancel( mask );
}

void Op::ClearEvent()
{
    m_event.Clear();
}


// dependency �� reset ����
void Op::ResetDependency()
{
    int dstDepNum = GetDstDepNum();
    for( int i = 0; i < dstDepNum; ++i ){
        GetDstDep(i)->Reset();
    }
}

// Op���g���ăX�P�W���[������
void Op::RescheduleSelf( bool clearIssueState )
{
    // �X�P�W���[���ɓ����Ă��Ȃ���΍ăX�P�W���[�����Ȃ�
    if( !IsDispatched() ) {
        return;
    }

    // dependency �̃��Z�b�g
    ResetDependency();
    
    // event �̃L�����Z��
    CancelEvent();

    // Reset an issue state.
    if( clearIssueState ){
        m_issueState = IssueState();
    }
}

// ���ߎ��s
void Op::ExecutionBegin()
{
    m_core->GetEmulator()->Execute(this, m_opInfo);
}

void Op::ExecutionEnd()
{
    ASSERT( GetStatus() == OpStatus::OS_EXECUTING );
    WriteExecutionResults();
    SetStatus( OpStatus::OS_FINISHED );
}

// Write execution results to physical registers.
void Op::WriteExecutionResults()
{
    // write back
    for( int i = 0; i < m_dstNum; ++i ){
        m_dstPhyReg[i]->SetVal( m_dstResultValue[i] );
    }
}

void Op::DissolveSrcReg()
{
    if( GetStatus() == OpStatus::OS_FETCH ){
        return;
    }
    for( int i = 0; i < m_srcNum; ++i ) {
        m_srcPhyReg[i]->RemoveConsumer( m_iterator );
    }
}

void Op::DissolveSrcMem()
{
    if( GetStatus() == OpStatus::OS_FETCH ){
        return;
    }
    for( int i = 0; i < MAX_SRC_MEM_NUM; ++i ) {
        if( m_srcMem[i] != NULL ) {
            m_srcMem[i]->RemoveConsumer( m_iterator );
        }
    }
}

// consumer ���߂̒��ł����Ƃ��ŏ��Ƀt�F�b�`���ꂽ���߂�Ԃ�
// consumer ���߂����Ȃ����0��Ԃ�
OpIterator Op::GetFirstConsumer()
{
    OpIterator result(0);

    // �������W�X�^�̒���������Ƃ���Ƀt�F�b�`���ꂽconsumer��T��
    int dstRegNum = m_dstNum;
    for( int i = 0; i < dstRegNum; ++i ) {
        if(m_dstReg[i] == UNSET_REG) continue;

        PhyReg* phyReg = m_dstPhyReg[i];
        const Dependency::ConsumerListType& consumers = phyReg->GetConsumers();
        
        if( consumers.empty() ) continue;
        
        // �v���O�����E�I�[�_�Ń��l�[�����s��ꃊ�X�g�ɓ�����Ă���̂�
        // ���X�g�̐擪�ɂ��閽�߂���Ԑ�Ƀt�F�b�`���ꂽ
        OpIterator frontOp = consumers.front();
        if( result.IsNull() || result->GetSerialID() > frontOp->GetSerialID() ) {
            result = frontOp;
        }
    }

    // �������W�X�^�̒���������Ƃ���Ƀt�F�b�`���ꂽconsumer��T��
    int dstMemNum = MAX_DST_MEM_NUM;
    for( int i = 0; i < dstMemNum; ++i ) {
        MemDependencyPtr memDependency = m_dstMem[i];
        if( memDependency == 0 ) continue;

        const Dependency::ConsumerListType& consumers = memDependency->GetConsumers();
        if( consumers.empty() ) continue;
        
        // �v���O�����E�I�[�_�Ń��l�[�����s��ꃊ�X�g�ɓ�����Ă���̂�
        // ���X�g�̐擪�ɂ��閽�߂���Ԑ�Ƀt�F�b�`���ꂽ
        OpIterator frontOp = consumers.front();
        if( result.IsNull() || result->GetSerialID() > frontOp->GetSerialID() ) {
            result = frontOp;
        }
    }
    
    if( m_opClass->IsStore() ){
        MemOrderManager* memOrder = m_thread->GetMemOrderManager();
        OpIterator consumer = memOrder->GetConsumerLoad( m_iterator, m_memAccess, 0 );
        if( !consumer.IsNull() && 
            ( result.IsNull() ||
              result->GetSerialID() > consumer->GetSerialID() 
            )
        ){
            result = consumer;
        }
    }
    return result;

}

// �\�[�X���W�X�^���Z�b�g���āA�������W�X�^��consumer�ɒǉ�
void Op::SetSrcReg(int index, int phyRegNo)
{
    m_srcReg[index] = phyRegNo;
    m_srcPhyReg[index] = GetPhyReg(phyRegNo);
    GetPhyReg(phyRegNo)->AddConsumer(m_iterator);
}

// �\�[�X�̃��������Z�b�g���āAconsumer�ɒǉ�
void Op::SetSrcMem(int index, MemDependencyPtr memDep)
{
    m_srcMem[index] = memDep;
    memDep->AddConsumer(m_iterator);
}

// index �Ԗڂ� destination �̕������W�X�^�ԍ����Z�b�g
void Op::SetDstReg(int index, int phyRegNo)
{
    m_dstReg[index] = phyRegNo; 
    PhyReg* phyReg = GetPhyReg(phyRegNo);
    m_dstPhyReg[index] = phyReg;
}

void Op::SetDstMem(int index, MemDependencyPtr dstMem)
{
    m_dstMem[index] = dstMem; 
}

// index �Ԗڂ� destination �̕������W�X�^�ԍ���Ԃ�
int Op::GetDstReg(int index)
{
    return m_dstReg[index];
}

// index �Ԗڂ� source �̕������W�X�^�ԍ���Ԃ�
int Op::GetSrcReg(int index)
{
    return m_srcReg[index];
}

int Op::GetDstDepNum() const
{
    int memDepNum = 0;
    for( int i = 0; i < MAX_DST_MEM_NUM; i++ ){
        if( m_dstMem[i] ){
            memDepNum++;
        }
        else{
            break;
        }
    }
    return m_dstNum + memDepNum;
}

Dependency* Op::GetDstDep( int index ) const
{
    if( index < m_dstNum ){
        return GetDstPhyReg( index );
    }
    else{
        return m_dstMem[ index - m_dstNum ].get();
    }
}

// Returns a execution unit of this op.
ExecUnitIF* Op::GetExecUnit() const
{
    ASSERT( m_scheduler != NULL );
    return m_scheduler->GetExecUnit( GetOpClass().GetCode() ); 
}


std::string Op::ToString(int detail, bool valDetail, const char* delim)
{
    ostringstream oss;

    // PC�܂�
    oss 
        << "GID: " << GetGlobalSerialID() << delim
        << "TID: " << GetTID() << delim
        << "SID: " << GetSerialID() << delim
        << "RID: " << GetRetireID() << delim
        << "PID: " << GetPID() << delim
        << "PC: " << GetPC().pid << "/" 
        << hex << GetPC().address << dec
        << "[" << GetNo() <<"]" << delim;

    if(detail == 0) return oss.str();

    // status �܂�
    oss << GetStatus().ToString() << delim;

    if(detail == 1) return oss.str();

    oss << GetOpInfo()->GetMnemonic() << delim;
    oss << GetOpClass().ToString() << delim;

    // �_�����W�X�^�܂�
    int dstNum = GetOpInfo()->GetDstNum();
    for(int i = 0; i < dstNum; ++i) {
        oss << "d" << i << ": " ;
        oss << GetOpInfo()->GetDstOperand(i) << delim; 
    }
    for(int i = dstNum;
        i < SimISAInfo::MAX_DST_REG_COUNT; ++i)
    {
        oss << "d" << i << ": -1"<< delim; 
    }

    int srcNum = GetOpInfo()->GetSrcNum();
    for(int i = 0; i < srcNum; ++i) {
        oss << "s" << i << ": "
            << GetOpInfo()->GetSrcOperand(i) <<delim; 
    }
    for(int i = GetOpInfo()->GetSrcNum();
        i < SimISAInfo::MAX_SRC_REG_COUNT; ++i)
    {
        oss << "s" << i << ": -1"<< delim; 
    }

    if(detail == 2) return oss.str();

    // next pc �܂�
    oss << "TPC: " << GetTakenPC().pid << "/"
        << hex << GetTakenPC().address << dec
        << ( GetTaken() ? "(t)" : "(n)" ) << delim;

    if(detail == 3) return oss.str();
    // �������W�X�^�̊��蓖��
    for(int i = 0; i < SimISAInfo::MAX_DST_REG_COUNT; ++i) {
        if( m_dstReg[i] != UNSET_REG) {
            oss << "r" << GetOpInfo()->GetDstOperand(i)
                << "= p" << GetDstReg(i); 
        }else {
            oss << "r_ = p_"; 
        }
        oss << delim;
    }

    for(int i = 0; i < SimISAInfo::MAX_SRC_REG_COUNT; ++i) {
        if( m_srcReg[i] != UNSET_REG ) {
            oss << "r" << GetOpInfo()->GetSrcOperand(i)
                << "= p" << GetSrcReg(i); 
        }else {
            oss << "r_ = p_"; 
        }
        oss << delim;
    }

    if(detail == 4) return oss.str();

    // �������W�X�^�̒l�܂�
    for(int i = 0; i < SimISAInfo::MAX_DST_REG_COUNT; ++i) {
        if( m_dstReg[i] != UNSET_REG) {
            oss << "r" << GetOpInfo()->GetDstOperand(i)
                << "= " << GetDst(i); 
            if( valDetail ) {
                oss << "/" << (double)GetDst(i)
                    << "/" << hex << GetDst(i) << dec;
            }
        }else {
            oss << "r_ = 0"; 
        }
        oss << delim;
    }

    for(int i = 0; i < SimISAInfo::MAX_SRC_REG_COUNT; ++i) {
        if( m_srcReg[i] != UNSET_REG ) {
            oss << "r" << GetOpInfo()->GetSrcOperand(i)
                << "= " << GetSrc(i); 
            if( valDetail ) {
                oss << "/" << (double)GetSrc(i)
                    << "/" << hex << GetSrc(i) << dec;
            }
        }else {
            oss << "r_ = 0"; 
        }
        oss << delim;
    }

    if(detail == 5) return oss.str();

    // MemAccess �̒��g�܂�
    oss << "Mem: " << hex << GetMemAccess().address.address << dec << "/"
        << GetMemAccess().size << "/"
        << (GetMemAccess().sign ? "s" : "u") << "/"
        << GetMemAccess().value;

    if( valDetail ) {
        oss << "/" << (double)GetMemAccess().value
            << "/" << hex << GetMemAccess().value << dec;
    }
    oss << delim;
    return oss.str();
}
