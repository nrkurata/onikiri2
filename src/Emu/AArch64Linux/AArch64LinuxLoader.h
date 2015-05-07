#ifndef __ARMLINUXLOADER_H__
#define __ARMLINUXLOADER_H__

#include "../Utility/System/Loader/Linux64Loader.h"


namespace Onikiri {
	namespace AArch64Linux {
		// ARMLinux ELF 用のローダー
		class AArch64LinuxLoader : public EmulatorUtility::Linux64Loader
		{
		public:
			AArch64LinuxLoader();
			virtual ~AArch64LinuxLoader();

			// LoaderIF の実装
			virtual u64 GetInitialRegValue(int index) const;
		private:
		};

	} // namespace ARMLinux
} // namespace Onikiri

#endif
