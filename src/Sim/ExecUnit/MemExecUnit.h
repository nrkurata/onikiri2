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


#ifndef SIM_EXEC_UNIT_MEM_EXEC_UNIT_H
#define SIM_EXEC_UNIT_MEM_EXEC_UNIT_H

#include <utility>

#include "Env/Param/ParamExchange.h"

#include "Sim/ExecUnit/PipelinedExecUnit.h"
#include "Sim/Op/OpArray/OpArray.h"

namespace Onikiri 
{
    class Cache;
    class CacheSystem;
    class MemOrderManager;

    class MemExecUnit : public PipelinedExecUnit
    {

    public:
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY("@FloatConversionLatency",  m_floatConversionLatency)
            END_PARAM_PATH()
            CHAIN_BASE_PARAM_MAP( PipelinedExecUnit )
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( MemOrderManager,    "memOrderManager",  m_memOrderManager )
            RESOURCE_ENTRY( CacheSystem,        "cacheSystem",      m_cacheSystem )
            CHAIN_BASE_RESOURCE_MAP( PipelinedExecUnit )
        END_RESOURCE_MAP()

        MemExecUnit();
        virtual ~MemExecUnit();

        void Initialize( InitPhase phase );

        // ���s���C�e���V��� FinishEvent ��o�^����
        virtual void Execute(OpIterator op);

        // OpClass �����肤�郌�C�e���V�̎�ނ̐���Ԃ�
        virtual int GetLatencyCount(const OpClass& opClass);

        // OpClass �ƃC���f�N�X���烌�C�e���V��Ԃ�
        virtual int GetLatency(const OpClass& opClass, int index);

        // accessors
        Cache* GetCache(){ return m_cache; }

    protected:
        
        // �L���b�V��
        CacheSystem*    m_cacheSystem;
        Cache*          m_cache;

        // �L���b�V���̐�
        int m_cacheCount;
        
        // float �� Load �̕ϊ����C�e���V
        int m_floatConversionLatency;
        PhysicalResourceArray<MemOrderManager> m_memOrderManager;

        // Read�̎��s���C�e���V��Ԃ�
        int GetExecutedReadLatency( OpIterator op );

        // Write�̎��s���C�e���V��Ԃ�
        int GetExecutedWriteLatency( OpIterator op );

        // Get the actual latency of executed 'op'.
        int GetExecutedLatency( OpIterator op );

        // Get a producer store of 'consumer'.
        OpIterator GetProducerStore( OpIterator consumer );
    };

}; // namespace Onikiri

#endif // __MEMEXECUNIT_H__

