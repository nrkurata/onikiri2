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

#include "Sim/Pipeline/Fetcher/Fetcher.h"
#include "Sim/Pipeline/Fetcher/Steerer/FetchThreadSteererIF.h"

#include "Env/Env.h"
#include "Sim/Dumper/Dumper.h"
#include "Utility/RuntimeError.h"

#include "Interface/EmulatorIF.h"

#include "Sim/InorderList/InorderList.h"
#include "Sim/Thread/Thread.h"
#include "Sim/Core/Core.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Predictor/DepPred/RegDepPred/RegDepPredIF.h"
#include "Sim/Predictor/DepPred/MemDepPred/MemDepPredIF.h"
#include "Sim/Memory/Cache/Cache.h"
#include "Sim/Memory/Cache/CacheSystem.h"
#include "Sim/System/ForwardEmulator.h"

#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/System/GlobalClock.h"
#include "Sim/Op/Op.h"
#include "Sim/Op/OpInitArgs.h"
#include "Sim/Predictor/BPred/BPred.h"
#include "Sim/Dependency/PhyReg/PhyReg.h"
#include "Sim/Memory/MemOrderManager/MemOrderManager.h"
#include "Sim/Foundation/SimPC.h"

using namespace std;
using namespace boost;
using namespace Onikiri;


namespace Onikiri
{
    HookPoint<Fetcher, Fetcher::FetchHookParam> Fetcher::s_fetchHook;
    HookPoint<Fetcher, Fetcher::SteeringHookParam> Fetcher::s_fetchSteeringHook;
    HookPoint<Fetcher, Fetcher::FetchDecisionHookParam> Fetcher::s_fetchDecisionHook;
    HookPoint<Fetcher, Fetcher::BranchPredictionHookParam> Fetcher::s_branchPredictionHook;
};  // namespace Onikiri

Fetcher::Fetcher() :
    m_fetchWidth(0),    
    m_fetchLatency(0),
    m_numFetchedOp(0),
    m_numFetchedPC(0),
    m_numFetchGroup(0),
    m_numBranchInFetchGroup(0),
    m_idealMode(false),
    m_checkLatencyMismatch(false),
    m_currentFetchThread(0),
    m_bpred(0),
    m_cacheSystem(0),
    m_emulator(0),
    m_globalClock(0),
    m_forwardEmulator(0),
    m_fetchThreadSteerer(0)
{
}

Fetcher::~Fetcher()
{
}

void Fetcher::Initialize(InitPhase phase)
{
    PipelineNodeBase::Initialize(phase);

    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
    }
    else if(phase == INIT_POST_CONNECTION){

        // Check whether the member variables are set correctly.
        CheckNodeInitialized( "emulator", m_emulator );
        CheckNodeInitialized( "bpred",    m_bpred );
        CheckNodeInitialized( "cacheSystem",   m_cacheSystem );
        CheckNodeInitialized( "globalClock",   m_globalClock );

        int iCacheLatency = m_cacheSystem->GetFirstLevelInsnCache()->GetStaticLatency();
        if( m_checkLatencyMismatch && m_fetchLatency != iCacheLatency ){
            THROW_RUNTIME_ERROR( 
                "'@FetchLatency' and the latency of the L1 I-cache do not match. "
                "If this configuration is intended, disable '@CheckLatencyMismatch'."
            );
        }

        if( m_fetchLatency < iCacheLatency ){
            THROW_RUNTIME_ERROR( 
                "'@FetchLatency' is shorter than the latency of the L1 I-cache." 
            );
        }

        if( GetLowerPipelineNode() == NULL ){
            THROW_RUNTIME_ERROR( "Renamer is not connected." );
        }

        DisableLatch();
    }
}

void Fetcher::Finalize()
{
    m_stallCycles.others = GetStalledCycles();

    m_stallCycles.total = m_stallCycles.others;
    // These stalls are not implemented by using ClockedResource sytem.
    m_stallCycles.total +=
        m_stallCycles.currentSyscall +  
        m_stallCycles.nextSyscall +
        m_stallCycles.checkpoint +
        m_stallCycles.inorderList;

    ReleaseParam();
}

