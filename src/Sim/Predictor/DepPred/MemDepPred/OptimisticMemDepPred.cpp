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

#include "Sim/Predictor/DepPred/MemDepPred/OptimisticMemDepPred.h"

using namespace Onikiri;

OptimisticMemDepPred::OptimisticMemDepPred() : 
    m_core(0),
    m_numAccessOrderViolated(0)
{
}

OptimisticMemDepPred::~OptimisticMemDepPred()
{
    ReleaseParam();
}

void OptimisticMemDepPred::Initialize(InitPhase phase)
{
}

// �A�h���X��v/�s��v�\��
// �S�Ă̐�sstore�ɑ΂��ĕs��v�Ɨ\������̂ŁA�������Ȃ�
void OptimisticMemDepPred::Resolve(OpIterator op)
{
}

// �S�Ă̐�sstore�ɑ΂��ĕs��v�Ɨ\������̂ŁA�������Ȃ�
void OptimisticMemDepPred::Allocate(OpIterator op)
{
}

// �S�Ă̐�sstore�ɑ΂��ĕs��v�Ɨ\������̂ŁA�������Ȃ�
void OptimisticMemDepPred::Commit(OpIterator op)
{
}

// �S�Ă̐�sstore�ɑ΂��ĕs��v�Ɨ\������̂ŁA�������Ȃ�
void OptimisticMemDepPred::Flush(OpIterator op)
{
}

// MemOrderManager�ɂ���āAMemOrder��conflict���N������op�̑g(producer, consumer)�������Ă��炤
// OptimisticMemDepPred�ł͉������Ȃ�
void OptimisticMemDepPred::OrderConflicted(OpIterator producer, OpIterator consumer)
{
    m_numAccessOrderViolated++;
}

// ���ۂɂ�PhyReg�ł͖�����������̈ˑ��ł��邽�߁APhyReg�ւ̊��蓖�Ă͍s���Ȃ�
// ���̂��߁A�K��true��Ԃ�
bool OptimisticMemDepPred::CanAllocate(OpIterator* infoArray, int numOp)
{
    return true;
}
