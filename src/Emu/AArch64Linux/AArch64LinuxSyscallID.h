#ifndef __AARCH64LINUX_SYSCALLID_H__
#define __AARCH64LINUX_SYSCALLID_H__

namespace Onikiri {
	namespace AArch64Linux {

		namespace SyscallID {
			const int syscall_id_exit = 1;
			const int syscall_id_fork = 2;
			const int syscall_id_read = 3;
			const int syscall_id_write = 4;
			const int syscall_id_open = 5;
			const int syscall_id_close = 6;
			
			const int syscall_id_creat = 8;
			const int syscall_id_link = 9;
			const int syscall_id_unlink = 10;
			const int syscall_id_execve = 11;
			const int syscall_id_chdir = 12;
			const int syscall_id_time = 13;
			const int syscall_id_mknod = 14;
			const int syscall_id_chmod = 15;
			const int syscall_id_lchown = 16;

			const int syscall_id_lseek = 19;
			const int syscall_id_getpid = 20;
			const int syscall_id_mprotect = 21;
			const int syscall_id_oldumount = 22;
			const int syscall_id_setuid = 23;
			const int syscall_id_getuid = 24;

			
			const int syscall_id_utime = 30;
			const int syscall_id_access = 33;
			const int syscall_id_sync = 36;
			const int syscall_id_kill = 37;
			const int syscall_id_rename = 38;
			const int syscall_id_mkdir = 39;
			const int syscall_id_rmdir = 40;
			const int syscall_id_dup = 41;
			const int syscall_id_times = 43;
			const int syscall_id_brk = 45;
			const int syscall_id_setgid = 46;
			const int syscall_id_getgid = 47;
			const int syscall_id_geteuid = 49;
			const int syscall_id_getegid = 50;
			const int syscall_id_ioctl = 54;
			const int syscall_id_fcntl = 55;
			const int syscall_id_setpgid = 57;
			const int syscall_id_umask = 60;
			const int syscall_id_dup2 = 63;
			const int syscall_id_setsid = 66;
			const int syscall_id_sgetmask = 67;
			const int syscall_id_setreuid = 70;
			const int syscall_id_setregid = 71;
//			const int syscall_id_setregid = 72;
			const int syscall_id_munmap = 73;
//			const int syscall_id_mprotect = 74;
			const int syscall_id_setrlimit = 75;
			const int syscall_id_getrlimit = 76;
			const int syscall_id_getrusage = 77;
			const int syscall_id_gettimeofday = 78;
			const int syscall_id_getgroups = 79;
			const int syscall_id_setgroups = 80;
			const int syscall_id_readlink = 85;
			const int syscall_id_mmap = 90;
//			const int syscall_id_munmap = 91;
			const int syscall_id_truncate = 92;
			const int syscall_id_ftruncate = 93;
			const int syscall_id_fchmod = 94;
			const int syscall_id_fchown = 95;
			const int syscall_id_stat = 106;
			const int syscall_id_lstat = 107;
			const int syscall_id_fstat = 108;
			const int syscall_id_uname = 122;
//			const int syscall_id_mprotect = 125;
			const int syscall_id_sigprocmask = 126;


			/*

			const int syscall_id_fstat = 91;
			const int syscall_id_fcntl = 92;
			const int syscall_id_osf_select = 93;
			const int syscall_id_osf_gettimeofday = 116;
			const int syscall_id_osf_getrusage = 117;
			*/
			const int syscall_id_readv = 120;
			const int syscall_id_writev = 121;
			/*
			const int syscall_id_osf_settimeofday = 122;

			const int syscall_id_fchown = 123;
			const int syscall_id_fchmod = 124;
			const int syscall_id_setreuid = 126;
			const int syscall_id_setregid = 127;

			const int syscall_id_rename = 128;
			const int syscall_id_truncate = 129;
			const int syscall_id_ftruncate = 130;
			const int syscall_id_flock = 131;

			const int syscall_id_mkdir = 136;
			const int syscall_id_rmdir = 137;

			const int syscall_id_getrlimit = 144;
			const int syscall_id_setrlimit = 145;

			const int syscall_id_osf_pid_block = 153;
			const int syscall_id_osf_pid_unblock = 154;
			const int syscall_id_sigaction = 156;
			const int syscall_id_getpgid = 233;
			const int syscall_id_getsid = 234;

			const int syscall_id_osf_getsysinfo = 256;
			const int syscall_id_osf_setsysinfo = 257;

			const int syscall_id_times = 323;

			const int syscall_id_uname = 339;
			const int syscall_id_nanosleep = 340;
			*/
			const int syscall_id_mremap = 341;
			/*
			const int syscall_id_setresuid = 343;
			const int syscall_id_getresuid = 344;
			*/
			const int syscall_id_rt_sigaction = 352;
			const int syscall_id_rt_sigprocmask = 353;
			/*
			const int syscall_id_select = 358;
			const int syscall_id_gettimeofday = 359;
			const int syscall_id_settimeofday = 360;

			const int syscall_id_utimes = 363;
			const int syscall_id_getrusage = 364;
			
			*/
			const int syscall_id_getcwd = 367;
			
			const int syscall_id_gettid = 378;
			const int syscall_id_exit_group = 405;
			
			const int syscall_id_tgkill = 424;

			const int syscall_id_stat64 = 425;
			const int syscall_id_lstat64 = 426;
			const int syscall_id_fstat64 = 427;
			

		} // namespace SyscallID

		using namespace SyscallID;

	}	// namespace ARMLinux
} // namespace Onikiri

#endif
