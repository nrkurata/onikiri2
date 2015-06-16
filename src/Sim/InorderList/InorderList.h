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


#ifndef SIM_INORDER_LIST_INORDER_LIST_H
#define SIM_INORDER_LIST_INORDER_LIST_H

#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Op/OpInitArgs.h"
#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/ExecUnit/ExecUnitIF.h"
#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/Core/DataPredTypes.h"
#include "Sim/Op/OpContainer/OpList.h"

namespace Onikiri 
{
    class Core;
    class Thread;
    class CheckpointMaster;
    class Checkpoint;
    class MemOrderManager;
    class OpNotifier;
    class MemOrderManager;
    class RegDepPredIF;
    class MemDepPredIF;
    class Fetcher;
    class Dispatcher;
    class Renamer;
    class CacheSystem;
    class Retirer;

    class InorderList :
        public PhysicalResourceNode
    {

    public:

        // parameter mapping
        BEGIN_PARAM_MAP("")
            
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@Capacity", m_capacity );
                PARAM_ENTRY( "@RemoveOpsOnCommit", m_removeOpsOnCommit );
            END_PARAM_PATH()

            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@NumRetiredOps",   m_retiredOps )
                PARAM_ENTRY( "@NumRetiredInsns", m_retiredInsns )
            END_PARAM_PATH()

        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( CheckpointMaster, "checkpointMaster", m_checkpointMaster )
            RESOURCE_ENTRY( Core,     "core",       m_core )
            RESOURCE_ENTRY( Thread,   "thread",     m_thread )
        END_RESOURCE_MAP()

        InorderList();
        virtual ~InorderList();

        // �������p���\�b�h
        void Initialize(InitPhase phase);

        // --- Op �̐����A�폜�Ɋւ�郁���o�֐�
        OpIterator ConstructOp(const OpInitArgs& args);
        void DestroyOp(OpIterator op);

        // --- �t�F�b�`�Ɋւ��֐�
        // Fetcher �ɂ���ăt�F�b�`���ꂽ
        void PushBack(OpIterator op);

        // --- Remove a back op from a list.
        void PopBack();

        // --- �擪/�����̖��߂₠�閽�߂̑O/���̖��߂𓾂�֐�
        // �v���O�����E�I�[�_�Ő擪�̖��߂�Ԃ�
        OpIterator GetCommittedFrontOp();           // �R�~�b�g���̖��߂��܂ޑS���ߒ��̐擪�̖��߂�Ԃ�
        OpIterator GetFrontOp();    // ���R�~�b�g���ߒ��̐擪��Ԃ�
        // �Ō�Ƀt�F�b�`���ꂽ���߂�Ԃ�
        OpIterator GetBackOp();  

        // op�̑O/����Index�������Ă���Op��Ԃ�
        // PC�ɑ΂��ĕ�����Op������ꍇ�ł��֌W�Ȃ��ɑO/��
        OpIterator GetPrevIndexOp(OpIterator op);
        OpIterator GetNextIndexOp(OpIterator op);
        
        // op�̑O/����PC�������Ă���Op��Ԃ�
        // PC�ɑ΂��ĕ�����Op������ꍇ�͔�΂��đO/��
        OpIterator GetPrevPCOp(OpIterator op);
        OpIterator GetNextPCOp(OpIterator op);
        // op�Ɠ���PC�����擪��Op(MicroOp�̐擪)��Ԃ�
        OpIterator GetFrontOpOfSamePC(OpIterator op);

        // �󂩂ǂ���
        bool IsEmpty();

        // Return whether an in-order list can reserve entries or not.
        bool CanAllocate( int ops );

        // Returns its capacity.
        int GetCapacity() const { return m_capacity; }

        // Returns the number of retired instructions.
        s64 GetRetiredInstructionCount() const { return m_retiredOps; }


        // 
        // --- ���߂̑���
        //

        // �n���ꂽ���߂��R�~�b�g���C���^�C�A������
        void Commit( OpIterator op );
        void Retire( OpIterator op );

        // accessors
        OpNotifier* GetNotifier() const { return m_notifier; }
        s64 GetRetiredOps()     const { return m_retiredOps;    }
        s64 GetRetiredInsns()   const { return m_retiredInsns;  }

        // This method is called when a simulation mode is changed.
        virtual void ChangeSimulationMode( PhysicalResourceNode::SimulationMode mode )
        {
            m_mode = mode;
        }

        // ����\���~�X����op ���t���b�V������ۂ̃t�b�N
        static HookPoint<InorderList> s_opFlushHook;

        // Flush all backward ops from 'startOp'.
        // This method flushes ops including 'startOp' itself.
        // This method must be called from 'Recoverer' because this method does not 
        // recover processor states from check-pointed data.
        int FlushBackward( OpIterator startOp );


    protected:
        // member variables
        Core*               m_core;
        CheckpointMaster*   m_checkpointMaster;
        Thread*             m_thread;
        OpNotifier*         m_notifier;
        MemOrderManager*    m_memOrderManager;
        RegDepPredIF*       m_regDepPred;
        MemDepPredIF*       m_memDepPred;
        Fetcher*            m_fetcher;
        Renamer*            m_renamer;
        Dispatcher*         m_dispatcher;
        Retirer*            m_retirer;
        CacheSystem*        m_cacheSystem;

        // Ops are pushed into these list in program-order.
        OpList  m_inorderList;      // A list of ops that are not committed yet. This list corresponds to a ROB.
        OpList  m_committedList;    // A list of ops that are committed and are not retired yet.

        OpArray* m_opArray;

        PhysicalResourceNode::SimulationMode m_mode;

        int m_capacity;             // The capacity of an in-order list(ROB).
        bool m_removeOpsOnCommit;   // If this parameter is true, ops are removed on commitment,
                                    // otherwise ops are removed on retirement.

        s64 m_retiredOps;
        s64 m_retiredInsns;

        // Notify that 'op' is committed/retired/flushed to modules. 
        void NotifyCommit( OpIterator op );
        void NotifyRetire( OpIterator op );
        void NotifyFlush( OpIterator op );

        // �n���ꂽ���߂��t���b�V������
        // �����ł̃t���b�V���Ƃ͕���\���~�X���̂悤��
        // �p�C�v���C������̖��߂̍폜���Ӗ�����
        void Flush( OpIterator op );        // �t�b�N���܂߂������

    };  // class InorderList

}   // namespace Onikiri 

#endif  // #ifndef SIM_INORDER_LIST_INORDER_LIST_H


