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


#ifndef __EMULATORUTILITY_LOADERIF_H__
#define __EMULATORUTILITY_LOADERIF_H__

namespace Onikiri {
    namespace EmulatorUtility {

        class MemorySystem;

        // �^�[�Q�b�g�̃o�C�i�����������Ƀ��[�h���C������ݒ肷��C���^�[�t�F�C�X
        // Linux �p��������Ȃ�
        class LoaderIF
        {
        public:
            virtual ~LoaderIF() {}

            // �o�C�i�� command �� memory �Ƀ��[�h����
            virtual void LoadBinary(MemorySystem* memory, const String& command) = 0;

            // �X�^�b�N [stackHead, stackHead+stackSize) �Ɉ�����ݒ肷��
            virtual void InitArgs(MemorySystem* memory, u64 stackHead, u64 stackSize, const String& command, const String& commandArgs) = 0;

            // �o�C�i���̃��[�h���ꂽ�̈�̐擪�A�h���X�𓾂�
            // ���o�C�i�����A���̈�Ƀ��[�h����邱�Ƃ����肵�Ă���
            virtual u64 GetImageBase() const = 0;

            // �G���g���|�C���g�𓾂�
            virtual u64 GetEntryPoint() const = 0;

            // �R�[�h�̈�͈̔͂�Ԃ� (�J�n�A�h���X�C�o�C�g��) (Emulator�̍œK���p)
            virtual std::pair<u64, size_t> GetCodeRange() const = 0;

            // ���W�X�^�̏����l�𓾂�
            virtual u64 GetInitialRegValue(int index) const = 0;
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
