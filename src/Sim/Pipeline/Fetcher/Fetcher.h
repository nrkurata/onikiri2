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


#ifndef SIM_PIPELINE_FETCHER_FETCHER_H
#define SIM_PIPELINE_FETCHER_FETCHER_H

#include "Types.h"
#include "Utility/Collection/fixed_size_buffer.h"

#include "Interface/Addr.h"
#include "Interface/OpInfo.h"
#include "Env/Param/ParamExchange.h"

#include "Sim/ISAInfo.h"
#include "Sim/Foundation/Checkpoint/CheckpointMaster.h"
#include "Sim/Pipeline/PipelineNodeBase.h"
#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/Op/OpClassStatistics.h"

namespace Onikiri
{
    class BPred;
    class Cache;
    class Thread;
    class EmulatorIF;
    class GlobalClock;
    class Scheduler;
    class RegDepPredIF;
    class MemDepPredIF;
    class MemOrderManager;
    class Cache;
    class CacheSystem;
    class ForwardEmulator;
    class FetchThreadSteererIF;

    //
    // A class that fetches ops.
    //
    class Fetcher : 
        public PipelineNodeBase
    {

    public:

        // A hook parameter for s_fetchHook.
        // Cannot use an usual calling scheme for hook, 
        // because 'op' may be not created yet at Fetch().
        struct FetchHookParam
        {
            OpIterator op;  
        };

        // A hook parameter for s_fetchSteeringHook.
        struct SteeringHookParam
        {   
            // out
            Thread* targetThread;                       // A selected thread.

            // in
            PhysicalResourceArray<Thread>* threadList;  // Candidate threads for steering.
            bool    update;                             // Update internal context or not.
        };

        struct FetchDecisionHookParam
        {   
            Thread* thread;
            PC pc;
            OpInfo** infoArray;
            int numOp;
            bool canFetch;
        };

        struct BranchPredictionHookParam
        {
            PC fetchGroupPC;
            OpIterator op;
        };

        // parameter mapping
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@FetchWidth",     m_fetchWidth );
                PARAM_ENTRY( "@FetchLatency" ,  m_fetchLatency );
                PARAM_ENTRY( "@IdealMode",      m_idealMode );
                PARAM_ENTRY( "@CheckLatencyMismatch",   m_checkLatencyMismatch );
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                RESULT_ENTRY( "@NumFetchedOps", m_numFetchedOp )
                BEGIN_PARAM_PATH( "NumStalledCycles/" )
                    RESULT_ENTRY( "@Total",             m_stallCycles.total )
                    RESULT_ENTRY( "@CurrentSyscall",    m_stallCycles.currentSyscall )
                    RESULT_ENTRY( "@NextSyscall",       m_stallCycles.nextSyscall )
                    RESULT_ENTRY( "@CheckpointFull",    m_stallCycles.checkpoint )
                    RESULT_ENTRY( "@InorderListFull",   m_stallCycles.inorderList )
                    RESULT_ENTRY( "@Others",            m_stallCycles.others )
                END_PARAM_PATH()
                RESULT_ENTRY( "@NumFetchedPCs",     m_numFetchedPC )
                RESULT_ENTRY( "@NumFetchGroups",    m_numFetchGroup )
                RESULT_RATE_ENTRY( 
                    "@AverageFetchGroupSize",
                    m_numFetchedPC, m_numFetchGroup
                )
                CHAIN_PARAM_MAP("OpStatistics", m_opClassStat)
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( EmulatorIF, "emulator", m_emulator )
            RESOURCE_ENTRY( Thread, "thread",   m_thread )
            RESOURCE_ENTRY( Core,   "core",     m_core )
            RESOURCE_ENTRY( BPred,  "bPred",    m_bpred )
            RESOURCE_ENTRY( CacheSystem,    "cacheSystem",  m_cacheSystem )
            RESOURCE_ENTRY( GlobalClock,"globalClock",  m_globalClock )
            RESOURCE_ENTRY( ForwardEmulator,"forwardEmulator",  m_forwardEmulator )
            RESOURCE_ENTRY( FetchThreadSteererIF, "fetchThreadSteerer", m_fetchThreadSteerer)
        END_RESOURCE_MAP()

        Fetcher();
        virtual ~Fetcher();

        // Initialization/Finalization
        virtual void Initialize(InitPhase phase);
        virtual void Finalize();

        // PipelineNodeIF
        virtual void Evaluate();
        virtual void Update();
        virtual void Commit(OpIterator op);

        // ���s�I�����ɌĂ΂��
        void Finished(OpIterator op);

        // accessors
        const int GetFetchWidth()   const { return m_fetchWidth;}
        BPred*      GetBPred()      const { return m_bpred;     }
        EmulatorIF* GetEmulator()   const { return m_emulator;  }

        void SetInitialNumFetchedOp(u64 num);

        //
        // --- Hook
        //
        // Prototype : void Method( HookParameter<Fetcher, FetchHookParam>* param )
        static HookPoint<Fetcher, FetchHookParam> s_fetchHook;
        
        // The hook point of 'GetFetchThread()'
        // Prototype : void Method( HookParameter<Fetcher, SteeringHookParam>* param )
        static HookPoint<Fetcher, SteeringHookParam> s_fetchSteeringHook;

        // The hook point of 'CanFetch()'
        // Prototype : void Method( HookParameter<Fetcher,FetchDecisionHookParam>* param )
        static HookPoint<Fetcher, FetchDecisionHookParam> s_fetchDecisionHook;

        // The hook point of 'PredictNextPC()'
        // Prototype : void Method( HookParameter<Fetcher,BranchPredictionParam>* param )
        static HookPoint<Fetcher, BranchPredictionHookParam> s_branchPredictionHook;

    protected:
        typedef PipelineNodeBase BaseType;

        // �t�F�b�`�ς�Op���i�[����z��
        typedef 
            fixed_sized_buffer< OpIterator, SimISAInfo::MAX_OP_INFO_COUNT_PER_PC, Fetcher >
            FetchedOpArray;

        int m_fetchWidth;       // fetch��
        int m_fetchLatency;     // fetch���C�e���V

        // �t�F�b�`����Op�̐�
        u64 m_numFetchedOp;
        // �t�F�b�`����PC�̐�
        u64 m_numFetchedPC;
        // �t�F�b�`�����t�F�b�`�O���[�v�̐� (1�ȏ�̖��߂��t�F�b�`������)
        u64 m_numFetchGroup;
        // �t�F�b�`�O���[�v���̕���
        int m_numBranchInFetchGroup;
        // �t�F�b�`�O���[�v���̕��򐔂̍ő� (����1�Œ�)
        static const int m_maxBranchInFetchGroup = 1;
        // ���@�̃t�F�b�`���s�� (1���߂��t�F�b�`��fetchWidth�{��)
        bool m_idealMode;
        // Check whether the number of fetch stages and the latency of a L1 cache match or not.
        bool m_checkLatencyMismatch;
        // ���Ƀt�F�b�`���s���X���b�h�̃C���f�b�N�X
        int m_currentFetchThread;

        // Updated content decided by Evaluate() in this cycle.
        struct Evaluated
        {
            // Whether InorderList is empty or not at this cycle.
            bool isInorderListEmpty;

            // Whether serializing is required or not
            bool reqSerializing;

            // Fetch PC
            PC fetchPC;

            // Fetch thread
            Thread* fetchThread;
            
            Evaluated() :
                isInorderListEmpty( false ),
                reqSerializing( false ),
                fetchThread( NULL )
            {
            }
        } m_evaluated;

        // ����\����
        BPred* m_bpred;                 

        // CacheSystem
        CacheSystem* m_cacheSystem;

        // ���߂̏��������Ă����G�~�����[�^
        EmulatorIF* m_emulator;

        // �O���[�o���N���b�N
        GlobalClock* m_globalClock;

        // Forward Emulator.
        ForwardEmulator* m_forwardEmulator;

        // FetchThreadSteerer
        FetchThreadSteererIF* m_fetchThreadSteerer;

        // ���v�p�̃J�E���^
        struct StallCycles
        {
            s64 currentSyscall; // �㗬�ɃV�X�e���R�[��������̂ŃX�g�[��
            s64 nextSyscall;    // ���Ƀt�F�b�`���閽�߂��V�X�e���R�[���Ȃ̂ŃX�g�[��
            s64 checkpoint;     // �`�F�b�N�|�C���g�̐�������Ȃ��̂ŃX�g�[��
            s64 inorderList;    // Stalled by the shortage of the entries of InorderList.
            s64 total;          // Total stalled cycles.
            s64 others;         
            StallCycles():
                currentSyscall(0), nextSyscall(0), checkpoint(0), inorderList(0), total(0), others(0)
            {
            }
        } m_stallCycles;

        // Statistics of ops.
        OpClassStatistics m_opClassStat;

        // �㗬���̖��߂����^�C�A���Ȃ��Ƃ����Ȃ����߂��ǂ���
        bool IsSerializingRequired( const OpInfo* const info ) const;
        bool IsSerializingRequired( OpIterator op ) const;
        bool IsSerializingRequired( Thread* thread ) const;

        void CreateCheckpoint(OpIterator op);
        void BackupOnCheckpoint( OpIterator op, bool before );

        // infoArray��numOp���� Op �� fetch ���� fetchedOp �Ɋi�[����֐�
        void Fetch( Thread* thread, FetchedOpArray& fetchedOp, PC pc, OpInfo** infoArray, int numOp );  

        // ����\�����s���֐�
        void PredictNextPC(OpIterator op, PC fetchGroupPC);
        void PredictNextPCBody( BranchPredictionHookParam* param );

        // iCache�ɃA�N�Z�X���ă��C�e���V��Ԃ��֐�
        const int GetICacheReadLatency(const PC& pc);

        // Enter an op to the fetcher pipeline.
        // An op is send to the renamer automatically when the op exits the fetch pipeline.
        void EnterPipeline( OpIterator op, int nextEventCycle );

        // array����op�������Ƃ��āC�����o�֐��|�C���^����Ăяo�����s��
        void ForEachOp(FetchedOpArray& c, int size, void (Fetcher::*func)(OpIterator));

        // ��Ɠ���������������Ƃ��
        template <typename T1>
        void ForEachOpArg1(FetchedOpArray& c, int size, T1 arg1, void (Fetcher::*func)(OpIterator, T1));

        // infoArray ���t�F�b�`�ł��邩�ǂ���
        void CanFetchBody( FetchDecisionHookParam* param );
        bool CanFetch( Thread* thread, PC pc, OpInfo** infoArray, int numOp );

        // Decide a thread that fetches ops in this cycle.
        Thread* GetFetchThread( bool Update );

    };

}; // namespace Onikiri

#endif // SIM_PIPELINE_FETCHER_FETCHER_H

