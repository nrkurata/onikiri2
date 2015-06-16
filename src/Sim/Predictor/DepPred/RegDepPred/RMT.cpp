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

#include "Sim/Predictor/DepPred/RegDepPred/RMT.h"
#include "Sim/Foundation/Checkpoint/CheckpointMaster.h"
#include "Interface/ISAInfo.h"
#include "Sim/Op/Op.h"
#include "Sim/Foundation/Hook/HookUtil.h"



using namespace Onikiri;
using namespace std;
using namespace boost;

namespace Onikiri
{
    HookPoint<RMT,RMT::HookParam> RMT::s_allocateRegHook;
    HookPoint<RMT,RMT::HookParam> RMT::s_releaseRegHook;
    HookPoint<RMT,RMT::HookParam> RMT::s_deallocateRegHook;
};  // namespace Onikiri


// RMT �̏������O�� RegisterFile / CheckpointMaster �̏��������s���Ă���K�v������
RMT::RMT() :
    m_numLogicalReg(0),
    m_numRegSegment(0),
    m_core(0),
    m_checkpointMaster(0),
    m_emulator(0),
    m_registerFile(0),
    m_regFreeList(0),
    m_allocationTable()
{
}

RMT::~RMT()
{
    ReleaseParam();
}

void RMT::Initialize(InitPhase phase)
{
    if(phase == INIT_POST_CONNECTION){
        // �����o�ϐ��̃`�F�b�N
        CheckNodeInitialized( "registerFile",   m_registerFile );
        CheckNodeInitialized( "regFreeList",    m_regFreeList );
        CheckNodeInitialized( "emulator",       m_emulator );
        CheckNodeInitialized( "checkpointMaster", m_checkpointMaster );

        ISAInfoIF* isaInfo = m_emulator->GetISAInfo();

        m_numLogicalReg = isaInfo->GetRegisterCount();
        m_numRegSegment = isaInfo->GetRegisterSegmentCount();

        // checkpointedData �̏�����
        CheckpointMaster* checkpointMaster = m_checkpointMaster;
        m_allocationTable.Initialize(
            checkpointMaster,
            CheckpointMaster::SLOT_RENAME
        );

        // �Z�O�����g�����e�[�u���Ɋi�[
        m_segmentTable.assign(-1);
        for (int i = 0; i < m_numLogicalReg; ++i) {
            int segment = isaInfo->GetRegisterSegmentID(i);
            m_segmentTable[i] = segment;
        }
        vector<int> logicalRegNum( m_numRegSegment , 0 );
        
        // �_�����W�X�^�̐��������W�X�^��������
        for (int i = 0; i < m_numLogicalReg; i++) {
            // freelist ����1���W�X�^�ԍ������炤
            int segment = GetRegisterSegmentID(i);
            int phyRegNo = m_regFreeList->Allocate(segment);
            logicalRegNum[segment]++;

            // �_�����W�X�^���������W�X�^�̊��蓖��
            (*m_allocationTable)[i] = phyRegNo;

            // ���W�X�^�̏�Ԃ̏�����
            // �����ŏ���������Ȃ��������̂�allocate����鎞�ɏ����������
            PhyReg* phyReg = (*m_registerFile)[phyRegNo];
            phyReg->Clear();

            // ��ԏ��߂͑S�����f�B
            phyReg->Set();
            // value �� m_emulator����󂯎��
            phyReg->SetVal(m_emulator->GetInitialRegValue(0, i));
        }

        // ����p�̃e�[�u���̏�����
        // ������Ԃł̓f�X�e�B�l�[�V�������W�X�^�Ƃ��Ċ��蓖�Ă�ꂽ�������W�X�^�͂��Ȃ�
        // 
        m_releaseTable.resize(m_registerFile->GetTotalCapacity(), -1);
    }
}

// �}�b�v�\���Ђ��Ę_�����W�X�^�ԍ��ɑΉ����镨�����W�X�^�̔ԍ���Ԃ�
int RMT::ResolveReg(int lno)
{
    // RMT �̏ꍇ�CResolveReg �ŕ���p�͐����Ȃ����߁C
    // PeekReg �ɈϏ�
    return PeekReg(lno);
}

// ResolveReg �Ɠ��l�ɕ������W�X�^�ԍ���Ԃ��D
// �������C����p���Ȃ����Ƃ��ۏ؂����
int RMT::PeekReg(int lno) const
{
    ASSERT(
        lno >= 0 && lno < m_numLogicalReg,
        "illegal register No.: %d\n", lno
    );

    return ( m_allocationTable.GetCurrent() )[lno];
}


// �t���[���X�g���犄�蓖�ĉ\�ȕ������W�X�^�̔ԍ����󂯎��
int RMT::AllocateReg( OpIterator op, int lno )
{
    HookParam hookParam = {op, lno, 0};
    HookEntry(
        this,
        &RMT::AllocateRegBody,
        &s_allocateRegHook,
        &hookParam 
    );
    return hookParam.physicalRegNum;
}

