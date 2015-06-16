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


#ifndef EMU_UTILITY_COMMON_EMULATOR_H
#define EMU_UTILITY_COMMON_EMULATOR_H

#include "Emu/Utility/GenericOperation.h"
#include "Interface/EmulatorIF.h"
#include "Interface/OpStateIF.h"
#include "Interface/OpClassCode.h"
#include "Env/Param/ParamExchange.h"
#include "SysDeps/Endian.h"
#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/ProcessState.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Memory/MemoryUtility.h"
#include "Emu/Utility/System/VirtualSystem.h"
#include "Emu/Utility/ExtraOpInfoWrapper.h"
#include "Emu/Utility/OpEmulationState.h"
#include "Emu/Utility/SkipOp.h"
#include "Emu/Utility/GenericOperation.h"

namespace Onikiri {
    class SystemIF;

    namespace EmulatorUtility {

        // �G�~�����[�^
        template <class Traits>
        class CommonEmulator : 
            public EmulatorIF,
            public MemIF, 
            public ParamExchange
        {
        private:
            // Traits �́C�����̌^��typedef�����\����
            // �����̌^���e�V�X�e���ɓ��������^�ɒ�`���邱�ƂŁC�e�V�X�e����p�̃G�~�����[�^���`�ł���
            typedef typename Traits::ISAInfoType ISAInfoType;
            typedef typename Traits::OpInfoType OpInfoType;
            typedef typename Traits::ConverterType ConverterType;
            typedef typename Traits::LoaderType LoaderType;
            typedef typename Traits::SyscallConvType SyscallConvType;

            typedef ParamExchange ParamExchangeType;
        public:
            CommonEmulator( SystemIF* simSystem );
            virtual ~CommonEmulator();

            // ParamExchange ���̊֐���錾
            using ParamExchangeType::LoadParam;
            using ParamExchangeType::ReleaseParam;
            using ParamExchangeType::GetRootPath;
            using ParamExchangeType::ParamEntry;

            // EmulatorIF �̎���
            virtual std::pair<OpInfo**, int> GetOp(PC pc);
            virtual MemIF* GetMemImage();
            virtual void Execute(OpStateIF* op, OpInfo* opinfo);
            virtual void Commit(OpStateIF* op, OpInfo* opinfo);
            virtual int GetProcessCount() const;
            virtual PC GetEntryPoint(int pid) const;
            virtual u64 GetInitialRegValue(int pid, int index) const;
            virtual ISAInfoIF* GetISAInfo();
            virtual PC Skip(PC pc, u64 skipCount, u64* regArray, u64* executedInsnCount, u64* executedOpCount);
            virtual void SetExtraOpDecoder( ExtraOpDecoderIF* extraOpDecoder );

            // MemIF �̎���
            virtual void Read( MemAccess* access );
            virtual void Write( MemAccess* access );

        private:
            bool CreateProcesses( SystemIF* simSystem );
            
            // �������̂��߂̔�o�[�`�����Ȏ���
            std::pair<OpInfo**, int> GetOpBody(PC pc);

            ISAInfoType m_alpha64Info;
            std::vector<EmulatorUtility::ProcessState*> m_processes;
            ConverterType m_insnConverter;

            // �ϊ��������ʂ� OpInfo ���L���b�V��
            boost::pool<> m_opInfoArrayPool;
            boost::object_pool< OpInfoType > m_opInfoPool;
            boost::object_pool< ExtraOpInfoWrapper<ISAInfoType> > m_exOpInfoWrapperPool;

            class CodeWordHash
            {
            public:
                size_t operator()(const u32 value) const
                {
                    return (size_t)((value >> 26) ^ value);
                }
            };
            // �A�h���X����C���̃A�h���X�̖��߂ɑΉ�����OpInfo�𓾂�
            typedef std::pair<OpInfo**, int> OpInfoArray;
            std::vector< std::vector< OpInfoArray > > m_ProcessOpInfoCache;
            // ���ߌꂩ��CopInfoArray ���̑Ή�����OpInfo�̃|�C���^�ւ̃|�C���^�� OpInfo �̐��𓾂�
            typedef unordered_map<u32, OpInfoArray, CodeWordHash> CodeWordOpInfoMap;
            CodeWordOpInfoMap m_codeWordToOpInfo;

            // param_map
            int m_processCount;
            
            // �O�����߃f�R�[�_
            ExtraOpDecoderIF* m_extraOpDecoder;

            // CRCs of PCs and results of all retired ops.
            bool m_enableResultCRC;
            typedef typename boost::crc_32_type CRC_Type;
            std::vector< CRC_Type > m_resultCRCs;
            void CalcCRC( const PC& pc, u64 dstResult );
            std::vector< u32 > GetCRC_Result()
            {
                std::vector< u32 > result;
                for( std::vector< CRC_Type >::iterator i = m_resultCRCs.begin();
                     i != m_resultCRCs.end();
                     ++i
                ){
                    result.push_back( (*i)() );
                }
                return result;
            }

        public:

            BEGIN_PARAM_MAP( "" )
                BEGIN_PARAM_PATH( "/Session/Emulator/" )
                    PARAM_ENTRY( "@EnableResultCRC32Calculation", m_enableResultCRC )
                    BEGIN_PARAM_PATH("Processes/")
                        PARAM_ENTRY("count(Process)", m_processCount)
                    END_PARAM_PATH()
                END_PARAM_PATH()
                RESULT_ENTRY( "/Session/Result/System/@ResultCRC32", GetCRC_Result() )
            END_PARAM_MAP()
        };

