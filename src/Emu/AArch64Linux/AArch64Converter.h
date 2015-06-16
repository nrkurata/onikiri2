#ifndef __AARCH64LINUX_AARCH64CONVERTER_H__
#define __AARCH64LINUX_AARCH64CONVERTER_H__

#include "Emu/AArch64Linux/AArch64Info.h"
#include "Emu/AArch64Linux/AArch64Decoder.h"
#include "Emu/AArch64Linux/AArch64OpInfo.h"
#include "Emu/Utility/CommonConverter.h"

namespace Onikiri {

	namespace AArch64Linux {

		// ARMの命令を，OpInfo の列に変換する
		struct AArch64ConverterTraits {
			typedef AArch64OpInfo OpInfoType;
			typedef AArch64Decoder DecoderType;

			typedef u32 CodeWordType;
			static const int MaxOpInfoDefs = 17;
			static const int MaxDstOperands = AArch64OpInfo::MaxDstRegCount;
			static const int MaxSrcOperands = AArch64OpInfo::MaxSrcCount;	// SrcReg と SrcImm の合計
		};

		// Alphaの命令を，OpInfo の列に変換する
		class AArch64Converter : public EmulatorUtility::CommonConverter<AArch64ConverterTraits>
		{
		public:
			AArch64Converter();
			virtual ~AArch64Converter();

		private:
			// CommonConverter のカスタマイズ
			virtual bool IsZeroReg(int reg) const;
			virtual std::pair<OperandType, int> GetActualSrcOperand(int srcTemplate, const DecodedInsn& decoded) const;
			virtual int GetActualRegNumber(int regTemplate, const DecodedInsn& decoded) const;
			virtual const OpDef* GetOpDefUnknown() const;
			virtual void CreateOpDefs(OpDef* partialBase, int baseSize, OpDef* partialExtra, int extraSize);
			virtual OpDef* FinalizeOpDefs();

			// OpDef
			static OpDef m_OpDefsBranch[];
			static OpDef m_OpDefsLoadStore[];
			static OpDef m_OpDefsDataProcess[];
			static OpDef m_OpDefsSIMD[];
			static OpDef m_OpDefsFP[];
			static OpDef m_OpDefUnknown;

			std::vector<OpDef> m_OpDefVector;
			std::vector<std::string> m_OpDefName;
			OpDef* m_CreatedOpDefs;
			int m_CreatedOpDefsSize;

			static void AArch64UnknownOperation(EmulatorUtility::OpEmulationState* opState);
		};
	} // namespace ARMLinux
} // namespace Onikiri

#endif