// Retire�����̂ŁA�l�����p����邱�Ƃ̂Ȃ��Ȃ镨�����W�X�^���������
void RMT::ReleaseReg( OpIterator op, const int lno, int phyRegNo )
{
    HookParam hookParam = { op, lno, phyRegNo };
    HookEntry(
        this,
        &RMT::ReleaseRegBody,
        &s_releaseRegHook,
        &hookParam 
    );
}

// Flush���ꂽ�̂�op�̃f�X�e�B�l�[�V�����E���W�X�^���t���[���X�g�ɖ߂�
void RMT::DeallocateReg( OpIterator op, const int lno, int phyRegNo )
{
    HookParam hookParam = { op, lno, phyRegNo };
    HookEntry(
        this,
        &RMT::DeallocateRegBody,
        &s_deallocateRegHook,
        &hookParam 
    );
}


// AllocateReg�̎���
void RMT::AllocateRegBody( HookParam* param )
{
    int lno = param->logicalRegNum;
    int segment = GetRegisterSegmentID( lno );

    ASSERT( 
        lno >= 0 && lno < m_numLogicalReg,
        "illegal register No.: %d\n", lno
    );

    // �t���[���X�g���畨�����W�X�^�ԍ����󂯎��
    int phyRegNo = m_regFreeList->Allocate( segment );

    // allocation���ɕ������W�X�^��������
    PhyReg* phyReg = (*m_registerFile)[ phyRegNo ];
    phyReg->Clear();

    // reg��commit���ɉ�����镨�����W�X�^��m_releaseTable�ɓo�^
    // ����܂�reg�̘_�����W�X�^�Ɋ��蓖�Ă��Ă����������W�X�^���������
    m_releaseTable[ phyRegNo ] = ( m_allocationTable.GetCurrent() )[ lno ];

    // <�_�����W�X�^�A�������W�X�^>�̃}�b�s���O�e�[�u�����X�V
    ( m_allocationTable.GetCurrent() )[ lno ] = phyRegNo;

    param->physicalRegNum = phyRegNo;
}

// Retire�����̂ŁA�l�����p����邱�Ƃ̂Ȃ��Ȃ镨�����W�X�^���������
void RMT::ReleaseRegBody( HookParam* param )
{
    int lno = param->logicalRegNum;
    ASSERT(
        lno >= 0 && lno < m_numLogicalReg,
        "illegal register No.: %d\n", lno 
    );
    
    int segment = GetRegisterSegmentID( lno );

    // ������镨�����W�X�^���t���[���X�g�ɖ߂�
    int releasedPhyReg = m_releaseTable[ param->physicalRegNum ];
    m_regFreeList->Release( segment, releasedPhyReg );
    param->physicalRegNum = releasedPhyReg;
}

// Flush���ꂽ�̂�op�̃f�X�e�B�l�[�V�����E���W�X�^���t���[���X�g�ɖ߂�
void RMT::DeallocateRegBody( HookParam* param )
{
    int lno = param->logicalRegNum;
    ASSERT(
        lno >= 0 && lno < m_numLogicalReg,
        "illegal register No.: %d\n", lno 
    );

    int segment = GetRegisterSegmentID( lno );

    m_regFreeList->Release( segment, param->physicalRegNum );
}


bool RMT::CanAllocate(OpIterator* infoArray, int numOp)
{
    // �K�v�ȃ��W�X�^�����Z�O�����g���Ƃɐ�����
    boost::array<size_t, SimISAInfo::MAX_REG_SEGMENT_COUNT> requiredRegCount;
    requiredRegCount.assign(0);

    for(int i = 0; i < numOp; ++i) {
        OpInfo* opInfo = infoArray[i]->GetOpInfo();

        int dstNum = opInfo->GetDstNum();
        for( int k = 0; k < dstNum; ++k ){
            int dstOperand = opInfo->GetDstOperand( k );
            int segment = GetRegisterSegmentID( dstOperand );
            ++requiredRegCount[segment];
        }
    }

    // �Z�O�����g���ƂɊ��蓖�ĉ\�����f
    for(int m = 0; m < m_numRegSegment; ++m){
        size_t freeRegCount = (size_t)m_regFreeList->GetFreeEntryCount(m);
        if( freeRegCount < requiredRegCount[m] ) {
            return false;
        }
    }
    return true;
}

// �_��/�������W�X�^�̌�
int RMT::GetRegSegmentCount()
{
    return m_numRegSegment;
}

int RMT::GetLogicalRegCount(int segment)
{
    if( segment >= m_numRegSegment )
        return 0;

    return m_segmentTable[segment];
}

int RMT::GetTotalLogicalRegCount()
{
    return m_numLogicalReg;
}
