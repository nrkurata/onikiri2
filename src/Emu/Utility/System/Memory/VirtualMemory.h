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


#ifndef __EMULATORUTILITY_VIRTUAL_MEMORY_H__
#define __EMULATORUTILITY_VIRTUAL_MEMORY_H__

#include "Emu/Utility/System/Memory/HeapAllocator.h"


namespace Onikiri {
    namespace EmulatorUtility {

        // Page size
        static const int VIRTUAL_MEMORY_PAGE_SIZE_BITS = 12;
        
        // Page attribute
        typedef u32 VIRTUAL_MEMORY_ATTR_TYPE;
        static const u32 VIRTUAL_MEMORY_ATTR_READ  = 1 << 0;    // readable
        static const u32 VIRTUAL_MEMORY_ATTR_WRITE = 1 << 1;    // writable
        static const u32 VIRTUAL_MEMORY_ATTR_EXEC  = 1 << 2;    // executable

        // Information about a physical memory page.
        // Each instance of this structure corresponds to each 'physical' page.
        struct PhysicalMemoryPage
        {
            u8* ptr;        // Physical memory address
            int refCount;   // A reference count for a copy-on-write function.
        };

        // Page table entry
        // Each instance of this structure corresponds to each 'logical' page.
        struct PageTableEntry
        {
            PhysicalMemoryPage*      phyPage;   // Physical page
            VIRTUAL_MEMORY_ATTR_TYPE attr;      // Page attribute
        };

        // 1-entry TLB
        class TLB
        {
        protected:
            u64   m_addr;
            bool  m_valid;
            PageTableEntry m_body;
            u64 m_offsetBits;
            u64 m_addrMask;

        public:
            TLB( int offsetBits );
            ~TLB();
            bool Lookup( u64 addr, PageTableEntry* entry ) const;
            void Write( u64 addr, const PageTableEntry& entry ); 
            void Flush();
        };

        // target �̃A�h���X���� host �̃A�h���X�ւ̕ϊ����s���N���X
        class PageTable
        {
        public:
            // �}�b�v�P�ʂ��I�t�Z�b�g�Ŏw�肵�� PageTable ���\�z���� (PageSize = 1 << offsetBits)
            explicit PageTable(int offsetBits);
            ~PageTable();

            // �\�z���ɐݒ肳�ꂽ�y�[�W�T�C�Y�𓾂�
            size_t GetPageSize() const;

            // �\�z���ɐݒ肳�ꂽ�I�t�Z�b�g�̃r�b�g���𓾂�
            int GetOffsetBits() const;

            // �A�h���X�̃y�[�W�O�����݂̂̃}�X�N
            u64 GetOffsetMask() const;

            // target �̃A�h���X addr �Ɋ��蓖�Ă��Ă��� host �̕����������A�h���X�𓾂�
            void *TargetToHost(u64 addr);

            // target �A�h���X��Ԃ�targetAddr���܂ރ}�b�v�P�ʂɁChostAddr �̕��������������蓖�Ă� (PageSize �o�C�g)
            // Set an attribute ('attr') to target page.
            void AddMap(u64 targetAddr, u8* hostAddr, VIRTUAL_MEMORY_ATTR_TYPE attr);
            bool IsMapped(u64 targetAddr) const;

            // Copy address mapping for copy-on-write.
            // Set an attribute ('dstAttr') to target page.
            void CopyMap( u64 dstTargetAddr, u64 srcTargetAddr, VIRTUAL_MEMORY_ATTR_TYPE dstAttr );

            // targetAddr���܂ރ}�b�v�P�ʂɊ��蓖�Ă�ꂽ�G���g������������/����
            bool GetMap( u64 targetAddr, PageTableEntry* page );
            bool SetMap( u64 targetAddr, const PageTableEntry& page );

            // Get a reference count of a page including targetAddr
            int GetPageReferenceCount( u64 targetAddr ); 

            // targetAddr���܂ރ}�b�v�P�ʂւ̊��蓖�Ă���������
            // �Ԃ�l�͉�����̃��t�@�����X�J�E���g
            int RemoveMap(u64 targetAddr);

        private:
            class AddrHash
            {
            private:
                int m_offsetBits;
            public:
                explicit AddrHash(int offsetBits) : m_offsetBits(offsetBits) {}

