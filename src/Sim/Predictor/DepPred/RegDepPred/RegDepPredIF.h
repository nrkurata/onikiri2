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


#ifndef __REGDEPPRED_IF_H__
#define __REGDEPPRED_IF_H__

#include "Sim/Predictor/DepPred/DepPredIF.h"

namespace Onikiri 
{

    // ���W�X�^�̈ˑ��֌W�\����̃C���^�[�t�F�[�X
    class RegDepPredIF : public DepPredIF 
    {
    public:
        virtual ~RegDepPredIF(){};

        // �\�[�X�E���W�X�^�ɑΉ����镨�����W�X�^�ԍ���Ԃ�
        virtual int ResolveReg(const int lno) = 0;

        // ResolveReg �Ɠ��l�ɕ������W�X�^�ԍ���Ԃ��D
        // ���������̃��\�b�h�̌Ăяo���ɂ�蕛��p���Ȃ������ۏ؂����D
        // �G�~�����[�V����<>�V�~�����[�V�������Ȃǂ�
        // �R���e�L�X�g�擾���ȂǂɎg�p
        virtual int PeekReg(const int lno) const = 0;
        
        // �f�X�e�B�l�[�V�����E���W�X�^�ɕ������W�X�^�ԍ������蓖�Ă�
        virtual int AllocateReg(OpIterator op, const int lno) = 0;

        // retire�����̂ŁAop��������ׂ��������W�X�^�����
        virtual void ReleaseReg(OpIterator op, const int lno, int phyRegNo) = 0;
        // flush���ꂽ�̂ŁAop�̃f�X�e�B�l�[�V�����E���W�X�^�����
        virtual void DeallocateReg(OpIterator op, const int lno, int phyRegNo) = 0; 

        // num�������W�X�^�����蓖�Ă邱�Ƃ��ł��邩�ǂ���
        virtual bool CanAllocate(OpIterator* infoArray, int numOp) = 0;

        // �_��/�������W�X�^�̌�
        virtual int GetRegSegmentCount() = 0;
        virtual int GetLogicalRegCount(int segment) = 0;
        virtual int GetTotalLogicalRegCount() = 0;
    };

}; // namespace Onikiri

#endif // __REGDEPPRED_IF_H__