// Returns whether 'info' requires serializing or not.
bool Fetcher::IsSerializingRequired( const OpInfo* const info ) const
{
    return info->GetOpClass().IsSyscall();
}

bool Fetcher::IsSerializingRequired( OpIterator op ) const
{
    return IsSerializingRequired( op->GetOpInfo() );
}

bool Fetcher::IsSerializingRequired( Thread* thread ) const
{
    // ���񉻂��Ȃ��Ƃ����Ȃ����߂��R�A���ɂ�����X�g�[��
    InorderList* inorderList = thread->GetInorderList();
    OpIterator frontCommittedOp = inorderList->GetCommittedFrontOp();
    OpIterator frontOp          = inorderList->GetFrontOp();

    // In serializing state.
    if( !inorderList->IsEmpty() &&
        ( ( !frontOp.IsNull() && IsSerializingRequired( frontOp ) ) ||
          ( !frontCommittedOp.IsNull() && IsSerializingRequired( frontCommittedOp ) ) )
    ){
        return true;
    }
    else{
        return false;
    }
}


// 1���߂��Ƃ�fetch�o���邩�ǂ���������
void Fetcher::CanFetchBody( FetchDecisionHookParam* param )
{
}


bool Fetcher::CanFetch( Thread* thread, PC pc, OpInfo** infoArray, int numOp ) 
{
    FetchDecisionHookParam param = { thread, pc, infoArray, numOp, false };

    HOOK_SECTION_PARAM( s_fetchDecisionHook, param )
    {
        // ���񉻂��Ȃ��Ƃ����Ȃ����߂��R�A���ɂ�����X�g�[��
        // In serializing state.
        if( IsSerializingRequired( thread ) ){
            ++m_stallCycles.currentSyscall;
            param.canFetch = false;
            return param.canFetch;
        }

        // The number of required checkpoints for this fetch group.
        int numCheckpointReq = 0;
    
        // ���񉻂��Ȃ��Ƃ����Ȃ����߂��ǂ���
        bool reqSerializing = false;

        for( int k = 0; k < numOp; ++k ) {
            OpInfo* info = infoArray[k];

            if( IsSerializingRequired(info) ) {
                reqSerializing = true;
            }

            // �t�F�b�`�O���[�v���ɕ����̕�����܂܂Ȃ��悤�ɂ���
            if( !m_idealMode && info->GetOpClass().IsBranch() ) {
                if( m_numBranchInFetchGroup >= m_maxBranchInFetchGroup ){
                    param.canFetch = false;
                    return param.canFetch;
                }
                ++m_numBranchInFetchGroup;
            }

            if( GetCore()->IsRequiredCheckpointBefore(pc, info) ) {
                ++numCheckpointReq;
            }

            if( GetCore()->IsRequiredCheckpointAfter(pc, info) ) {
                ++numCheckpointReq;
            }
        }

        // checkpointMaster �̃`�F�b�N
        if( !thread->GetCheckpointMaster()->CanCreate( numCheckpointReq ) ) {
            ++m_stallCycles.checkpoint;
            param.canFetch = false;
            return param.canFetch;
        }

        if( !thread->GetInorderList()->CanAllocate( numOp ) ){
            ++m_stallCycles.inorderList;
            param.canFetch = false;
            return param.canFetch;
        }

        // ���񉻂��Ȃ��Ƃ����Ȃ�����
        if( reqSerializing ){
            // ���񉻂��Ȃ��Ƃ����Ȃ��Ȃ�A�㗬�����󂩂ǂ����Ԃ�
            InorderList* inorderList = thread->GetInorderList();
            if( m_evaluated.isInorderListEmpty && inorderList->IsEmpty() ){
                param.canFetch = true;
                return param.canFetch;
            }
            else {
                ++m_stallCycles.nextSyscall;
                param.canFetch = false;
                return param.canFetch;
            }
        }

        param.canFetch = true;
    }
    return param.canFetch;

}

// array����op�������Ƃ��āC�����o�֐��|�C���^����Ăяo�����s��
inline void Fetcher::ForEachOp(
    FetchedOpArray& c, 
    int size, 
    void (Fetcher::*func)(OpIterator) )
{
    for( int i = 0; i < size; i++ ){
        (this->*func)(c[i]);
    }
}