        template <class Traits>
        CommonEmulator<Traits>::CommonEmulator( SystemIF* simSystem )
            :   m_opInfoArrayPool(sizeof(OpInfo*)),
                m_extraOpDecoder(0),
                m_enableResultCRC( false )
        {
            // param, �v���Z�X���ǂݍ���
            LoadParam();
            if (!CreateProcesses(simSystem)) {
                THROW_RUNTIME_ERROR("failed to create target processes.");
            }
            Operation::testroundmode(NULL);
        }

        template <class Traits>
        CommonEmulator<Traits>::~CommonEmulator()
        {
            for (std::vector<ProcessState*>::iterator e = m_processes.begin(); e != m_processes.end(); ++e) {
                delete *e;
            }

            ReleaseParam();
        }

        template <class Traits>
        ISAInfoIF* CommonEmulator<Traits>::GetISAInfo()
        {
            return &m_alpha64Info;
        }

        template <class Traits>
        bool CommonEmulator<Traits>::CreateProcesses( SystemIF* simSystem )
        {
            ASSERT(m_processes.empty(), "CommonEmulator<Traits>::CreateProcesses called twice.");

            m_ProcessOpInfoCache.resize( m_processCount );
            m_resultCRCs.resize( m_processCount );

            for (int i = 0; i < m_processCount; i++) {
                ProcessCreateParam createParam(i);
                ProcessState* processState = new ProcessState(i);
                processState->Init<Traits>( createParam, simSystem );
                m_processes.push_back(processState);
                m_ProcessOpInfoCache[i].resize( processState->GetCodeRange().second, OpInfoArray((OpInfo**)NULL, 0));
            }
            
            return true;
        }

        // �O�����߃f�R�[�_���Z�b�g����
        template <class Traits>
        void CommonEmulator<Traits>::SetExtraOpDecoder( ExtraOpDecoderIF* extraOpDecoder ) 
        {
            m_extraOpDecoder = extraOpDecoder;
        }

        template <class Traits>
        int CommonEmulator<Traits>::GetProcessCount() const
        {
            return m_processCount;
        }

        template <class Traits>
        PC CommonEmulator<Traits>::GetEntryPoint(int pid) const
        {
            ASSERT((size_t)pid < m_processes.size());
            return PC(pid, TID_INVALID, m_processes[pid]->GetEntryPoint() );
        }

        template <class Traits>
        u64 CommonEmulator<Traits>::GetInitialRegValue(int pid, int index) const
        {
            assert((size_t)pid < m_processes.size());
            return m_processes[pid]->GetInitialRegValue(index);
        }

