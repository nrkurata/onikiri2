// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Ryota Shioya.
// Copyright (c) 2005-2015 Masahiro Goshima.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
// 
// 


#ifndef __EMULATORUTILITY_VIRTUALSYSTEM_H__
#define __EMULATORUTILITY_VIRTUALSYSTEM_H__

#include "SysDeps/posix.h"

namespace Onikiri {
    namespace EmulatorUtility {
        typedef POSIX::posix_struct_stat HostStat;

        // target��host�Ԃ�FD�ϊ����s���N���X
        class FDConv
        {
        public:
            static const int InvalidFD = -1;

            FDConv();
            ~FDConv();

            // target��FD��host��FD�̕ϊ����s��
            int TargetToHost(int targetFD) const;
            int HostToTarget(int hostFD) const;

            // FD�̑Ή���ǉ�
            bool AddMap(int targetFD, int hostFD);

            // FD�̑Ή����폜
            bool RemoveMap(int targetFD);

            // �󂢂Ă��� target ��FD��Ԃ�
            int GetFirstFreeFD();
        private:
            void ExtendFDMap();
            void ExtendFDMap(size_t size);

            // target��fd��host��fd�ɕϊ�����\�Dhost��fd�����蓖�Ă��Ă��Ȃ����InvalidFD�������Ă���
            std::vector<int> m_FDTargetToHostTable;
        };
        
        // �v���Z�X�� open ���̃t�@�C���� unlink �����Ƃ��̋�����
        // Unix �ɍ��킹�邽�߂̃N���X
        // �ڍׂ� VirtualSystem::Unlink �ɋL�q
        class DelayUnlinker
        {
        public:
            DelayUnlinker();
            ~DelayUnlinker();

            // TargetFD<>path �̑Ή���ǉ�
            bool AddMap(int targetFD, std::string path);
            // TargetFD<>path �̑Ή����폜
            bool RemoveMap(int targetFD);
            // TargetFD<>path �̑Ή�����p�X���擾
            std::string GetMapPath(int targetFD);
            // �폜���� path ��ǉ�
            bool AddUnlinkPath(std::string path);
            // �폜���� path �� Unlink ���邩�ǂ���
            bool IfUnlinkable(int targetFD);
            // �폜���� path ���폜
            bool RemoveUnlinkPath(std::string path);
        private:
            std::map<int, std::string> m_targetFDToPathTable;
            std::list<std::string> m_delayUnlinkPathList;
        };

        // target�̂��߂̉��z�V�X�e���i�t�@�C�����j���
        //  - target�̃v���Z�X����fd�ƃz�X�g��fd�̕ϊ����s��
        //  - target����working directory������
        //  - �����̎擾
        class VirtualSystem : public ParamExchange
        {
        public:

            VirtualSystem();
            ~VirtualSystem();

            // VirtualSystem��Open���Ă��Ȃ��t�@�C���𖾎��I��FD�̕ϊ��\�ɒǉ�����(stdin, stdout, stderr��)
            bool AddFDMap(int targetFD, int hostFD, bool autoclose = false);
            // �^�[�Q�b�g�̍�ƃf�B���N�g����ݒ肷��
            void SetInitialWorkingDir(const boost::filesystem::path& dir);

            // �V�X�e���R�[���Q
            int GetErrno();

            int GetPID();
            int GetUID();
            int GetEUID();
            int GetGID();
            int GetEGID();

            char* GetCWD(char* buf, int maxlen);
            int ChDir(const char* path);

            // �t�@�C�����J���D�J�����t�@�C���͎�����FD�̕ϊ��\�ɒǉ������
            int Open(const char* filename,int oflag);

            int Read(int targetFD, void *buffer, unsigned int count);
            int Write(int targetFD, void* buffer, unsigned int count);
            int Close(int targetFD);

            int Stat(const char* path, HostStat* s);
            int FStat(int fd, HostStat* s);
            int LStat(const char* path, HostStat* s);

            s64 LSeek(int fd, s64 offset, int whence);
            int Dup(int fd);

            int Access(const char* path, int mode);
            int Unlink(const char* path);
            int Rename(const char* oldpath, const char* newpath);

            int Truncate(const char* path, s64 length);
            int FTruncate(int fd, s64 length);

            int MkDir(const char* path, int mode);

            // target��FD��host��FD�̕ϊ����s��
            int FDTargetToHost(int targetFD) const
            {
                return m_fdConv.TargetToHost(targetFD);
            }
            int FDHostToTarget(int hostFD) const
            {
                return m_fdConv.HostToTarget(hostFD);
            }

            // �����̎擾
            s64 GetTime();
            s64 GetClock();
            void AddInsnTick()
            {
                m_executedInsnTick++;
            }
            s64 GetInsnTick()
            {
                return m_executedInsnTick;
            }

            DelayUnlinker* GetDelayUnlinker(){
                return &m_delayUnlinker;
            };

            BEGIN_PARAM_MAP("/Session/Emulator/System/Time/")
                PARAM_ENTRY( "@UnixTime",    m_unixTime )
                PARAM_ENTRY( "@EmulationMode", m_timeEmulationModeStr )
            END_PARAM_MAP()
        private:
            boost::filesystem::path GetHostPath(const char* targetPath);
            void AddAutoCloseFD(int fd);
            void RemoveAutoCloseFD(int fd);

            FDConv m_fdConv;
            // �f�X�g���N�g���Ɏ�����close����fd
            std::vector<int> m_autoCloseFD;
            DelayUnlinker m_delayUnlinker;

            boost::filesystem::path m_cwd;

            // ����
            int EmulationModeStrToInt( const std::string& );
            u64 m_unixTime;
            u64 m_executedInsnTick;
            std::string m_timeEmulationModeStr;
            int m_timeEmulationMode;
            enum {
                TIME_HOST,
                TIME_FIXED,
                TIME_INSTRUCTION,
            };
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
