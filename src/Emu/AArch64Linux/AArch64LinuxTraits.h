#ifndef __ARMLINUX_TRAITS_H__
#define __ARMLINUX_TRAITS_H__

#include "ARMLinuxSyscallConv.h"
#include "ARMLinuxLoader.h"
#include "ARMInfo.h"
#include "ARMConverter.h"
#include "ARMOpInfo.h"

namespace Onikiri {
	namespace AArch64Linux {

		struct ARMLinuxTraits {
			typedef AArch64Info ISAInfoType;
			typedef AArch64OpInfo OpInfoType;
			typedef AArch64Converter ConverterType;
			typedef AArch64LinuxLoader LoaderType;
			typedef AArch64LinuxSyscallConv SyscallConvType;

			static const bool IsBigEndian = false;
		};

	} // namespace ARMLinux
} // namespace Onikiri

#endif
