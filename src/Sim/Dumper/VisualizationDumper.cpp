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

#include "Sim/Dumper/VisualizationDumper.h"

#include "Sim/Dumper/DumpState.h"
#include "Sim/Op/Op.h"
#include "Sim/Core/Core.h"
#include "Sim/Dumper/DumpFileName.h"
#include "Version.h"

using namespace Onikiri;
using namespace std;
using namespace boost;

static const char g_kanataFileFormatHeader[] = "Kanata";

VisualizationDumper::OpState::OpState() :
    dumpState( DS_INVALID ),
    visSerialID( INVALID_VIS_SERIAL ),
    inPipeline( false ),
    stalled( false ),
    lastRawStages()
{
}

static const String MakeKanataHeaderString()
{
    String headerStr;
    headerStr.format(
        "%s\t%04x\n", 
        g_kanataFileFormatHeader,
        ONIKIRI_KANATA_FILE_VERSION
    );
    return headerStr;
}

VisualizationDumper::VisualizationDumper()
{
    m_visLastPrintCycle = 0;
    m_visCurrentCycle = 0;
    m_enabled = false;
    m_skipInsns = 0;
    m_visSerialID = 0;
}

VisualizationDumper::~VisualizationDumper()
{
}

//
// m_coreStateTable �ւ̃A�N�Z�T
//

VisualizationDumper::OpState* VisualizationDumper::GetOpState( OpIterator op )
{
    return &m_coreStateTable[op->GetCore()].opState[op];
}

DUMP_STATE VisualizationDumper::GetLastState( OpIterator op )
{
    return GetOpState(op)->dumpState;
}

void VisualizationDumper::SetLastState( OpIterator op, DUMP_STATE state )
{
    GetOpState(op)->dumpState = state;
}

u64 VisualizationDumper::GetVisSerialID( OpIterator op )
{
    return GetOpState(op)->visSerialID;
}

void VisualizationDumper::SetVisSerialID( OpIterator op, u64 visSerialID)
{
    GetOpState(op)->visSerialID = visSerialID;
}


bool VisualizationDumper::IsEnabled()
{
    return m_enabled;
}

// �_���v�̃X�L�b�v����
bool VisualizationDumper::IsDumpSkipped( OpIterator op )
{
    return ( GetTotalRetiredInsnCount() < m_skipInsns ) ? true : false;
}

// DUMP_STATE <> �\�����ϊ�
const char* VisualizationDumper::ToStringFromDumpState( DUMP_STATE state )
{
    return g_visualizerDumperStrTbl[state];
}

//
// Additional label information is printed by 'L' command.
//
// �C�ӂ̖��߂̏��i���߃A�h���X��Op�R�[�h�C���W�X�^�ԍ��Ȃǁj�D
// ����͂��̂܂�Kanata �ɕ\�������D
//
void VisualizationDumper::PrintOpInitLabel( const OpIterator op )
{
    stringstream label;
    label <<
        hex << op->GetPC().address <<
        dec << " ";

    OpInfo* opInfo = op->GetOpInfo();

    // dst operand�Ɋւ���\��
    int dstNum = op->GetOpInfo()->GetDstNum();
    if( dstNum != 0 ){
        if( dstNum == 1 ){
            label << "r" << opInfo->GetDstOperand( 0 );
        }
        else{
            label << "(";

            bool first = true;
            for( int i = 0; i < opInfo->GetDstNum(); i++ ){
                if( !first ){
                    label << ", ";
                }
                label << "r" << opInfo->GetDstOperand( i );
                first = false;
            }
            label << ")";
        }

        label << " = ";
    }

    // op class��\��
    //label << OpClassCode::ToString(op->GetOpClass().GetCode()) << "(";
    // �j�[���j�b�N��\��
    label << opInfo->GetMnemonic() << "(";

    // src operand�Ɋւ���\��
    bool first = true;
    for (int j = 0; j < opInfo->GetSrcNum(); j++)
    {
        if (!first) label << ", ";
        label << "r" << opInfo->GetSrcOperand(j);
        first = false;
    }
    label << ")";

    PrintOpLabel( op, POLT_LABEL, label.str() );
}