template <typename T1>
inline void Fetcher::ForEachOpArg1(
    FetchedOpArray& c, 
    int size, 
    T1 arg1,
    void (Fetcher::*func)(OpIterator, T1) )
{
    for( int i = 0; i < size; i++ ){
        (this->*func)(c[i], arg1);
    }
}

// pc/infoArray/numOp����t�F�b�`����fetchedOp�Ɋi�[����֐�
void Fetcher::Fetch(Thread* thread, FetchedOpArray& fetchedOp, PC pc, OpInfo** infoArray, int numOp )
{
    InorderList* InorderList = thread->GetInorderList();
    Core* core = GetCore();

    u64 baseSerialID = thread->GetOpSerialID();
    u64 baseRetireID = thread->GetOpRetiredID();

    fetchedOp.clear();
    for( int mopIndex = 0; mopIndex < numOp; ++mopIndex ) {

        FetchHookParam param;
        HOOK_SECTION_PARAM( s_fetchHook, param ){
            OpInitArgs args = 
            {
                &pc,                        // PC*      pc;
                infoArray[ mopIndex ],      // OpInfo* opInfo;
                mopIndex,                   // int      no;
                m_globalClock->GetInsnID(), // u64      globalSerialID;
                baseSerialID + mopIndex,    // u64      serialID;
                baseRetireID + mopIndex,    // u64      retireID;
                core,                       // Core*        core;
                thread                      // Thread*  thread;
            };

            OpIterator op = InorderList->ConstructOp( args );
            param.op = op;

            g_dumper.Dump( DS_FETCH, op );

            fetchedOp.push_back( op );
            InorderList->PushBack( op );

            m_globalClock->AddInsnID( 1 );
            m_forwardEmulator->OnFetch( op );
        }   // HOOK_SECTION_PARAM( s_fetchHook, param ){

    }   // for( int mopIndex = 0; mopIndex < numOp; ++mopIndex ) {
}

void Fetcher::CreateCheckpoint( OpIterator op )
{
    const PC& pc = op->GetPC();
    OpInfo* const opInfo = op->GetOpInfo();

    if( GetCore()->IsRequiredCheckpointBefore(pc, opInfo) ){
        CheckpointMaster* master = op->GetThread()->GetCheckpointMaster();
        Checkpoint* newCheckpoint = master->CreateCheckpoint();
        op->SetBeforeCheckpoint( newCheckpoint );
    }

    if( GetCore()->IsRequiredCheckpointAfter(pc, opInfo) ){
        CheckpointMaster* master = op->GetThread()->GetCheckpointMaster();
        Checkpoint* newCheckpoint = master->CreateCheckpoint();
        op->SetAfterCheckpoint( newCheckpoint );
    }
}

void Fetcher::BackupOnCheckpoint( OpIterator op, bool before )
{
    const PC& pc = op->GetPC();
    OpInfo* const opInfo = op->GetOpInfo();

    if( before && GetCore()->IsRequiredCheckpointBefore(pc, opInfo) ) {
        CheckpointMaster* master = op->GetThread()->GetCheckpointMaster();
        master->Backup(op->GetBeforeCheckpoint(), CheckpointMaster::SLOT_FETCH);
    }

    if( !before && GetCore()->IsRequiredCheckpointAfter( pc, opInfo ) ) {
        CheckpointMaster* master = op->GetThread()->GetCheckpointMaster();
        master->Backup(op->GetAfterCheckpoint(), CheckpointMaster::SLOT_FETCH);
    }
}

// ����\�����s���֐��̖{��
void Fetcher::PredictNextPCBody(BranchPredictionHookParam* param)
{
    PC predPC;

    PC fetchGroupPC = param->fetchGroupPC;
    OpIterator op = param->op;
    // �{���̓t�F�b�`�O���[�v�ɑ΂���1�񂾂�BTB������
    // ����Ƀt�F�b�`�O���[�v���̕���ɑ΂��Ă̂�BTB������
    if (op->GetOpClass().IsBranch()) {
        predPC = GetBPred()->Predict( op, fetchGroupPC );

        // BTB/RAS �̖��������G���g�� ���C�ႤPID�̂��̂�Ԃ��Ă���
        predPC.pid = fetchGroupPC.pid;
        predPC.tid = fetchGroupPC.tid;
    }
    else {
        SimPC pc(op->GetPC());
        predPC = pc.Next();
    }

    op->SetPredPC( predPC );
/*
    ASSERT(
        op->GetPC().pid == op->GetPredPC().pid,
        "predicted pid not match."
    );
*/

}

