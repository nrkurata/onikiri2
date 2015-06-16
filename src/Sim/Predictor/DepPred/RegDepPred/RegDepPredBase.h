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


#ifndef __REGDEPPRED_H__
#define __REGDEPPRED_H__

#include "Sim/Predictor/DepPred/RegDepPred/RegDepPredIF.h"
#include "Sim/Dependency/PhyReg/PhyReg.h"

namespace Onikiri 
{

    // ���W�X�^�̈ˑ��֌W�\����̃x�[�X�N���X�D
    // DepPredIF �֘A�̃��\�b�h�͂��̃N���X�Ŏ�������Ă���C
    // ����炩�烌�W�X�^�ˑ���͂̃��\�b�h���Ă΂��
    class RegDepPredBase : public RegDepPredIF 
    {
        SharedPtrObjectPool<PhyReg> m_phyRegPool;
    public:

        RegDepPredBase();
        virtual ~RegDepPredBase();

        // -- DepPredIF 
        
        // op��Consumer
        virtual void Resolve(OpIterator op);
        // op��Producer
        virtual void Allocate(OpIterator op);
        // Commit���A�����_�����W�X�^�ԍ��ɏ������ޒ��O�̖��߂̃f�X�e�B�l�[�V�����E���W�X�^�����
        virtual void Commit(OpIterator op);
        // Flush���A�����̃f�X�e�B�l�[�V�����E���W�X�^�����
        virtual void Flush(OpIterator op);
    };

}; // namespace Onikiri

#endif // __REGDEPPRED_H__