        template <class Traits>
        std::pair<OpInfo**, int> CommonEmulator<Traits>::GetOpBody(PC pc)
        {
            ProcessState* process = m_processes[pc.pid];

            // �v���Z�X���Ƃ̃L���b�V�����܂�����
            const std::pair<u64, size_t> codeRange = process->GetCodeRange();
            // �Z���I�]���𖳌��ɂ���������炷���߂� && �ł͂Ȃ� & ���g�p
            bool withinCodeRange = (codeRange.first <= pc.address) & (pc.address < codeRange.first+codeRange.second);

            OpInfoArray* processOpInfoCacheEntry = NULL;
            if (withinCodeRange) {
                processOpInfoCacheEntry = &m_ProcessOpInfoCache[pc.pid][static_cast<size_t>(pc.address-codeRange.first)];
                if (processOpInfoCacheEntry->first != NULL)
                    return *processOpInfoCacheEntry;
            }

            // ���ߌ���擾
            EmuMemAccess codeAccess( pc.address, 4 );
            process->GetMemorySystem()->ReadMemory( &codeAccess );
            u32 codeWord = (u32)codeAccess.value;

            // ���ߌ���L�[�Ƃ����L���b�V��������
            typename CodeWordOpInfoMap::iterator e = m_codeWordToOpInfo.find(codeWord);
            if (e != m_codeWordToOpInfo.end()) {
                if (processOpInfoCacheEntry)
                    *processOpInfoCacheEntry = e->second;
                return e->second;
            }
            else {
                // ���ϊ��Ȃ�Εϊ����s��
                std::pair<ExtraOpInfoIF**, int> decodedExOps;
                std::vector<OpInfoType> tempOpInfo;
                OpInfo** opInfoArray;
                int nOps;

                if( m_extraOpDecoder &&
                    m_extraOpDecoder->Decode( codeWord, &decodedExOps ) 
                ){
                    // �O���f�R�[�_�Ŗ��߃f�R�[�h���s��ꂽ�ꍇ�C
                    // ExtraOpInfoWrapper �Ƀf�R�[�h���ʂ��i�[

                    // �ϊ����ʂ��i�[����|�C���^�̔z��� opInfoArrayPool �Ɋm�ۂ���
                    nOps = decodedExOps.second;
                    opInfoArray = (OpInfo**)m_opInfoArrayPool.ordered_malloc( nOps );
                    for( int i = 0; i < nOps; i++ ){
                        ExtraOpInfoWrapper<ISAInfoType>* wrapper = m_exOpInfoWrapperPool.construct();
                        wrapper->SetExtraOpInfo( decodedExOps.first[i] );
                        opInfoArray[i] = wrapper;
                    }
                }
                else{
                    m_insnConverter.Convert(codeWord, back_inserter(tempOpInfo));

                    // �ϊ����ʂ��i�[����|�C���^�̔z��� opInfoArrayPool �Ɋm�ۂ���
                    nOps = (int)tempOpInfo.size();
                    opInfoArray = (OpInfo**)m_opInfoArrayPool.ordered_malloc(nOps);
                    for (int i = 0; i < nOps; i ++) {
                        // opInfoPool �� �ϊ����ʂ� opInfo �̃R�s�[���쐬���CopInfoArray �Ɋi�[
                        opInfoArray[i] = m_opInfoPool.construct(tempOpInfo[i]);
                    }
                }


                // �L���b�V���Ɋi�[���Č��ʂ�Ԃ�
                OpInfoArray result(opInfoArray, nOps);
                m_codeWordToOpInfo[codeWord] = result;
                if (processOpInfoCacheEntry)
                    *processOpInfoCacheEntry = result;

                return result;
            }
        }

        template <class Traits>
        std::pair<OpInfo**, int> CommonEmulator<Traits>::GetOp(PC pc)
        {
            return GetOpBody(pc);
        }

        template <class Traits>
        MemIF* CommonEmulator<Traits>::GetMemImage()
        {
            return this;
        }