// ����\�����s���֐�
void Fetcher::PredictNextPC(OpIterator op, PC fetchGroupPC)
{
    BranchPredictionHookParam param;
    param.fetchGroupPC = fetchGroupPC;
    param.op = op;

    HookEntry( this, &Fetcher::PredictNextPCBody, &s_branchPredictionHook, &param);
}

// Enter an op to a fetcher pipeline.
// An op is send to a renamer automatically when the op exits a fetch pipeline.
// A renamer is set to a fetcher by the renamer itself in SetUpperPipelineNode
// that calls SetLowerPipelineNode of the fetcher.
void Fetcher::EnterPipeline( OpIterator op, int nextEventCycle )
{
    GetLowerPipeline()->EnterPipeline( 
        op, 
        nextEventCycle, 
        GetLowerPipelineNode() 
    );
    op->SetStatus( OpStatus::OS_FETCH );
}

void Fetcher::Finished(OpIterator op)
{
    GetBPred()->Finished(op);
}

void Fetcher::Commit(OpIterator op)
{
    GetBPred()->Commit(op);
}

void Fetcher::Evaluate()
{
    // Actually fetched ops in this cycle are determined with 'updated'.
    Evaluated updated;
    updated.fetchThread = GetFetchThread( false );
    
    if( updated.fetchThread ){
        updated.fetchPC = updated.fetchThread->GetFetchPC();

        // In serializing state.
        InorderList* inorderList = updated.fetchThread->GetInorderList();
        updated.isInorderListEmpty = inorderList->IsEmpty();
        updated.reqSerializing = 
            ( !updated.isInorderListEmpty && IsSerializingRequired( updated.fetchThread ) );
    }

    m_evaluated = updated;
    BaseType::Evaluate();
}


