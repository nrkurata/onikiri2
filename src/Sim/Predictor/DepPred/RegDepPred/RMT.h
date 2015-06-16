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


#ifndef __RMT_H__
#define __RMT_H__

#include "Sim/Foundation/Checkpoint/CheckpointedData.h"
#include "Interface/EmulatorIF.h"
#include "Sim/Dependency/PhyReg/PhyReg.h"
#include "Sim/Register/RegisterFile.h"
#include "Sim/Register/RegisterFreeList.h"
#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/ISAInfo.h"

#include "Sim/Predictor/DepPred/RegDepPred/RegDepPredBase.h"

namespace Onikiri 
{

    // ���W�X�^�ˑ��֌W�\����̎���
    // �������W�X�^�̉���^�C�~���O�𐳊m�ɊǗ�

    class RMT :
        public RegDepPredBase, public PhysicalResourceNode
    {
    protected:
        int m_numLogicalReg;
        int m_numRegSegment;

        //
        Core* m_core;
        CheckpointMaster* m_checkpointMaster;

        // emulator
        EmulatorIF* m_emulator;

        // ���W�X�^�{��
        RegisterFile* m_registerFile;

        // �_�����W�X�^�̃Z�O�����g�����i�[����e�[�u��
        boost::array<int, SimISAInfo::MAX_REG_COUNT> m_segmentTable;

        // �t���[���X�g
        // Allocation���\�ȕ������W�X�^�̔ԍ����Ǘ�����
        RegisterFreeList* m_regFreeList;

        // �_�����W�X�^�ԍ����L�[�Ƃ����A�������W�X�^�̃}�b�s���O�e�[�u��
        CheckpointedData<
            boost::array< int, SimISAInfo::MAX_REG_COUNT >
        > m_allocationTable;

        // op�̃f�X�e�B�l�[�V�����E���W�X�^�̃R�~�b�g���ɉ������镨�����W�X�^�̃}�b�s���O�e�[�u��
        // op�̃f�X�e�B�l�[�V�����E���W�X�^�̕������W�X�^�ԍ����L�[�Ƃ���
        std::vector<int> m_releaseTable;

        // member methods
        int GetRegisterSegmentID(int index)
        {
            return m_segmentTable[index];
        }

    public:
        // s_releaseRegHook ����op �́C�������郌�W�X�^�̎�����ł͂Ȃ��C
        // ���̃��W�X�^�̉�����g���K�������߂�op
        struct HookParam
        {   
            OpIterator op;
            int logicalRegNum;
            int physicalRegNum;
        };

    protected:
        // AllocateReg/ReleaseReg/DeallocateReg �̎���
        virtual void AllocateRegBody( HookParam* param );
        virtual void ReleaseRegBody ( HookParam* param );
        virtual void DeallocateRegBody( HookParam* param ); 

    public:
        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( EmulatorIF, "emulator", m_emulator )
            RESOURCE_ENTRY( Core, "core", m_core )
            RESOURCE_ENTRY( RegisterFile, "registerFile", m_registerFile )
            RESOURCE_ENTRY( RegisterFreeList, "registerFreeList", m_regFreeList )
            RESOURCE_ENTRY( CheckpointMaster, "checkpointMaster", m_checkpointMaster )
        END_RESOURCE_MAP()

        RMT();
        virtual ~RMT();

        virtual void Initialize(InitPhase phase);

        // �\�[�X�E���W�X�^�ɑΉ����镨�����W�X�^�ԍ���Ԃ�
        virtual int ResolveReg(const int lno);

        // ResolveReg �Ɠ��l�ɕ������W�X�^�ԍ���Ԃ��D
        // �������C���݂̊��蓖�ď�Ԃ��ϑ�����݂̂ŁC
        // ����p���Ȃ����Ƃ�ۏ؂���D
        virtual int PeekReg(const int lno) const;

        // �f�X�e�B�l�[�V�����E���W�X�^�ɕ������W�X�^�ԍ������蓖�Ă�
        virtual int AllocateReg(OpIterator op, const int lno);

        // retire�����̂ŁAop��������ׂ��������W�X�^�����
        virtual void ReleaseReg(OpIterator op, const int lno, int phyRegNo);
        // flush���ꂽ�̂ŁAop�̃f�X�e�B�l�[�V�����E���W�X�^�����
        virtual void DeallocateReg(OpIterator op, const int lno, int phyRegNo); 

        // num�������W�X�^�����蓖�Ă邱�Ƃ��ł��邩�ǂ���
        virtual bool CanAllocate(OpIterator* infoArray, int numOp);

        // �_��/�������W�X�^�̌�
        virtual int GetRegSegmentCount();
        virtual int GetLogicalRegCount(int segment);
        virtual int GetTotalLogicalRegCount();

        //
        // Hook
        //

        // Prototype : void Method( HookParameter<RMT,HookParam>* param )
        static HookPoint<RMT,HookParam> s_allocateRegHook;
        static HookPoint<RMT,HookParam> s_releaseRegHook;
        static HookPoint<RMT,HookParam> s_deallocateRegHook;

    };

}; // namespace Onikiri

#endif // __RMT_H__

