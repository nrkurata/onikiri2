#ifndef __ARMLINUX_ARMCONVERTER_H__
#define __ARMLINUX_ARMCONVERTER_H__

#include "AArch64Info.h"
#include "AArch64Decoder.h"
#include "AArch64OpInfo.h"
#include "../Utility/CommonConverter.h"

namespace Onikiri {

	namespace AArch64Linux {

		// ARM�̖��߂��COpInfo �̗�ɕϊ�����
		struct AArch64ConverterTraits {
			typedef AArch64OpInfo OpInfoType;
			typedef AArch64Decoder DecoderType;

			typedef u32 CodeWordType;
			static const int MaxOpInfoDefs = 17;
			static const int MaxDstOperands = AArch64OpInfo::MaxDstRegCount;
			static const int MaxSrcOperands = AArch64OpInfo::MaxSrcCount;	// SrcReg �� SrcImm �̍��v
		};

		// Alpha�̖��߂��COpInfo �̗�ɕϊ�����
		class AArch64Converter : public EmulatorUtility::CommonConverter<AArch64ConverterTraits>
		{
		public:
			AArch64Converter();
			virtual ~AArch64Converter();

		private:
			// CommonConverter �̃J�X�^�}�C�Y
			virtual bool IsZeroReg(int reg) const;
			virtual std::pair<OperandType, int> GetActualSrcOperand(int srcTemplate, const DecodedInsn& decoded) const;
			virtual int GetActualRegNumber(int regTemplate, const DecodedInsn& decoded) const;
			virtual const OpDef* GetOpDefUnknown() const;
			virtual void CreateOpDefs(OpDef* partialBase, int baseSize, OpDef* partialExtra, int extraSize);
			virtual OpDef* FinalizeOpDefs();

			// OpDef
			static OpDef m_OpDefsBase[];
			static OpDef m_OpDefsLoadStore[];
			static OpDef m_OpDefsSIMD[];
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
