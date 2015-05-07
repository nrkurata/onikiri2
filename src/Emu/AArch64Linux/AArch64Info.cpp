#include <pch.h>
#include "AArch64Info.h"
#include "AArch64Converter.h"

using namespace std;
using namespace Onikiri;
using namespace AArch64Linux;

ISA_TYPE AArch64Info::GetISAType()
{
	return ISA_AARCH64;
}

int AArch64Info::GetRegisterSegmentID(int regNum)
{
	const int segmentRange[] = {
		 0, 	// Int
		16, 	// FP
		48, 	// CPSR
		49, 	// TMP
		50		// Н≈Се+1
	};
	const int nElems = sizeof(segmentRange)/sizeof(segmentRange[0]);

	ASSERT(0 <= regNum && regNum < RegisterCount, "regNum out of bound");
	ASSERT(segmentRange[nElems-1] == RegisterCount);

	return static_cast<int>( upper_bound(segmentRange, segmentRange+nElems, regNum) - segmentRange ) - 1;
}

int AArch64Info::GetRegisterSegmentCount()
{
	return 6;
}

int AArch64Info::GetInstructionWordBitSize()
{
	return InstructionWordBitSize;
}

int AArch64Info::GetRegisterWordBitSize()
{
	return 64;
}

int AArch64Info::GetRegisterCount()
{
	return RegisterCount;
}

int AArch64Info::GetAddressSpaceBitSize()
{
	return 64;
}

int AArch64Info::GetMaxSrcRegCount()
{
	return MaxSrcRegCount;
}

int AArch64Info::GetMaxDstRegCount()
{
	return MaxDstRegCount;
}

int AArch64Info::GetMaxOpInfoCountPerPC()
{
	return AArch64ConverterTraits::MaxOpInfoDefs;
}

bool AArch64Info::IsLittleEndian()
{
	return true;
}

int AArch64Info::GetMaxMemoryAccessByteSize()
{
	return MAX_MEMORY_ACCESS_WIDTH;
}
