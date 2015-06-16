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


#ifndef EMU_UTILITY_COMMON_CONVERTER_H
#define EMU_UTILITY_COMMON_CONVERTER_H

#include "Interface/OpClassCode.h"

namespace Onikiri {

    namespace EmulatorUtility {

        // �Œ蒷�̖��߂� OpInfo �̗�ɕϊ�����
        template <class Traits>
        class CommonConverter : public ParamExchange
        {
        public:
            // Traits �͈ȉ��� typedef �ƒ萔���`���� struct
            typedef typename Traits::OpInfoType OpInfoType;
            typedef typename Traits::DecoderType DecoderType;
            typedef typename Traits::CodeWordType CodeWordType;

            static const int MaxOpInfoInCode = Traits::MaxOpInfoDefs;       // 1�̖��߂�ϊ������Ƃ��ɐ��������OpInfo�̍ő吔

            CommonConverter();
            virtual ~CommonConverter();

            // ���ߌ� codeWord ��OpInfoType�ɕϊ����āCinserter �̎����R���e�i�Ɋi�[����
            // ex.) Iter = back_insert_iterator<OpInfoType>
            template<typename Iter>
            void Convert(CodeWordType codeWord, Iter out) const;

            // ParamExchange ���̊֐���錾
            typedef ParamExchange ParamExchangeType;
            using ParamExchangeType::LoadParam;
            using ParamExchangeType::ReleaseParam;
            using ParamExchangeType::GetRootPath;
            using ParamExchangeType::ParamEntry;

            BEGIN_PARAM_MAP("/Session/Emulator/")
                BEGIN_PARAM_PATH("Converter/")
                    PARAM_ENTRY("@EnableSplitLoadStore", m_enableSplitLoadStore)
                END_PARAM_PATH()
            END_PARAM_MAP()

        protected:

            static const int MaxOpInfoDefs = Traits::MaxOpInfoDefs;     // OpDef ���� OpInfoDef �z��̃T�C�Y
            static const int MaxDstOperands = Traits::MaxDstOperands;   // �f�X�e�B�l�[�V�����̍ő吔
            static const int MaxSrcOperands = Traits::MaxSrcOperands;   // SrcReg �� SrcImm �̍��v�̍ő�

            // �֗��Ɏg�����߂�typedef
            typedef typename DecoderType::DecodedInsn DecodedInsn;
            typedef typename OpInfoType::OperandType OperandType;

            struct OpInfoDef
            {
                OpClassCode::OpClassCode Iclass;

                // �I�y�����h�e���v���[�g
                int DstTemplate[MaxDstOperands];
                int SrcTemplate[MaxSrcOperands];

                // ���̖��߂���������֐�
                typename OpInfoType::EmulationFunc Func;
            };

            struct OpDef {
                const char* Name;           // ���߂̖��O (�R�����g�ɂ̂ݎg�p)
                u32         Mask;           // �}�X�N
                u32         Opcode;         // ���� & mask == opcode �̖��߂ɑ΂��Ă���OpDef���}�b�`����
                int         nOpInfoDefs;    // �}�C�N�����ߐ�
                OpInfoDef   OpInfoDefs[MaxOpInfoDefs];  // �}�C�N�����ߒ�`
            };


            bool IsSplitLoadStoreEnabled() const;
            void AddToOpMap(OpDef* opDefs, int count);
            const OpDef* FindOpDef(CodeWordType codeWord) const;
            OpInfoType ConvertOneOpInfo(
                const DecodedInsn& decoded,
                const OpInfoDef& opInfoDef, 
                int microOpNum,
                int microOpIndex,
                const char* mnemonic
            ) const;

            typedef std::map<CodeWordType, const OpDef *> OpcodeMap;
            // Note: this must be an 'ordered' map
            typedef std::map<CodeWordType, OpcodeMap, std::greater<CodeWordType> > MaskMap;

            // �J�X�^�}�C�Y�|�C���g

            // ���W�X�^�ԍ�reg���[�����W�X�^���ǂ����𔻒肷��
            virtual bool IsZeroReg(int reg) const = 0;

            // srcTemplate �ɑΉ�����I�y�����h�̎�ނƁC���W�X�^�Ȃ�Δԍ����C���l�Ȃ��index��Ԃ�
            virtual std::pair<OperandType, int> GetActualSrcOperand(int srcTemplate, const DecodedInsn& decoded) const = 0;

            // regTemplate ������ۂ̃��W�X�^�ԍ����擾����
            virtual int GetActualRegNumber(int regTemplate, const DecodedInsn& decoded) const = 0;

            // ����`���߂ɑΉ�����OpDef�𓾂�
            virtual const OpDef* GetOpDefUnknown() const = 0;

        private:
            MaskMap m_opMap;
            bool m_enableSplitLoadStore;

        };

        template<class Traits>
        CommonConverter<Traits>::CommonConverter()
        {
            LoadParam();
        }

        template<class Traits>
        CommonConverter<Traits>::~CommonConverter()
        {
            ReleaseParam();
        }

        template<class Traits>
        template<typename Iter>
        void CommonConverter<Traits>::Convert(CodeWordType codeWord, Iter out) const
        {
            DecoderType decoder;
            DecodedInsn decoded;
            decoder.Decode(codeWord, &decoded);
            const OpDef* opdef = FindOpDef(codeWord);

            for (int i = 0; i < opdef->nOpInfoDefs; i ++) {
                const OpInfoDef &opInfoDef = opdef->OpInfoDefs[i];
                *out++ = 
                    ConvertOneOpInfo(
                        decoded,
                        opInfoDef, 
                        opdef->nOpInfoDefs,
                        i, 
                        opdef->Name
                    );
            }
        }

