#ifndef __AARCH64LINUX_TRAITS_H__
#define __AARCH64LINUX_TRAITS_H__

#include "Emu/AArch64Linux/AArch64LinuxSyscallConv.h"
#include "Emu/AArch64Linux/AArch64LinuxLoader.h"
#include "Emu/AArch64Linux/AArch64Info.h"
#include "Emu/AArch64Linux/AArch64Converter.h"
#include "Emu/AArch64Linux/AArch64OpInfo.h"

namespace Onikiri {
	namespace AArch64Linux {

		struct AArch64LinuxTraits {
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
