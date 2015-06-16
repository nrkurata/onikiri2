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


#ifndef SIM_DUMPER_VISUALIZATION_DUMPER_H
#define SIM_DUMPER_VISUALIZATION_DUMPER_H

#include <fstream>
#include <string>

#include "Utility/String.h"

#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Op/OpContainer/OpExtraStateTable.h"
#include "Sim/Foundation/Resource/ResourceNode.h"

#include "Env/Param/ParamExchange.h"
#include "Sim/Dumper/DumpState.h"

namespace Onikiri
{
    class Core;
    class Thread;

    // simulation�̌��ʂ�Visualize���邽�߂̃N���X
    class VisualizationDumper : public ParamExchange
    {
    public:
        // parameter mapping
        BEGIN_PARAM_MAP( "/Session/Environment/Dumper/VisualizationDumper/" )
            PARAM_ENTRY( "@FileName",   m_visFileName )
            PARAM_ENTRY( "@EnableDump", m_enabled )
            PARAM_ENTRY( "@EnableGzip", m_gzipEnabled )
            PARAM_ENTRY( "@GzipLevel",  m_gzipLevel )
            PARAM_ENTRY( "@SkipInsns",  m_skipInsns )
        END_PARAM_MAP()

        // constructor/destructor
        VisualizationDumper();
        ~VisualizationDumper();

        void Initialize( const String& suffix, PhysicalResourceArray<Core>& coreList );
        void Finalize();
        bool IsEnabled();

        // VisDump�������������߂ɊO����Ă΂��֐�

        // VisualizationDumper�Ɍ��݂�cycle/insn���������邽�߂̊֐�
        void SetCurrentCycle( const s64 cycle );
        void SetCurrentInsnCount( Thread* thread, const s64 count );

        // op�̌��݂�State(PipelineStage/Retire/Flush�Ȃ�)��\������
        void PrintOpState( OpIterator op, DUMP_STATE state );

        // Dump the begin/end of stall.
        void PrintStallBegin( OpIterator op );
        void PrintStallEnd  ( OpIterator op );

        // Print dependency arrows from producerOp to consumerOp.
        // This method is typically used for printing wakeup.
        void PrintOpDependency(
            const OpIterator producerOp, 
            const OpIterator consumerOp, 
            DumpDependency type 
        );

        // Print user defined stages. 
        // See comments for DumpLane.
        //   op:    Dumped op.
        //   begin: The stage is begun or not.
        //   stage: User defined stage string.
        //   lane:  A lane of the stage.
        void PrintRawOutput( OpIterator op, bool begin, const char* stage, DumpLane lane );

    private:
        
        static const u64 INVALID_VIS_SERIAL = ~((u64)0);
        
        // Each op has OpState, a extra state for dump.
        struct OpState 
        {
            DUMP_STATE dumpState;   // Op�̍Ō��State
            u64 visSerialID;        // �o�̓t�@�C�����ł̃V���A��
            bool inPipeline;        // �iKanata�ŕ\���\�Ȕ͈͂́j�p�C�v���C�����ɂ��邩�ǂ���
            bool stalled;           // Op is stalled or not.

            struct Stage
            {
                bool in;
                std::string name;
                Stage() : in(false)
                {
                }
            };
            std::map< DumpLane, Stage > lastRawStages;

            OpState();
        };

        // OpState can be retrieved from CoreState like the following:
        // GetOpState() or 
        // m_coreStateTable[ op->GetCore() ]->opState[ op ]
        struct CoreState
        {
            OpExtraStateTable<OpState> opState;
        };

        struct ThreadState
        {
            s64 retiredInsnCount;
        };
        
        // Each core/thread has extra information for dump.
        typedef unordered_map< Core*, CoreState >       CoreStateMap;
        typedef unordered_map< Thread*, ThreadState >   ThreadStateMap;
        CoreStateMap    m_coreStateTable; 
        ThreadStateMap  m_threadStateTable;

        OpState*   GetOpState( OpIterator op );
        DUMP_STATE GetLastState( OpIterator op );
        void SetLastState  ( OpIterator op, DUMP_STATE state );
        u64  GetVisSerialID( OpIterator op );
        void SetVisSerialID( OpIterator op, u64 visSerialID );
        
        s64 GetTotalRetiredInsnCount();

        bool m_enabled;     // VisDump�������������ǂ���
        bool m_gzipEnabled; // VisDump��gzip���k���邩�ǂ���
        int  m_gzipLevel;   // gzip�̈��k���x��

        std::string m_visFileName; // visDump���o�͂���t�@�C����
        boost::iostreams::filtering_ostream m_visStream;    // visDump���o�͂���t�@�C���X�g���[��

        s64 m_visCurrentCycle;  // �V�~�����[�^�̌��݂̎���
        s64 m_visLastPrintCycle;
        s64 m_skipInsns;        // The count of insns for skipping dump.

        u64 m_visSerialID;      // ���O�t�@�C�����ł̃V���A��ID

        // �_���v�̃X�L�b�v����
        bool IsDumpSkipped( OpIterator op );

        // DUMP_STATE <> �\�����ϊ�
        const char* ToStringFromDumpState( DUMP_STATE state );

        // Print initialization of 'op'.
        // PrintOpInitLabel is only called from PrintOpInit.
        void PrintOpInit( const OpIterator op );
        void PrintOpInitLabel( const OpIterator op );

        // Print a label text.
        enum OpLabelType{
            POLT_LABEL  = 0,
            POLT_DETAIL = 1
        };
        void PrintOpLabel( const OpIterator op, OpLabelType type, const std::string& label );

        // ���݂̃T�C�N����\��
        void PrintCycle();

        // op���O��PipelineStage����o�����Ƃ�\��
        void PrintLastPipelineStage( const OpIterator op, DUMP_STATE lastState );

        // op��Retire/Flush���ꂽ���Ƃ�\��
        void PrintOpEnd( const OpIterator op, DUMP_STATE lastState );
        void PrintOpEndLabel( const OpIterator op );

        // op���V����PipelineStage�ɓ��������Ƃ�\��
        void PrintNextPipelineStage( const OpIterator op, DUMP_STATE state );
        
        // Print stall states.
        void PrintStallBody( const OpIterator op, bool stall );
        void PrintStall( const OpIterator op, bool stall );

    };

};

#endif //SIM_DUMPER_VIS_DUMPER_H


