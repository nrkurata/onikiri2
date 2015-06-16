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


#ifndef __PIPELINE_NODE_IF_H__
#define __PIPELINE_NODE_IF_H__

#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Foundation/TimeWheel/ClockedResourceIF.h"

namespace Onikiri 
{
    class Pipeline;
    class Core;

    // PipelineNode�̃C���^�t�F�[�X
    class PipelineNodeIF/*  : public ClockedResourceIF*/
    {
    public:
        PipelineNodeIF(){}
        virtual ~PipelineNodeIF(){}

        // Op��Commit����Ƃ�
        virtual void Commit( OpIterator op ) = 0;

        // Op��Cancel�����Ƃ�
        virtual void Cancel( OpIterator op ) = 0;

        // �����̒�����Op������
        virtual void Retire( OpIterator op ) = 0;

        // Op��Flush�����Ƃ�
        virtual void Flush( OpIterator op ) = 0;

        // �p�C�v���C�����X�g�[��������
        virtual void StallThisCycle() = 0;
        // �p�C�v���C����cycle���X�g�[������
        virtual void StallNextCycle(int cycle) = 0;

        // ������Pipeline���擾
        virtual Pipeline* GetLowerPipeline() = 0;

        // ������PipelineNode���擾
        virtual PipelineNodeIF* GetLowerPipelineNode() = 0;

        // �㗬�̃p�C�v���C���m�[�h���擾
        virtual PipelineNodeIF* GetUpperPipelineNode() = 0;

        // Set lower pipeline nodes.
        virtual void SetLowerPipelineNode( PipelineNodeIF* lower ) = 0;

        // This method is called when an op exits an upper pipeline.
        // This method is used for a pipeline writing a buffer(ex. Dispatcher).
        virtual void ExitUpperPipeline( OpIterator op ) = 0;

        // This method is called when an op exits an lower pipeline.
        virtual void ExitLowerPipeline( OpIterator op ) = 0;

        // Returns whether this node can allocate entries or not.
        virtual bool CanAllocate( int ops ) = 0;
    };

}; // namespace Onikiri

#endif // __PIPELINENODEIF_H__