        template <class Traits>
        void CommonEmulator<Traits>::Execute(OpStateIF* op, OpInfo* opInfoBase)
        {
            OpInfoType* opInfo = static_cast<OpInfoType*>(opInfoBase);
            int pid = op->GetPC().pid;
            ProcessState* process = m_processes[pid];

            // ���߂����s����
            OpEmulationState opState(op, opInfo, process);  // op����I�y�����h����ǂ�� OpEmulationState ���\�z
            opInfo->GetEmulationFunc()(&opState);
            opState.ApplyEmulationState<OpInfoType>();  // op �ɒl��ݒ�
            process->GetVirtualSystem()->AddInsnTick();
        }

        template <class Traits>
        void CommonEmulator<Traits>::Commit( OpStateIF* op, OpInfo* info )
        {
            if( m_enableResultCRC ){
                u64 dst = info->GetDstNum() > 0 ? op->GetDst(0) : 0;
                CalcCRC( op->GetPC(), dst );
            }
        }

        template <class Traits>
        void CommonEmulator<Traits>::CalcCRC( const PC& pc, u64 dstResult )
        {
            m_resultCRCs[ pc.pid ].process_bytes( &pc.address , sizeof(pc.address) );
            m_resultCRCs[ pc.pid ].process_bytes( &dstResult , sizeof(dstResult) );
        }

        template <class Traits>
        PC CommonEmulator<Traits>::Skip(PC pc, u64 skipCount, u64* regArray, u64* executedInsnCount, u64* executedOpCount)
        {
            if (skipCount == 0) {
                if (executedInsnCount)
                    *executedInsnCount = 0;
                if (executedOpCount)
                    *executedOpCount = 0;
                return pc;
            }

            u64 totalOpCount = 0;
            u64 initialSkipCount = skipCount;
            ProcessState* process = m_processes[pc.pid];
            VirtualSystem* virtualSystem = process->GetVirtualSystem();
            bool enableResultCRC = m_enableResultCRC;

            SkipOp op(this);
            while (skipCount-- != 0 && pc.address != 0) {
                std::pair<OpInfo**, int> ops_pair = GetOpBody(pc);
                OpInfo** opInfoArray = ops_pair.first;
                int opCount = ops_pair.second;

                // opInfoArray �̖��߂�S�Ď��s
                for (int opIndex = 0; opIndex < opCount; opIndex ++) {
                    totalOpCount++;
                    OpInfoType* opInfo = static_cast<OpInfoType*>( opInfoArray[opIndex] );
                    
                    OpEmulationState opState(&op, opInfo, process, pc, pc.address + ISAInfoType::InstructionWordBitSize/8, regArray);   // op����I�y�����h����ǂ�� OpEmulationState ���\�z
                    opInfo->GetEmulationFunc()(&opState);
                    opState.ApplyEmulationStateToRegArray<OpInfoType>(regArray);

                    if( enableResultCRC ){
                        u64 dst = opInfo->GetDstNum() > 0 ? opState.GetDst(0) : 0;
                        CalcCRC( pc, dst );
                    }
                    virtualSystem->AddInsnTick();
    
                    if (opIndex == opCount-1) {
                        if (opState.GetTaken())
                            pc.address = opState.GetTakenPC();
                        else
                            pc.address += ISAInfoType::InstructionWordBitSize/8;
                        break;
                    }
                }
            }

            if (executedInsnCount)
                *executedInsnCount = initialSkipCount - skipCount;
            if (executedOpCount)
                *executedOpCount = totalOpCount;
            return pc;
        }

        template <class Traits>
        void CommonEmulator<Traits>::Read( MemAccess* access )
        {
            int pid = access->address.pid;
            ProcessState* process = m_processes[pid];
            process->GetMemorySystem()->ReadMemory(access);
        }

        template <class Traits>
        void CommonEmulator<Traits>::Write( MemAccess* access )
        {
            int pid = access->address.pid;
            ProcessState* process = m_processes[pid];
            process->GetMemorySystem()->WriteMemory(access);
        }

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif

