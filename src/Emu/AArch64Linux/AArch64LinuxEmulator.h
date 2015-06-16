#ifndef __AARCH64LINUX_EMULATOR_H__
#define __AARCH64LINUX_EMULATOR_H__

#include "Emu/Utility/CommonEmulator.h"
#include "Emu/AArch64Linux/AArch64LinuxTraits.h"

namespace Onikiri {
	namespace AArch64Linux {

		typedef EmulatorUtility::CommonEmulator<AArch64LinuxTraits> AArch64LinuxEmulator;

	} // namespace ARMLinux
} // namespace Onikiri

#endif
