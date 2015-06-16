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


#ifndef __EMULATORIF_H__
#define __EMULATORIF_H__

#include <utility>

#include "Interface/SystemIF.h"
#include "Interface/MemIF.h"
#include "Interface/Addr.h"
#include "Interface/OpInfo.h"
#include "Interface/OpStateIF.h"
#include "Interface/ISAInfo.h"
#include "Interface/ExtraOpDecoderIF.h"

namespace Onikiri {

    // �G�~�����[�^�̃C���^�[�t�F�[�X
    class EmulatorIF {

    public:
        EmulatorIF() {}
        virtual ~EmulatorIF() {}

        // �߂�l�� first: pc�ɂ��閽�߂𕪉����Đ������ꂽ�C
        // OpInfo�ւ̃|�C���^�̔z��@second: MOp�̐�
        virtual std::pair<OpInfo**, int> GetOp(PC pc) = 0;

        // sim���Ƀ������̃C���[�W��������
        // �X�g�A�L���[�Ƀf�[�^�������������ɓǂݏ������邽��
        virtual MemIF* GetMemImage() = 0;

        // opInfo �̖��߂����s����D�\�[�X�I�y�����h���̎擾�C���ʂ̊i�[�� opStateIF �ɑ΂��čs��
        virtual void Execute(OpStateIF* opStateIF, OpInfo* opInfo) = 0;

        // Commit instructions executed in Execute().
        // Currently, CRC calculation of pcs for debugging is done in Commit().
        virtual void Commit(OpStateIF* opStateIF, OpInfo* opInfo) = 0;

        // �������ꂽ�v���Z�X�̐����擾����
        virtual int GetProcessCount() const = 0;

        // pid�Ԃ̃v���Z�X�̃G���g���[�|�C���g���擾����
        virtual PC GetEntryPoint(int pid) const = 0;

        // ���W�X�^�̏����l���擾����
        virtual u64 GetInitialRegValue(int pid, int index) const = 0;

        // ISA���̎擾
        virtual ISAInfoIF* GetISAInfo() = 0;

        // pc ���� skipCount ���ߎ��s����D���s�������PC��Ԃ��DexecutedInsnCount, executedOpCount�Ɏ��ۂɎ��s�ł������ߐ���Op����Ԃ� (NULL��)
        virtual PC Skip(PC pc, u64 skipCount, u64* regArray, u64* executedInsnCount, u64* executedOpCount) = 0;

        // �O�����߃f�R�[�_���Z�b�g����
        virtual void SetExtraOpDecoder( ExtraOpDecoderIF* extraOpDecoder ) = 0;
    };

}; // namespace Onikiri


#endif
