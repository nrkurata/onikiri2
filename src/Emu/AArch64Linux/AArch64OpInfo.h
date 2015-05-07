#ifndef __ARM_OPINFO_H__
#define __ARM_OPINFO_H__

#include "../Utility/CommonOpInfo.h"
#include "AArch64Info.h"

namespace Onikiri {
	namespace AArch64Linux {

		typedef EmulatorUtility::CommonOpInfo<AArch64Info> AArch64OpInfo;

	}
}

#endif
