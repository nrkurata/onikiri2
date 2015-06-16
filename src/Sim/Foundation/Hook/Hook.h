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
// Available hook method formats :
//   void Hook( HookParameterType* )
//   void Hook( Caller )
//   void Hook( Caller, OpIterator )
//   void Hook( Caller, OpIterator, ParameterType )
//   void Hook( Caller, ParameterType* )
//   void Hook( OpIterator )
//   void Hook( OpIterator, ParameterType* )
//   void Hook( ParameterType* )
//


#ifndef SIM_FOUNDATION_HOOK_HOOK_H
#define SIM_FOUNDATION_HOOK_HOOK_H

#include "Utility/RuntimeError.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Foundation/Hook/HookDecl.h"

namespace Onikiri 
{
    //
    // A parameter used when a hook is called.
    // ParamT has a default type, which is CallerT itself, and is 
    // defined in "HookDecl.h" like the following:
    //
    // template <typename CallerT, typename ParamT = CallerT> 
    //
    template <typename CallerT, typename ParamT>
    class HookParameter
    {
    public:
        typedef CallerT CallerType;
        typedef ParamT  ParameterType;

        HookParameter(OpIterator op) 
            : m_op(op), m_caller(0), m_parameter(0)
        {
        }

        HookParameter(CallerType* caller = NULL, ParameterType* parameter = NULL)
            : m_op(0), m_caller(caller), m_parameter(parameter)
        {
        }

        HookParameter(OpIterator op, CallerType* caller = NULL, ParameterType* parameter = NULL)
            : m_op(op), m_caller(caller), m_parameter(parameter)
        {
        }

        OpIterator GetOp() const 
        {
            // A NULL op can be passed.
            return m_op;
        }

        CallerType* GetCaller() const
        {
            ASSERT(m_caller != 0, "caller is not available");
            return m_caller;
        }

        ParameterType* GetParameter() const
        {
            ASSERT(m_parameter != 0, "parameter is not available");
            return m_parameter;
        }

    private:
        OpIterator m_op;
        CallerType* m_caller;
        ParameterType* m_parameter;

    };

    // Hook types
    // This type defines a timing when a registered method is called.
    struct HookType 
    {
        enum Type 
        {
            HOOK_BEFORE,  // Before a hooked body is called.
            HOOK_AROUND,  // A hooked body call is totally bypassed.
            HOOK_AFTER,   // After a hooked body is called.
            HOOK_END      // End sentinel
        };
    };

    // A hook point. Any class methods can be registered to
    // a hook point instance and called.
    // ParamT has a default value that is CallerT itself and is 
    // defined in "HookDecl.h"
    template< typename CallerT, typename ParamT >
    class HookPoint
    {
    public:
        typedef CallerT CallerType;
        typedef ParamT  ParameterType;
        typedef HookParameter<CallerType, ParameterType> HookParameterType;

        // hook �����֐��̊��N���X
        class HookFunctionBase
        {
        public:
            virtual ~HookFunctionBase(){};
            virtual void operator()(HookParameterType* hookParameter) = 0;
        };

        // hook �����֐�(ClassType�̃I�u�W�F�N�g�ƃ����o�ϐ��ւ̃|�C���^�����)
        template <typename ClassType>
        class HookFunction : public HookFunctionBase
        {
        public:
            typedef void(ClassType::* FuncType)();
            HookFunction(ClassType* object, FuncType func) :
              m_object(object), m_func(func)
            {
            }

            void operator()(HookParameterType* hookParameter)
            {
                (m_object->*m_func)();
            }
        protected:
            ClassType* m_object;
            FuncType m_func;
        };

        // HookParameterType* ��n��
        template <typename ClassType>
        class HookFunctionWithParam : public HookFunctionBase 
        {
        public:
            typedef void(ClassType::* FuncType)( HookParameterType* );
            HookFunctionWithParam(ClassType* object, FuncType func) : 
              m_object(object), m_func(func)
            {
            }

            void operator()(HookParameterType* hookParameter)
            {
                (m_object->*m_func)( hookParameter );
            }
        protected:
            ClassType* m_object;
            FuncType m_func;
        };