void Fetcher::Update()
{

    const int cacheOffsetBitSize = 
        m_cacheSystem->GetFirstLevelInsnCache()->GetOffsetBitSize();

    // �t�F�b�`�O���[�v���̕��򐔂������� (CanFetch�Ŏg�p) 
    m_numBranchInFetchGroup = 0;

    Thread* fetchThread = GetFetchThread( true );
    if( !fetchThread ){
        // �R�A���̑S�ẴX���b�h���I�����Ă���
        return;
    }
    if( m_evaluated.reqSerializing ){
        ++m_stallCycles.currentSyscall;
        return;
    }

    ASSERT( m_evaluated.fetchThread == fetchThread );

    PC fetchGroupPC = fetchThread->GetFetchPC();
    if( fetchGroupPC != m_evaluated.fetchPC ){
        // Branch prediction miss recovery is done 
        return;
    }

    int numFetchedPC = 0;

    for(int i = 0; i < GetFetchWidth(); ++i) {
        // emu
        const SimPC pc = fetchThread->GetFetchPC();

        if( m_idealMode )
            fetchGroupPC = pc;

        if( pc.address == 0 )
            break;

        // �L���b�V���̃��C�����E���܂�������t�F�b�`���I��
        if( !m_idealMode &&
            (fetchGroupPC.address >> cacheOffsetBitSize) != 
            (pc.address >> cacheOffsetBitSize)
        ){
            break;
        }

        pair<OpInfo**, int> ops = m_emulator->GetOp(pc);

        OpInfo** opArray = ops.first;
        int numOp = ops.second;

        // emulator �ɓn����PC���疽�߂������Ȃ���ΏI��
        if( opArray == 0 || opArray[0] == 0)
            break;

        // ���̖��߂��t�F�b�`�ł��Ȃ��Ȃ�t�F�b�`�I��
        if( !CanFetch(fetchThread, pc, opArray, numOp) ) {
            break;
        }

        /*
        for( int k = 0; k < numOp; ++k ) {
        // fetch
        // checkpoint at decode
        ...
        // next pc update
        // checkpoint at dispatch
        // dispatch
        }
        �̌`�ɂȂ��ĂȂ����R�̃���

        PC��Op��1��1�ɑΉ����Ă��Ȃ��āAPC�̓r���̖��߂�
        �`�F�b�N�|�C���g�����\��������
        ����ŁA���߂̍ăt�F�b�`��PC�P�ʂōs���̂ŁA
        ��Ԃ̍X�V��PC�P�ʂōs��Ȃ��Ƃ����Ȃ�

        ���̂ق��F
        PC�̓r���̖��߂̎��s���ʂœ�����NextPC���ǂ��Ȃ邩��������Ȃ�
        ����\���~�X�̌��o�̂Ƃ���ōl����K�v�����肻��
        */

        // fetch
        FetchedOpArray fetchedOp;
        Fetch( fetchThread, fetchedOp, pc, opArray, numOp );

        // �K�v�Ȃ�`�F�b�N�|�C���g���쐬
        ForEachOp( fetchedOp, numOp, &Fetcher::CreateCheckpoint );
        ForEachOpArg1( fetchedOp, numOp, true/*before*/, &Fetcher::BackupOnCheckpoint );

        fetchThread->AddOpSerialID( numOp );
        fetchThread->AddOpRetiredID( numOp );

        // Update serialID, retireID and statistics.
        m_numFetchedOp += numOp;
        numFetchedPC++;
        for( s64 i = 0; i < numOp; i++ ){
            m_opClassStat.Increment(fetchedOp[i]);
        }

        // bpred
        ForEachOpArg1( fetchedOp, numOp, fetchGroupPC, &Fetcher::PredictNextPC );

        // next pc update
        fetchThread->SetFetchPC( fetchedOp[numOp - 1]->GetPredPC() );
        
        ForEachOpArg1( fetchedOp, numOp, false/*before*/, &Fetcher::BackupOnCheckpoint );

        // Register the fetched ops to the fetch pipeline.
        // Delay of I-Cache miss is implemented in the following stall process.
        ForEachOpArg1( fetchedOp, numOp, m_fetchLatency - 1, &Fetcher::EnterPipeline );

        // I-Cache Hit/miss decision
        int iCacheReadLatency = GetICacheReadLatency(pc);

        // -1 �́C�����ݏ������̃T�C�N�����������Ă���
        int stallCycles = iCacheReadLatency - m_fetchLatency - 1;   
        if( stallCycles > 0 ) {
            StallNextCycle( stallCycles );
        }

        // <TODO> �t�F�b�`�O���[�v���̕���̈ʒu���w�K�E�\������
        // taken�ȕ���Ńt�F�b�`�O���[�v���I��������
        if( !m_idealMode && 
            fetchedOp[numOp - 1]->GetPredPC() != pc.Next() 
        ){
            break;
        }
    }

    // 1�ȏ�̖��߂��t�F�b�`������t�F�b�`�O���[�v���𑝂₷
    if( numFetchedPC > 0 ){
        m_numFetchGroup++;
    }

    m_numFetchedPC += numFetchedPC;
}


// Decide a thread that fetches ops in this cycle.
Thread* Fetcher::GetFetchThread( bool update )
{
    SteeringHookParam hookParam = { NULL, &m_thread, update };
    
    HOOK_SECTION_PARAM( s_fetchSteeringHook, hookParam ){
        hookParam.targetThread = m_fetchThreadSteerer->SteerThread(update);
    }


    return hookParam.targetThread;
}

const int Fetcher::GetICacheReadLatency(const PC& pc)
{
    // ���߃L���b�V���A�N�Z�X�ł͗L���Ȗ��߂͂��Ȃ�
    // ��̖��߂�p���ăA�N�Z�X����
    CacheAccess access;
    access.address = pc;
    access.type = CacheAccess::OT_READ;
    access.op = OpIterator();   // Todo: add a valid op
    CacheAccessResult result = 
        m_cacheSystem->GetFirstLevelInsnCache()->Read( access, NULL );
    return result.latency;
}

void Fetcher::SetInitialNumFetchedOp(u64 num)
{
    m_numFetchedOp = num;
    // Todo: thread ����retireID �����l�̒����R�[�h������
}
