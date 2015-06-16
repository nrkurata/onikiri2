#include <pch.h>
#include "Emu/AArch64Linux/AArch64LinuxLoader.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::AArch64Linux;

namespace {
	const u16 MACHINE_ARM = 0x0028;
}

AArch64LinuxLoader::AArch64LinuxLoader()
	: Linux64Loader(MACHINE_ARM)		// machine = ARM
{
}

AArch64LinuxLoader::~AArch64LinuxLoader()
{
}

u64 AArch64LinuxLoader::GetInitialRegValue(int index) const
{
	const int STACK_POINTER_REGNUM = 13;

	if (index == STACK_POINTER_REGNUM)
		return GetInitialSp();
	else
		return 0;
}