        // Caller/OpIterator/ParameterType* ��n��
        template <typename ClassType>
        class HookFunctionWithSeparateParam : public HookFunctionBase 
        {
        public:
            typedef void(ClassType::* FuncType)( CallerType* caller, OpIterator op, ParameterType* param );
            HookFunctionWithSeparateParam(ClassType* object, FuncType func) : 
              m_object(object), m_func(func)
            {
            }

            void operator()(HookParameterType* hookParameter)
            {
                (m_object->*m_func)(
                    hookParameter->GetCaller(),  
                    hookParameter->GetOp(),  
                    hookParameter->GetParameter()
                );
            }
        protected:
            ClassType* m_object;
            FuncType m_func;
        };

        // OpIterator ��n��
        template <typename ClassType>
        class HookFunctionWithOp : public HookFunctionBase 
        {
        public:
            typedef void(ClassType::* FuncType)( OpIterator op );
        protected:
            ClassType* m_object;
            FuncType m_func;
        public:
            HookFunctionWithOp(ClassType* object, FuncType func) : 
              m_object(object), m_func(func)
            {
            }

            virtual void operator()(HookParameterType* hookParameter)
            {
                (m_object->*m_func)( hookParameter->GetOp() );
            }
        };

        // Caller ��n��
        template <typename ClassType>
        class HookFunctionWithCaller : public HookFunctionBase 
        {
        public:
            typedef void(ClassType::* FuncType)( CallerType* caller );
            HookFunctionWithCaller(ClassType* object, FuncType func) : 
              m_object(object), m_func(func)
            {
            }

            virtual void operator()(HookParameterType* hookParameter)
            {
                (m_object->*m_func)( hookParameter->GetCaller() );
            }
        protected:
            ClassType* m_object;
            FuncType m_func;
        };

        // Caller/OpIterator ��n��
        template <typename ClassType>
        class HookFunctionWithCallerAndOp : public HookFunctionBase 
        {
        public:
            typedef void(ClassType::* FuncType)( CallerType* caller, OpIterator op );
            HookFunctionWithCallerAndOp(ClassType* object, FuncType func) : 
              m_object(object), m_func(func)
            {
            }

            virtual void operator()(HookParameterType* hookParameter)
            {
                (m_object->*m_func)( hookParameter->GetCaller(), hookParameter->GetOp() );
            }
        protected:
            ClassType* m_object;
            FuncType m_func;
        };

        // OpIterator/ParameterType* ��n��
        template <typename ClassType>
        class HookFunctionWithOpAndRawParam : public HookFunctionBase 
        {
        public:
            typedef void(ClassType::* FuncType)( OpIterator op, ParameterType* parameter );
            HookFunctionWithOpAndRawParam(ClassType* object, FuncType func) : 
              m_object(object), m_func(func)
            {
            }

            virtual void operator()(HookParameterType* hookParameter)
            {
                (m_object->*m_func)( hookParameter->GetOp(), hookParameter->GetParameter() );
            }
        protected:
            ClassType* m_object;
            FuncType m_func;
        };

        // Caller/ParameterType* ��n��
        template <typename ClassType>
        class HookFunctionWithCallerAndRawParam : public HookFunctionBase 
        {
        public:
            typedef void(ClassType::* FuncType)( CallerType* caller, ParameterType* parameter );
            HookFunctionWithCallerAndRawParam(ClassType* object, FuncType func) : 
              m_object(object), m_func(func)
            {
            }

            virtual void operator()(HookParameterType* hookParameter)
            {
                (m_object->*m_func)( hookParameter->GetCaller(), hookParameter->GetParameter() );
            }
        protected:
            ClassType* m_object;
            FuncType m_func;
        };

        // ParameterType* ��n��
        template <typename ClassType>
        class HookFunctionWithRawParam : public HookFunctionBase 
        {
        public:
            typedef void(ClassType::* FuncType)( ParameterType* parameter );
            HookFunctionWithRawParam(ClassType* object, FuncType func) : 
              m_object(object), m_func(func)
            {
            }

            virtual void operator()(HookParameterType* hookParameter)
            {
                (m_object->*m_func)( hookParameter->GetParameter() );
            }
        protected:
            ClassType* m_object;
            FuncType m_func;
        };

