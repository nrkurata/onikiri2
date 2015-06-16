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


#ifndef __EMULATORUTILITY_MEMORY_SYSTEM_H__
#define __EMULATORUTILITY_MEMORY_SYSTEM_H__

#include "Emu/Utility/System/Memory/HeapAllocator.h"
#include "Emu/Utility/System/Memory/VirtualMemory.h"

namespace Onikiri {
    namespace EmulatorUtility {



        class MemorySystem
        {
            static const int RESERVED_PAGES = 256; // �S�a��ŗ\�񂷂�y�[�W
            static const int RESERVED_PAGE_NULL = 0;
            static const int RESERVED_PAGE_ZERO_FILLED = 1;
        public:
            MemorySystem( int pid, bool bigEndian, SystemIF* simSystem );
            ~MemorySystem();

            // �������Ǘ�
            // �V�X�e�����猩����y�[�W�T�C�Y
            u64 GetPageSize();

            // �A�h���X0���炱��ŕԂ��A�h���X�܂ł͗\��D
            u64 GetReservedAddressRange(){ return GetPageSize() * RESERVED_PAGES - 1; };

            // mmap�Ɏg���q�[�v�ɁC[addr, addr+size) �̗̈��ǉ�����
            void AddHeapBlock(u64 addr, u64 size);

            // brk�̏����l (���[�h���ꂽ�C���[�W�̖���) ��ݒ肷��
            void SetInitialBrk(u64 initialBrk);
            u64 Brk(u64 addr);
            u64 MMap(u64 addr, u64 length);
            u64 MRemap(u64 old_addr, u64 old_size, u64 new_size, bool mayMove = false);
            int MUnmap(u64 addr, u64 length);

            // �r�b�O�G���f�B�A�����ǂ���
            bool IsBigEndian() const 
            {
                return m_bigEndian;
            }

            u64 GetPageSize() const
            {
                return m_virtualMemory.GetPageSize();
            }

            // [addr, addr+size) ���܂ރy�[�W�ɕ��������������蓖�Ă�D����
            void AssignPhysicalMemory(u64 addr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr )
            {
                m_virtualMemory.AssignPhysicalMemory( addr, size, attr );
            }

            // �������ǂݏ���
            void ReadMemory( MemAccess* access ) 
            {
                m_virtualMemory.ReadMemory( access );
            }

            void WriteMemory( MemAccess* access )
            {
                m_virtualMemory.WriteMemory( access );
            }


            // �������֘A�̃w���p
            // targetAddr����size�o�C�g��value�̒l����������
            void TargetMemset(u64 targetAddr, int value, u64 size )
            {
                m_virtualMemory.TargetMemset( targetAddr, value, size );
            }
            // target �̃A�h���X src ����Chost �̃A�h���X dst �� size �o�C�g���R�s�[����
            void MemCopyToHost(void* dst, u64 src, u64 size)
            {
                m_virtualMemory.MemCopyToHost( dst, src, size );
            }
            // host �̃A�h���X src ����Ctarget �̃A�h���X dst �� size �o�C�g���R�s�[����
            void MemCopyToTarget(u64 dst, const void* src, u64 size)
            {
                m_virtualMemory.MemCopyToTarget( dst, src, size );
            }

        private:

            // Cehck a value is aligned on a page boundary.
            void CheckValueOnPageBoundary( u64 addr, const char* signature );

            VirtualMemory m_virtualMemory;
            HeapAllocator m_heapAlloc;

            // ���s�t�@�C������߂�̈�̏I�[ (currentBrk���܂܂�) Brk �Ŋg��
            u64 m_currentBrk;

            // �������m�ہC������ɃV�~�����[�^�ɃR�[���o�b�N�𓊂��邽�߂̃C���^�[�t�F�[�X
            SystemIF* m_simSystem;

            // PID
            int m_pid;

            // �^�[�Q�b�g���r�b�O�G���f�B�A�����ǂ���
            bool m_bigEndian;
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