//
// Fetch���ꂽop�̏���\������
//
// �o�͗�F
// >> I 0 0 0
// ���߂Ɋւ��郍�O���o�͂���O�ɂ��ꂪ�K�v 
// 2��� : �t�@�C�����ň�ӂ�ID 
// 3��� : ���߂̈�ӂ�ID 
// 4��� : TID�i�X���b�h���ʎq�j 
//
void VisualizationDumper::PrintOpInit(OpIterator op)
{
    // Update visSerialID.
    u64 visSerialID = m_visSerialID;
    m_visSerialID++;
    SetVisSerialID( op, visSerialID );

    // Print 'I' command.
    m_visStream <<
        "I\t" << 
        visSerialID             << "\t" <<
        op->GetGlobalSerialID() << "\t" <<
        op->GetTID() << "\n";
    
    // Additional label information is printed by 'L' command in PrintOpInitLabel.
    PrintOpInitLabel( op );

    // Update an op extra state.
    OpState* opState = GetOpState( op );
    opState->inPipeline = true;
    opState->stalled    = false;
    opState->lastRawStages.clear();
}



// Print a label text 
void VisualizationDumper::PrintOpLabel(  const OpIterator op, OpLabelType type, const string& label )
{
    m_visStream <<
        "L\t" << 
        GetVisSerialID( op )    << "\t" <<
        (type == POLT_LABEL ? '0' : '1' ) << "\t" <<
        label << "\n";
}

void VisualizationDumper::PrintCycle()
{
    stringstream stream;
    if (m_enabled && m_visLastPrintCycle != m_visCurrentCycle) {
        if (m_visLastPrintCycle == 0) {
            stream << "C=\t" << m_visCurrentCycle << "\n";
        } else {
            stream << "C\t" << (m_visCurrentCycle - m_visLastPrintCycle) << "\n";
        }
        m_visLastPrintCycle = m_visCurrentCycle;
    }
    m_visStream << stream.str();
}


// op���V����PipelineStage�ɓ��������Ƃ�\��
void VisualizationDumper::PrintNextPipelineStage( const OpIterator op, DUMP_STATE state )
{
    m_visStream <<
        "S\t" << 
        GetVisSerialID( op ) << "\t" << 
        "0" /*lane*/         << "\t" << 
        ToStringFromDumpState(state)         << "\n";
}

// op���O��PipelineStage����o�����Ƃ�\��
void VisualizationDumper::PrintLastPipelineStage( const OpIterator op, DUMP_STATE lastState )
{
    m_visStream <<
        "E\t" << 
        GetVisSerialID( op ) << "\t" << 
        "0" /*lane*/         << "\t" << 
        ToStringFromDumpState(lastState)     << "\n";
}

void VisualizationDumper::PrintStallBody( OpIterator op, bool stall )
{
    const char* tag = stall ? "S\t" : "E\t";
    m_visStream <<
        tag << 
        GetVisSerialID( op ) << "\t" <<
        "1\t"/*lane*/ << 
        "stl\t" <<
        "\n";
}

// Raw output 
void VisualizationDumper::PrintRawOutput( 
    OpIterator op, bool begin, const char*stage, DumpLane lane 
){
    if(!m_enabled)
        return;
    if( IsDumpSkipped( op ) )
        return;

    OpState* opState = GetOpState( op );
    if( !opState->inPipeline )
        return;

    OpState::Stage* info = &opState->lastRawStages[lane];

    if( begin && info->in ){
        PrintRawOutput( op, false, info->name.c_str(), lane );
    }

    PrintCycle();

    const char* tag = begin ? "S\t" : "E\t";
    m_visStream <<
        tag << 
        GetVisSerialID( op ) << "\t" <<
        lane  << "\t" << 
        stage << "\t" <<
        "\n";

    info->in   = begin;
    info->name = stage;
}