        HookPoint()
        {
            m_hookExists = false;
        }

        virtual ~HookPoint()
        {
            ReleaseHookVector(m_beforeFunction);
            ReleaseHookVector(m_aroundFunction);
            ReleaseHookVector(m_afterFunction);
        }

        // OpIterator �� Caller �� Parameter ���󂯎���ă��C���̏����̑O�ɍs���ė~�����������g���K����֐�
        void Trigger( OpIterator opIterator, CallerType* caller, ParameterType* parameter, HookType::Type hookType)
        {
            if( !IsHookExists(hookType) )
                return;

            HookParameterType hookParameter( opIterator, caller, parameter );
            Trigger( GetHookFunction(hookType), &hookParameter );
        }

        // OpIterator �� Caller ���󂯎���ă��C���̏����̑O�ɍs���ė~�����������g���K����֐�
        void Trigger( OpIterator opIterator, CallerType* caller, HookType::Type hookType)
        {
            if( !IsHookExists(hookType) )
                return;
            Trigger( opIterator, caller, 0, hookType);
        }

        // Caller �� Parameter ���󂯎���ă��C���̏����̑O�ɍs���ė~�����������g���K����֐�
        void Trigger( CallerType* caller, ParameterType* parameter, HookType::Type hookType)
        {
            if( !IsHookExists(hookType) )
                return;

            HookParameterType hookParameter( OpIterator(0), caller, parameter );
            Trigger( GetHookFunction(hookType), &hookParameter );
        }

        // Caller ���󂯎���ă��C���̏����̑O�ɍs���ė~�����������g���K����֐�
        void Trigger( CallerType* caller, HookType::Type hookType)
        {
            if( !IsHookExists(hookType) )
                return;
            Trigger( OpIterator(0), caller, 0, hookType );
        }

        //
        // --- Hook ��o�^����֐�
        // HookFunction~ ��Hook ���郁�\�b�h�̌`�ɑΉ����Ă���
        // HookParameter ����K�؂�Caller/Op/Parameter ���擾���ēn���K�v�����邽�߁C
        // �e���v���[�g�ň�ɂ܂Ƃ߂�͓̂��
        //

        // �p�����[�^�Ȃ�
        template <typename T>
        void Register(T* object, typename HookFunction<T>::FuncType func, int priority, HookType::Type hookType = HookType::HOOK_AFTER)
        {
            Register(GetHookFunction(hookType), new HookFunction<T>(object, func), priority);
        }

        // �p�����[�^���� Hook ��o�^����֐�
        template <typename T>
        void Register(T* object, typename HookFunctionWithParam<T>::FuncType func, int priority, HookType::Type hookType = HookType::HOOK_AFTER)
        {
            Register(GetHookFunction(hookType), new HookFunctionWithParam<T>(object, func), priority);
        }

        // Caller/OpIterator/ParameterType ���󂯎��Hook ��o�^����֐�
        template <typename T>
        void Register(T* object, typename HookFunctionWithSeparateParam<T>::FuncType func, int priority, HookType::Type hookType = HookType::HOOK_AFTER)
        {
            Register(GetHookFunction(hookType), new HookFunctionWithSeparateParam<T>(object, func), priority);
        }

        // OpIterator ���󂯎��Hook ��o�^����֐�
        template <typename T>
        void Register(T* object, typename HookFunctionWithOp<T>::FuncType func, int priority, HookType::Type hookType = HookType::HOOK_AFTER)
        {
            Register(GetHookFunction(hookType), new HookFunctionWithOp<T>(object, func), priority);
        }

        // Caller ���󂯎��Hook ��o�^����֐�
        template <typename T>
        void Register(T* object, typename HookFunctionWithCaller<T>::FuncType func, int priority, HookType::Type hookType = HookType::HOOK_AFTER)
        {
            Register(GetHookFunction(hookType), new HookFunctionWithCaller<T>(object, func), priority);
        }