                size_t operator()(const u64 value) const
                {
                    return (size_t)(value >> m_offsetBits);
                }
            };
            typedef unordered_map<u64, PageTableEntry, AddrHash > map_type;
            map_type m_map;
            int m_offsetBits;
            u64 m_offsetMask;
            TLB m_tlb;
            boost::pool<> m_phyPagePool;
        };


        class VirtualMemory
        {
        public:
            VirtualMemory( int pid, bool bigEndian, SystemIF* simSystem );
            ~VirtualMemory();

            // �������ǂݏ���
            void ReadMemory( MemAccess* access );
            void WriteMemory( MemAccess* access );

            //
            // Helper methods for memory access.
            // These methods are implemented in this class because
            // implementation using ReadMemory/WriteMemory is too slow.
            //
            // Note: These functions ignore page attribute.
            //

            // targetAddr����size�o�C�g��value�̒l����������
            void TargetMemset(u64 targetAddr, int value, u64 size);
            // target �̃A�h���X src ����Chost �̃A�h���X dst �� size �o�C�g���R�s�[����
            void MemCopyToHost(void* dst, u64 src, u64 size);
            // host �̃A�h���X src ����Ctarget �̃A�h���X dst �� size �o�C�g���R�s�[����
            void MemCopyToTarget(u64 dst, const void* src, u64 size);

            // �������Ǘ�
            u64 GetPageSize() const;

            // addr ���܂ރy�[�W�ɕ��������������蓖�Ă�DAddHeapBlock�����̈�Əd�Ȃ��Ă��Ă͂Ȃ�Ȃ�
            void AssignPhysicalMemory(u64 addr, VIRTUAL_MEMORY_ATTR_TYPE attr);
            // [addr, addr+size) ���܂ރy�[�W�ɕ��������������蓖�Ă�D����
            void AssignPhysicalMemory(u64 addr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr);
            // addr ���܂ރy�[�W�Ɋ��蓖�Ă��������������������
            void FreePhysicalMemory(u64 addr);
            // [addr, addr+size) ���܂ރy�[�W�Ɋ��蓖�Ă��������������������
            void FreePhysicalMemory(u64 addr, u64 size);
            
            // dstAddr ���܂ރy�[�W�� srcAddr ���܂ރy�[�W�Ɍ��݊��蓖�Ă��Ă��镨�������������蓖�Ă�D
            void SetPhysicalMemoryMapping( u64 dstAddr, u64 srcAddr, VIRTUAL_MEMORY_ATTR_TYPE attr );
            // [dstAddr, dstAddr+size) ���܂ރy�[�W�ɕ����������}�b�s���O���Z�b�g����D����
            void SetPhysicalMemoryMapping( u64 dstAddr, u64 srcAddr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr );

            // addr ���܂ރy�[�W�ɕ��������������蓖�ĂāCaddr ���܂ރy�[�W�Ɍ��݊��蓖�Ă��Ă����f�[�^���R�s�[
            void AssignAndCopyPhysicalMemory( u64 addr, VIRTUAL_MEMORY_ATTR_TYPE attr  );
            // [dstAddr, dstAddr+size) ���܂ރy�[�W�ɓ���
            void AssignAndCopyPhysicalMemory( u64 addr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr );

            // Write a page attribute to a page including 'addr'.
            void SetPageAttribute( u64 addr, VIRTUAL_MEMORY_ATTR_TYPE attr );

            // Copy-on-Write for a memory page including addr.
            // Return whether copy-on-write is done or not.
            bool CopyPageOnWrite( u64 addr );

            // �r�b�O�G���f�B�A�����ǂ���
            bool IsBigEndian() const {
                return m_bigEndian;
            }

        private:
            // addr ���� size �o�C�g�̃������̈���C�}�b�v�P�ʋ��E�ŕ�������
            // ���ʂ́CMemoryBlock�̃R���e�i�ւ̃C�e���[�^ Iter ��ʂ��Ċi�[����
            // �߂�l�͕������ꂽ��
            // �� Iter�́C�T�^�I�ɂ�MemoryBlock�̃R���e�i��inserter
            template <typename Iter>
            int SplitAtMapUnitBoundary(u64 addr, u64 size, Iter e) const;

            struct MemoryBlock {
                MemoryBlock(u64 addr, u64 size) { this->addr = addr; this->size = size; }
                u64 addr;
                u64 size;
            };
            typedef std::vector<MemoryBlock> BlockArray;

            // PageTable & FreeList
            PageTable m_pageTbl;
            boost::pool<> m_pool;

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
