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


#ifndef SIM_SYSTEM_SYSTEM_BASE_H
#define SIM_SYSTEM_SYSTEM_BASE_H

#include "Sim/Foundation/Hook/Hook.h"

#include "Sim/Thread/Thread.h"
#include "Sim/Core/Core.h"
#include "Interface/EmulatorIF.h"
#include "Env/Param/ParamExchange.h"
#include "Sim/Foundation/Resource/Builder/ResourceBuilder.h"

#include "Sim/System/GlobalClock.h"
#include "Sim/System/SimulationSystem/EmulatorWrapper.h"
#include "Sim/System/ArchitectureState.h"

namespace Onikiri 
{
    class ForwardEmulator;

    // Notify �� SystemManager �o�R�Ŏ󂯎�邽�߁CSystemBase �R���N���X��
    // SystemManagerIF::SetSystem ���o�R���Ď��g�� SystemManager �ɓo�^����K�v������D
    // �܂��C���g�̔j�󎞂ɂ� SystemManagerIF::SetSystem �o�R�Ŏ��g�̓o�^����������K�v������D
    // ����ɂ��C��O�ɂ����SystemBase �R���N���X���j�󂳂ꂽ�ꍇ�ł��C�K�؂�
    // SystemManager �̓o�^���������邱�Ƃ��o����D
    class SystemManagerIF
    {
    public:
        virtual ~SystemManagerIF(){};
        virtual void SetSystem( SystemIF* system ) = 0;
    };

    // �v���Z�b�T�S�̂��Ǘ�����N���X
    class SystemBase : public SystemIF
    {
    public:
    
        struct SystemContext
        {
            SystemContext();
            String targetArchitecture;  // Target architecture
            String mode;                // Simulation mode

            s64 executionCycles;    
            s64 executionInsns;

            s64 executedCycles; 
            std::vector<s64> executedInsns; // Executed insns in each thread.

            PhysicalResourceArray<Core>   cores;        // Todo: support non-uniform multi core
            PhysicalResourceArray<Thread> threads;
            PhysicalResourceArray<Cache>  caches;
            GlobalClock*                    globalClock;
            ForwardEmulator*                forwardEmulator;
            
            EmulatorIF*     emulator;
            EmulatorWrapper emulatorWrapper;

            ResourceBuilder *resBuilder;
            ArchitectureStateList architectureStateList;

            struct InorderParam
            {
                bool enableBPred;
                bool enableHMPred;
                bool enableCache;
            };
            InorderParam inorderParam;

            struct DebugParam
            {
                int debugPort;
            };
            DebugParam debugParam;
        };

        virtual void Run( SystemContext* context ) = 0;

        // SystemIF
        virtual void NotifyProcessTermination(int pid){}
        virtual void NotifySyscallReadFileToMemory(const Addr& addr, u64 size){}
        virtual void NotifySyscallWriteFileFromMemory(const Addr& addr, u64 size){}
        virtual void NotifyMemoryAllocation(const Addr& addr, u64 size, bool allocate){};

        //
        SystemBase();
        virtual ~SystemBase();
        void SetSystemManager( SystemManagerIF* systemManager );

    protected:
        SystemManagerIF* m_systemManager;
    };

}; // namespace Onikiri

#endif // SIM_SYSTEM_SYSTEM_BASE_H