void VisualizationDumper::PrintStall( const OpIterator op, bool stall )
{
    if(!m_enabled)
        return;
    if( IsDumpSkipped( op ) )
        return;
    if( !GetOpState( op )->inPipeline )
        return;

    OpState* state = GetOpState( op );
    ASSERT( 
        state->stalled != stall, 
        "%s\nOp serial ID: %lld",
        stall ?
            "Stall overlap." : 
            "The op that is not stalled is finished from stall.",
        op->GetSerialID() 
    );
    state->stalled = stall;

    PrintCycle();
    PrintStallBody( op, stall );
}

void VisualizationDumper::PrintStallBegin( const OpIterator op )
{
    PrintStall( op, true );
}

void VisualizationDumper::PrintStallEnd( const OpIterator op )
{
    PrintStall( op, false );
}

// op��Retire/Flush���ꂽ���Ƃ�\��
void VisualizationDumper::PrintOpEndLabel( const OpIterator op )
{
    OpInfo* opInfo = op->GetOpInfo();
    OpStatus opState = op->GetStatus();
    stringstream label;

    // ���W�X�^���
    if( opState >= OpStatus::OS_RENAME ){
        
        // dst operand�Ɋւ���\��
        int dstNum = opInfo->GetDstNum();
        if( dstNum != 0 ){
            if( dstNum == 1 ){
                label << "p" << op->GetDstReg(0) <<
                    ":0x" << hex << op->GetDst(0) << dec;
            } 
            else{
                bool first = true;
                label << "(";
                for (int i = 0; i < opInfo->GetDstNum(); i++){
                    if( !first ){
                        label << ", ";
                    }
                    label << "p" << op->GetDstReg(i) <<
                        ":0x" << hex << op->GetDst(i) << dec;
                    first = false;
                }
                label << ")\n";
            }
            label << " = ";
        }

    }

    // op class��\��
    label << OpClassCode::ToString(op->GetOpClass().GetCode()) << "( ";
    // �j�[���j�b�N��\��
    //label << opInfo->GetMnemonic() << "( ";

    // src operand�Ɋւ���\��
    if( opState >= OpStatus::OS_RENAME ){
        bool first = true;
        for( int j = 0; j < opInfo->GetSrcNum(); j++ ){
            if( !first ){
                label << ", ";
            }
            label <<
                "p" << op->GetSrcReg(j) <<
                ":0x" << hex << op->GetSrc(j) << dec;
            first = false;
        }
    }

    label << " )";

    // Dump a memory address.
    if( op->GetOpClass().IsMem() ){
        label <<
            " [addr: 0x" << 
            hex << op->GetMemAccess().address.address << dec << "]";
    }


    // �o��
    PrintOpLabel( op, POLT_DETAIL, label.str() );

}

void VisualizationDumper::PrintOpEnd( const OpIterator op, DUMP_STATE state )
{
    // Close all unclosed raw stages.
    OpState* opExState = GetOpState( op );

    typedef map< DumpLane, OpState::Stage >::iterator iterator;
    for( iterator i = opExState->lastRawStages.begin(); 
        i != opExState->lastRawStages.end();
        ++i
    ){
        if( i->second.in ){
            PrintRawOutput( op, false, i->second.name.c_str(), i->first );
        }
    }

    PrintOpEndLabel( op );

    // ��{���
    m_visStream << 
        "R\t" << 
        GetVisSerialID( op ) << "\t" <<                 // serial ID
        op->GetRetireID() << "\t" <<                    // Retire ID
        (state == DS_RETIRE ? "0" : "1") << "\n";       // Retire:0 Flush:1
    
    // Update an op state.
    GetOpState( op )->inPipeline = false;
}

