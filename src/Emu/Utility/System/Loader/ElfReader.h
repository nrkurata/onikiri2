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


#ifndef __EMULATORUTILITY_ELFREADER_H__
#define __EMULATORUTILITY_ELFREADER_H__

#include <fstream>
#include <vector>

#include "SysDeps/Endian.h"
#include "Emu/Utility/System/Loader/ELF64.h"

namespace Onikiri {
    namespace EmulatorUtility {

        // Elf64 reader
        class ElfReader
        {
        public:
            typedef ELF64_HEADER  Elf_Ehdr;
            typedef ELF64_SECTION Elf_Shdr;
            typedef ELF64_PROGRAM Elf_Phdr;
            typedef u64 Elf_Addr;
            typedef u32 Elf_Word;
            typedef u64 Elf_Off;

            typedef std::streamsize streamsize;

            explicit ElfReader();
            ~ElfReader();

            // ELF�t�@�C�����J���C�w�b�_����ǂݍ���
            // �ǂݍ��݂Ɏ��s����� runtime_error �𓊂���
            void Open(const char *name);
            void Close();

            int GetClass() const;
            int GetDataEncoding() const;
            int GetVersion() const;
            u16 GetMachine() const;
            bool IsBigEndian() const;

            int GetSectionHeaderCount() const;
            int GetProgramHeaderCount() const;

            Elf_Addr GetEntryPoint() const;
            const Elf_Shdr &GetSectionHeader(int index) const;
            const Elf_Phdr &GetProgramHeader(int index) const;

            // name �����Z�N�V������index�𓾂�
            // ������Ȃ����-1
            int FindSection(const char *name) const;
            // �Z�N�V����index�̃Z�N�V�������𓾂�
            const char *GetSectionName(int index) const;

            // �Z�N�V����index�̓��e��buf�ɓǂݍ��� (���s�� runtime_error �𓊂���)
            void ReadSectionBody(int index, char *buf, size_t buf_size) const;
            // offset ���� buf_size �����ǂݍ��� (���s�� runtime_error �𓊂���)
            void ReadRange(size_t offset, char *buf, size_t buf_size) const;

            // ELF�t�@�C���̃T�C�Y�𓾂�
            streamsize GetImageSize() const;
            // ELF�t�@�C����S�ēǂݍ��� (���s�� runtime_error �𓊂���)
            void ReadImage(char *buf, size_t buf_size) const;

            Elf_Off GetSectionHeaderOffset() const;
            Elf_Off GetProgramHeaderOffset() const;

        private:
            // ELF�w�b�_��ǂݍ��ށDmachine: ���҂����machine (���s�� runtime_error �𓊂���)
            void ReadELFHeader();
            // �Z�N�V�����w�b�_��ǂݍ��� (���s�� runtime_error �𓊂���)
            void ReadSectionHeaders();
            // �v���O�����w�b�_��ǂݍ��� (���s�� runtime_error �𓊂���)
            void ReadProgramHeaders();
            // �Z�N�V�����l�[���e�[�u�� (Elf_Ehdr.e_shstrndx�̎����Z�N�V����) �̓��e��ǂݍ��� (���s�� runtime_error �𓊂���)
            void ReadSectionNameTable();

            bool m_bigEndian;

            mutable std::ifstream m_file;

            Elf_Ehdr    m_elfHeader;
            std::vector<Elf_Shdr> m_elfSectionHeaders;
            std::vector<Elf_Phdr> m_elfProgramHeaders;
            char *m_sectionNameTable;
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif // #ifndef ELFREAD_H_INCLUDED