        template <class Traits>
        bool CommonConverter<Traits>::IsSplitLoadStoreEnabled() const
        {
            return m_enableSplitLoadStore;
        }

        template <class Traits>
        void CommonConverter<Traits>::AddToOpMap(OpDef* opDefs, int count)
        {
            for (int i = 0; i < count; i ++) {
                const OpDef &opdef = opDefs[i];

                ASSERT(m_opMap[opdef.Mask].find(opdef.Opcode) == m_opMap[opdef.Mask].end(), "opcode conflict detected");
                m_opMap[opdef.Mask][opdef.Opcode] = &opdef;
            }
        }

        // codeWord �ɑΉ����� opDef ��T��
        template <class Traits>
        const typename CommonConverter<Traits>::OpDef* CommonConverter<Traits>::FindOpDef(CodeWordType codeWord) const
        {
            // Mask �̌��������̂��珇�ɒT��
            for (typename MaskMap::const_iterator e = m_opMap.begin(); e != m_opMap.end(); ++e) {
                u32 mask = e->first;
                typename OpcodeMap::const_iterator i = e->second.find(codeWord & mask);
                if (i != e->second.end())
                    return i->second;
            }

            return GetOpDefUnknown();
        }

        // opInfoDef �ɏ]���Ė��߂�OpInfo�ɕϊ�����
        template <class Traits>
        typename CommonConverter<Traits>::OpInfoType 
            CommonConverter<Traits>::ConvertOneOpInfo(
                const DecodedInsn& decoded, 
                const OpInfoDef& opInfoDef, 
                int microNum,
                int microOpIndex,
                const char* mnemonic
            ) const
        {
            OpInfoType opInfo(OpClass(opInfoDef.Iclass));

            ASSERT(opInfoDef.Func != 0, "opInfoDef.Func is null");  // ����Ɉ���������Ƃ��� nOpInfoDefs ���s��
            opInfo.SetEmulationFunc(opInfoDef.Func);
            opInfo.SetMicroOpNum( microNum );
            opInfo.SetMicroOpIndex( microOpIndex );
            opInfo.SetMnemonic(mnemonic);

            // ����`���߂̏ꍇ�C���ߌ�𑦒l�Ƃ��ē����
            if (opInfoDef.Iclass == OpClassCode::UNDEF) {
                opInfo.SetDstOpNum(0);
                opInfo.SetSrcOpNum(1);
                opInfo.SetImmNum(1);

                opInfo.SetSrcRegNum(0);
                opInfo.SetDstRegNum(0);

                opInfo.SetImmOpMap(0, 0);
                opInfo.SetImm(0, static_cast<s32>(decoded.CodeWord));
                return opInfo;
            }

            // xxxMap���̈Ӗ��ɂ��Ă� CommonOpInfo.h ���Q��

            // Dst �I�y�����h���W�X�^�̐ݒ�
            // Dst�e���v���[�g����decoded���̑Ή����郌�W�X�^�ԍ����擾���Đݒ�
            int dstRegNum = 0;
            int dstOpNum = 0;
            for (int i = 0; i < MaxDstOperands; i ++) {
                int dstTemplate = opInfoDef.DstTemplate[i];
                if (dstTemplate == -1)
                    break;

                int dstReg = GetActualRegNumber( dstTemplate, decoded );
                ASSERT(dstReg != -1);

                if (!IsZeroReg(dstReg)) {
                    opInfo.SetDstRegOpMap(dstRegNum, i);

                    opInfo.SetDstReg(dstRegNum, dstReg);
                    dstRegNum ++;
                }
                dstOpNum ++;
            }
            opInfo.SetDstOpNum(dstOpNum);
            opInfo.SetDstRegNum(dstRegNum);

            // Src �I�y�����h (���W�X�^�E���l) �̐ݒ�
            int srcRegNum = 0;
            int srcOpNum = 0;
            int immNum = 0;
            for (int i = 0; i < MaxSrcOperands; i ++) {
                int srcTemplate = opInfoDef.SrcTemplate[i];
                if (srcTemplate == -1)
                    break;
                
                std::pair<OperandType, int> operand = GetActualSrcOperand( srcTemplate, decoded );
                switch (operand.first) {
                case OpInfoType::REG:
                    {
                        int srcReg = operand.second;
                        ASSERT(srcReg != -1);

                        if (IsZeroReg(srcReg)) {
                            // ���l�Ƃ��� 0 �����Ă���
                            opInfo.SetImmOpMap(immNum, i);

                            opInfo.SetImm(immNum, 0);
                            immNum ++;
                        }
                        else {
                            opInfo.SetSrcRegOpMap(srcRegNum, i);

                            opInfo.SetSrcReg(srcRegNum, srcReg);
                            srcRegNum ++;
                        }
                    }
                    break;
                case OpInfoType::IMM:
                    {
                        int immIndex = operand.second;

                        opInfo.SetImmOpMap(immNum, i);

                        opInfo.SetImm(immNum, decoded.Imm[immIndex]);
                        immNum ++;
                    }
                    break;
                default:
                    ASSERT(0, "Alpha64Converter Logic Error : invalid SrcType");
                }

                srcOpNum ++;
            }
            opInfo.SetSrcOpNum(srcOpNum);
            opInfo.SetSrcRegNum(srcRegNum);
            opInfo.SetImmNum(immNum);

            return opInfo;
        }

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
