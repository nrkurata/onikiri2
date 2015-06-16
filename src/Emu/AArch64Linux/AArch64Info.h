#ifndef __AARCH64LINUX_AARCH64INFO_H__
#define __AARCH64LINUX_AARCH64INFO_H__

#include "Interface/ISAInfo.h"

namespace Onikiri {
	namespace AArch64Linux {
		// AArch64 ÇÃ ISA èÓïÒ
		class AArch64Info : public ISAInfoIF
		{
		public:
			static const int InstructionWordBitSize = 32;
			static const int MaxSrcRegCount = 5;
			static const int MaxDstRegCount = 2;
			static const int MaxImmCount = 3;
			// Int : 32
			// FP : 32
			// CPSR : 1	(Current Program Status Register)
			// TMPR : 1 (Temp. Register)
			static const int RegisterCount = 32 + 32 + 1 + 1;

			static const int REG_CPST	= 48;
			static const int REG_TMP	= 49;

			static const int MAX_MEMORY_ACCESS_WIDTH = 8;

			virtual ISA_TYPE GetISAType();
			virtual int GetInstructionWordBitSize();
			virtual int GetRegisterWordBitSize();
			virtual int GetRegisterCount();
			virtual int GetAddressSpaceBitSize();
			virtual int GetMaxSrcRegCount();
			virtual int GetMaxDstRegCount();
			virtual int GetRegisterSegmentID(int regNum);
			virtual int GetRegisterSegmentCount();
			virtual int GetMaxOpInfoCountPerPC();
			virtual int GetMaxMemoryAccessByteSize();
			virtual bool IsLittleEndian();
		};
	}
}
#endif
