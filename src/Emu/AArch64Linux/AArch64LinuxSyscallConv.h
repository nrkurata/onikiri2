#ifndef __AARCH64LINUX_SYSCALLCONV_H__
#define __AARCH64LINUX_SYSCALLCONV_H__

#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/VirtualSystem.h"
#include "Emu/Utility/System/Syscall/Linux64SyscallConv.h"

namespace Onikiri {

	namespace EmulatorUtility {
		class ProcessState;
		class VirtualMemory;
		class OpEmulationState;
	}

	namespace AArch64Linux {

		class AArch64LinuxSyscallConv : public EmulatorUtility::Linux64SyscallConv
		{
		private:
			AArch64LinuxSyscallConv();
		public:
			AArch64LinuxSyscallConv(EmulatorUtility::ProcessState* processState);
			virtual ~AArch64LinuxSyscallConv();

			// SetArg によって与えられた引数に従ってシステムコールを行う
			virtual void Execute(EmulatorUtility::OpEmulationState* opState);
		protected:

			// arch dependent

			// conversion
			//virtual void write_stat64(u64 dest, const EmulatorUtility::HostStat &src);
			virtual int Get_MAP_ANONYMOUS();
			virtual int Get_MREMAP_MAYMOVE();
			virtual int Get_CLK_TCK();

			virtual u32 OpenFlagTargetToHost(u32 flag);
		};

	} // namespace ARMLinux
} // namespace Onikiri

#endif
