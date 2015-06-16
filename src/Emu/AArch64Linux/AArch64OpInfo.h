#ifndef __AARCH64_OPINFO_H__
#define __AARCH64_OPINFO_H__

#include "Emu/Utility/CommonOpInfo.h"
#include "Emu/AArch64Linux/AArch64Info.h"

namespace Onikiri {
	namespace AArch64Linux {

		typedef EmulatorUtility::CommonOpInfo<AArch64Info> AArch64OpInfo;

	}
}

#endif
