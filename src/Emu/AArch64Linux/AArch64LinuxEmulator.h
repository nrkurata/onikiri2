#ifndef __ARMLINUX_EMULATOR_H__
#define __ARMLINUX_EMULATOR_H__

#include "../Utility/CommonEmulator.h"
#include "ARMLinuxTraits.h"

namespace Onikiri {
	namespace AArch64Linux {

		typedef EmulatorUtility::CommonEmulator<ARMLinuxTraits> ARMLinuxEmulator;

	} // namespace ARMLinux
} // namespace Onikiri

#endif