        // Caller/OpIterator ���󂯎��Hook ��o�^����֐�
        template <typename T>
        void Register(T* object, typename HookFunctionWithCallerAndOp<T>::FuncType func, int priority, HookType::Type hookType = HookType::HOOK_AFTER)
        {
            Register(GetHookFunction(hookType), new HookFunctionWithCallerAndOp<T>(object, func), priority);
        }

        // Caller/ParameterType* ��n��
        template <typename T>
        void Register(T* object, typename HookFunctionWithCallerAndRawParam<T>::FuncType func, int priority, HookType::Type hookType = HookType::HOOK_AFTER)
        {
            Register(GetHookFunction(hookType), new HookFunctionWithCallerAndRawParam<T>(object, func), priority);
        }

        // OpIterator/ParameterType* ��n��
        template <typename T>
        void Register(T* object, typename HookFunctionWithOpAndRawParam<T>::FuncType func, int priority, HookType::Type hookType = HookType::HOOK_AFTER)
        {
            Register(GetHookFunction(hookType), new HookFunctionWithOpAndRawParam<T>(object, func), priority);
        }

        // ParameterType* ��n��
        template <typename T>
        void Register(T* object, typename HookFunctionWithRawParam<T>::FuncType func, int priority, HookType::Type hookType = HookType::HOOK_AFTER)
        {
            Register(GetHookFunction(hookType), new HookFunctionWithRawParam<T>(object, func), priority);
        }

        // ���C���̏����̑���ɓo�^����Ă���֐������邩�ǂ���
        bool HasAround() 
        {
            return !m_aroundFunction.empty();
        }

        // ���C���̏����̑���ɓo�^����Ă���֐��̐�
        int CountAround()
        {
            return static_cast<int>(m_aroundFunction.size());
        }

        // ��ł��t�b�N���o�^����Ă��邩�ǂ���
        INLINE bool IsAnyHookRegistered()
        {
            return m_hookExists;
        }

    private:

        typedef std::pair< HookFunctionBase*, int > HookPair;
        typedef std::vector< HookPair > HookVector;

        struct ComparePriority
        {
            bool operator()( const HookPair& lhv, const HookPair& rhv )
            { return lhv.second < rhv.second; }
        };

        // HookFunctionBase* �� int(�D�揇��) �� pair ���i�[���� vector
        HookVector m_beforeFunction;
        HookVector m_aroundFunction;
        HookVector m_afterFunction;
        bool m_hookExists;


        void Register(HookVector& hookVector, HookFunctionBase* hookFunction, int priority)
        {
            // hook ��o�^
            hookVector.push_back( std::make_pair(hookFunction, priority) );
            m_hookExists = true;

            // �D�揇�ʂŃ\�[�g
            // �V�~�����[�^�̏��������ɌĂԂ��̂Ȃ̂Ŗ���\�[�g���閳�ʂ͋C�ɂ��Ȃ�
            sort(hookVector.begin(), hookVector.end(), ComparePriority());
        }

        // �o�^����Ă��� Hook �����ׂČĂԊ֐�
        void Trigger(HookVector& hookVector, HookParameterType* hookParameter)
        {
            for(typename HookVector::iterator iter = hookVector.begin();
                iter != hookVector.end();
                ++iter)
            {
                (*(iter->first))(hookParameter);
            }
        }

        void ReleaseHookVector(HookVector& hookVector)
        {
            for(typename HookVector::iterator iter = hookVector.begin();
                iter != hookVector.end();
                ++iter)
            {
                delete iter->first;
            }
            hookVector.clear();
        }

        HookVector& GetHookFunction(HookType::Type hookType)
        {
            if( hookType == HookType::HOOK_AFTER ) {
                return m_afterFunction;
            }
            if( hookType == HookType::HOOK_BEFORE ) {
                return m_beforeFunction;
            }
            return m_aroundFunction;
        }

        bool IsHookExists(HookType::Type hookType)
        {
            if( hookType == HookType::HOOK_AFTER ) {
                return m_afterFunction.size() > 0;
            }
            if( hookType == HookType::HOOK_BEFORE ) {
                return m_beforeFunction.size() > 0;
            }
            return m_aroundFunction.size() > 0;
        }
    };

}; // namespace Onikiri

#endif // SIM_FOUNDATION_HOOK_HOOK_H