// open FileStream
void VisualizationDumper::Initialize( 
    const String& suffix, 
    PhysicalResourceArray<Core>& coreList 
){
    m_visCurrentCycle = 0;
    m_visLastPrintCycle = 0;

    LoadParam();
    if( IsEnabled() ){
        String fileName = 
            g_env.GetHostWorkPath() + 
            MakeDumpFileName( m_visFileName, suffix, m_gzipEnabled );

        if (m_gzipEnabled){
            m_visStream.push( 
                iostreams::gzip_compressor( 
                    iostreams::gzip_params(m_gzipLevel) 
                )
            );
        }

        m_visStream.push( iostreams::file_sink( fileName, ios::binary ) );
        m_visStream << MakeKanataHeaderString().c_str();


        for( int i = 0; i < coreList.GetSize(); i++ ){
            CoreState* coreState = &m_coreStateTable[ coreList[i] ];
            coreState->opState.Resize( *coreList[i]->GetOpArray() );
            for( int j = 0; j < coreList[i]->GetThreadCount(); j++ ){
                m_threadStateTable[ coreList[i]->GetThread(j) ].retiredInsnCount = 0;
            }
            
        }
    }
}

// close FileStream
void VisualizationDumper::Finalize()
{
    ReleaseParam();
}

// op�̌��݂�State(PipelineStage/Retire/Flush�Ȃ�)��\��
// State����Z�k���ďȗ������`�ŕ\�������(shrink(State))
// shrink()�̒��œo�^����Ă��Ȃ����̂�State�̐擪3���������\�������
void VisualizationDumper::PrintOpState(OpIterator op, DUMP_STATE state)
{
    if( !m_enabled )
        return;
    
    if( IsDumpSkipped( op ) )
        return;

    if( !GetOpState( op )->inPipeline && state != DS_FETCH )
        return;

    PrintCycle();

    // ���߂�op��visDump�ɏ���������(Fetch��)�Aop�̏���\������
    if(state == DS_FETCH ){
        PrintOpInit(op);
    }
    else{
        // �O��stage���������ꍇ�A����stage�̏I����\��: Fetch���ȊO
        PrintLastPipelineStage( op, GetLastState(op) );
    }

    if( state == DS_RETIRE || state == DS_FLUSH ){
        // PipelineStage�̑S�̂̏I����\���Fop��Retire/Flush���ꂽ��
        PrintOpEnd( op, state );
    } else {
        // op���V����PipelineStage�ɓ��������Ƃ�\��
        PrintNextPipelineStage( op, state );
        SetLastState( op, state ); // �Ō��stage���X�V
    }
}


// op���ق���op��WakeUp�������ɌĂ΂�(OpWakeUpEvent)�Aop�Ԃ�Dependency��\��
void VisualizationDumper::PrintOpDependency(
    const OpIterator producerOp, 
    const OpIterator consumerOp, 
    DumpDependency type 
){
    if( !m_enabled )
        return;
    if( IsDumpSkipped( producerOp ) )
        return;

    if( !GetOpState( consumerOp )->inPipeline || !GetOpState( producerOp )->inPipeline )
        return;

    PrintCycle();
    m_visStream << 
        "W\t" <<
        GetVisSerialID( consumerOp ) << "\t" << 
        GetVisSerialID( producerOp ) << "\t" << 
        type << "\n";
}

// VisualizationDumper�Ɍ��݂�cycle���������邽�߂̊֐�
void VisualizationDumper::SetCurrentCycle(const s64 cycle)
{
    m_visCurrentCycle = cycle;
}

void VisualizationDumper::SetCurrentInsnCount( Thread* thread, const s64 count )
{
    m_threadStateTable[thread].retiredInsnCount = count;
}

s64 VisualizationDumper::GetTotalRetiredInsnCount()
{
    s64 count = 0;
    for( ThreadStateMap::iterator i = m_threadStateTable.begin(); i != m_threadStateTable.end(); ++i ){
        count += i->second.retiredInsnCount;
    }
    return count;
}
