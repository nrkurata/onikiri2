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


#ifndef __EMULATORUTILITY_HEAPALLOCATOR_H__
#define __EMULATORUTILITY_HEAPALLOCATOR_H__

namespace Onikiri {
    namespace EmulatorUtility {

        // �^�[�Q�b�g�̃q�[�v�̈���Ǘ�����
        // 
        // �A�h���X��Ԃ݂̂��Ǘ����� (�����������̊��蓖�Ă͍s��Ȃ�)
        // �̈�̊m�ۂ̓y�[�W�P�ʂōs����
        class HeapAllocator
        {
        private:
            HeapAllocator() {}
        public:
            // pageSize
            explicit HeapAllocator(u64 pageSize);

            // �A�h���Xstart����length�o�C�g�̗̈���C����HeapAllocator�̎g�p����i�󂫁j�̈�Ƃ��Ēǉ�����
            bool AddMemoryBlock(u64 start, u64 length);

            // addr�̃A�h���X��length�o�C�g�̗̈���m�ۂ���
            // addr��0�̏ꍇ�C�󂫗̈�̒��œK���ȃA�h���X�Ɋm�ۂ����
            //
            // �߂�l: �m�ۂ��ꂽ�̈�̃A�h���X�D0�̏ꍇ�G���[
            //        �A�h���X�̓y�[�W���E�ɂ��邱�Ƃ��ۏ؂����
            u64 Alloc(u64 addr, u64 length);

            // addr�̈ʒu�Ɋm�ۂ��ꂽ�̈�̃T�C�Y�� old_size ���� new_size �ɕύX����
            //
            // �߂�l: �̈�̃A�h���X�D0�̏ꍇ�G���[
            //        �A�h���X�̓y�[�W���E�ɂ��邱�Ƃ��ۏ؂����
            u64 ReAlloc(u64 old_addr, u64 old_size, u64 new_size);

            // Alloc�����������̈���J������
            // �߂�l: ������true�C���s��false (�Ǘ�����Ă��Ȃ��������u���b�N��n������)
            bool Free(u64 addr);
            // Alloc�����̈�̈ꕔ�܂��͑S�����J������
            bool Free(u64 addr, u64 size);

            // addr ��Alloc���ꂽ�������̈�̃T�C�Y�𓾂�
            u64 GetBlockSize(u64 addr) const;

            // addr �͊m�ۂ���Ă���̈�ƌ������邩
            u64 IsIntersected(u64 addr, u64 length) const;

            u64 GetPageSize() const { return m_pageSize; }

        private:
            struct MemoryBlock
            {
                u64 Addr;
                u64 Bytes;

                explicit MemoryBlock(u64 addr_arg = 0, u64 bytes_arg = 0) : Addr(addr_arg), Bytes(bytes_arg) { }
                bool operator<(const MemoryBlock& mb) const{
                    return Addr < mb.Addr;
                }
                bool operator==(const MemoryBlock& mb) const {
                    return Addr == mb.Addr;
                }
                bool Intersects(const MemoryBlock& mb) const {
                    return (Addr <= mb.Addr && mb.Addr < Addr+Bytes)
                        || (mb.Addr <= Addr && Addr < mb.Addr+mb.Bytes);
                }
                bool Contains(const MemoryBlock& mb) const {
                    return (Addr <= mb.Addr && mb.Addr+mb.Bytes < Addr+Bytes);
                }
            };
            typedef std::list<MemoryBlock> BlockList;

            // m_freeList���̌��ԂȂ��אڂ��Ă��郁�����̈��1�ɂ܂Ƃ߂�
            void IntegrateFreeBlocks();
            BlockList::iterator FindMemoryBlock(BlockList& blockList, u64 addr);
            BlockList::const_iterator FindMemoryBlock(const BlockList& blockList, u64 addr) const;


            u64 in_pages(u64 bytes) const {
                return (bytes + m_pageSize - 1)/m_pageSize;
            }

            // free list�̓������̈�̒��ɒu���Ȃ�
            // target���������j�󂵂Ă�����mmap�����ȂȂ��悤��
            BlockList m_freeList;
            BlockList m_allocList;
            u64 m_pageSize;
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
