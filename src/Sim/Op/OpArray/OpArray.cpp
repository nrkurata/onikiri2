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


#include <pch.h>

#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Op/Op.h"

#include "Utility/RuntimeError.h"

using namespace Onikiri;

// --
OpArray::ArrayID::~ArrayID()
{
    delete m_op;
}

// --
OpArray::OpArray(int capacity) : 
    m_capacity(capacity)
{
    // Op �̊m��
    for(int k = 0; k < m_capacity; ++k) {
        // ArrayID �̍쐬(���̎��_�ł�Op���܂��쐬����Ă��Ȃ��̂�0)
        ArrayID* arrayID = new ArrayID(0, this, k);
        // arrayID �𗘗p���� op ���쐬
        Op* op = new Op( OpIterator(arrayID) );
        // arrayID �� op ���Z�b�g
        arrayID->SetOp(op);
            
        m_body.push_back(arrayID);
    }

    // �g�p�����ǂ����̃t���O�̏�����
    m_alive.resize(m_capacity, false);
    
    // free list �̏�����
    m_freeList.reserve(m_capacity);
    for(int k = 0; k < m_capacity; ++k) {
        m_freeList.push_back(k);
    }
}

OpArray::~OpArray()
{
    for(int k = 0; k < m_capacity; ++k) {
        delete m_body[k];
    }
    m_body.clear();
}

OpIterator OpArray::CreateOp()
{
    if( IsFull() ) {
        THROW_RUNTIME_ERROR("OpArray is full.(increase Core/@OpArrayCapacity)");
    }
    // free list �̖�����Ԃ�
    ID id = m_freeList.back();

    // free list ���� pop
    m_freeList.pop_back();

    // �g�p���t���O�𗧂Ă�
    ASSERT( 
        !IsAlive(id),
        "alive id reused.(id = %d)",
        id
    );
    m_alive[id] = true;

    // id�Ԗڂ̃I���W�i����OpIterator��Ԃ�
    return OpIterator(m_body[id]);
}

void OpArray::ReleaseOp(const OpIterator& opIterator)
{
    ID id = opIterator.GetID();

    ASSERT(
        IsAlive(opIterator),
        "not alive op released.(id = %d)",
        id
    );
    // ������ꂽ�̂Ő����t���O�𗎂Ƃ�
    m_alive[id] = false;
    m_freeList.push_back(id);
}

bool OpArray::IsAlive(const OpIterator& opIterator) const
{
    return IsAlive( opIterator.GetID() ); 
}
