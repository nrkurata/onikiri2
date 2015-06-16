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


//
// GlobalHistory�̊��N���X
//
// ����̃O���[�o���ȗ����������A����\����/���򖽗߂�Retire����Update���s��
// ���̃N���X�ł́A����\�����ɓ��@�I��Update�����A����~�X���ɂ�Checkpoint�ɂ��
// �����I��GlobalHistory�̊����߂����s����
//

#ifndef __GLOBALHISTORY_H__
#define __GLOBALHISTORY_H__

#include "Types.h"
#include "Sim/Foundation/Checkpoint/CheckpointedData.h"
#include "Sim/Foundation/Resource/ResourceNode.h"

namespace Onikiri 
{
    class CheckpointMaster;

    class GlobalHistory : public PhysicalResourceNode 
    {
    protected:
        CheckpointMaster* m_checkpointMaster;
        CheckpointedData<u64> m_globalHistory; // ���򗚗�: 1bit�������Taken/NotTaken�ɑΉ�

    public:
        GlobalHistory();
        virtual ~GlobalHistory();

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( CheckpointMaster, "checkpointMaster", m_checkpointMaster )
        END_RESOURCE_MAP()

        // ������
        void Initialize(InitPhase phase);

        // dirpred �̗\�����ɗ\�����ʂ� bpred ���狳���Ă��炤
        void Predicted(bool taken);
        // �����Retire����Taken/NotTaken�� bpred ���狳���Ă��炤
        void Retired(bool taken);

        // �ŉ��ʃr�b�g(1�ԍŐV�̂���)��(�����I��)�ύX����
        void SetLeastSignificantBit(bool taken);

        // accessors
        u64 GetHistory();

    };

}; // namespace Onikiri

#endif // __GLOBALHISTORY_H__

