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
// ���߂̎�ނ�����킷�N���X
// OpClassCode �̒l����OpClass �̋����͈�ӂɒ�܂邽�߁C
// �C���^�[�t�F�[�X�̈ꕔ�Ƃ��Ď���
//

#ifndef __OPCLASS_H__
#define __OPCLASS_H__

#include <string>
#include "Types.h"
#include "SysDeps/Inline.h"
#include "Interface/OpClassCode.h"

namespace Onikiri 
{

    class OpClass
    {
        OpClassCode::OpClassCode m_code;
        int m_opClassBits;

        static const int MASK_BRANCH                = 1 << 0;
        static const int MASK_CONDITIONAL_BRANCH    = 1 << 1;
        static const int MASK_UNCONDITIONAL_BRANCH  = 1 << 2;
        static const int MASK_INDIRECT_JUMP         = 1 << 3;
        static const int MASK_CALL                  = 1 << 4;
        static const int MASK_RETURN                = 1 << 5;
        static const int MASK_SUBROUTINE            = 1 << 6;
        static const int MASK_MEM       = 1 << 7;
        static const int MASK_LOAD      = 1 << 8;
        static const int MASK_STORE     = 1 << 9;
        static const int MASK_ADDR      = 1 << 10;
        static const int MASK_INT       = 1 << 11;
        static const int MASK_FLOAT     = 1 << 12;
        static const int MASK_IFCONV    = 1 << 13;
        static const int MASK_SYSCALL   = 1 << 14;
        static const int MASK_NOP       = 1 << 15;

        INLINE bool BitTest( int mask ) const
        {
            return m_opClassBits & mask ? true : false;
        }

    public:
        OpClass( OpClassCode::OpClassCode code ) : m_code(code)
        {
            m_opClassBits = 0;
            m_opClassBits |= OpClassCode::IsBranch( code )              ? MASK_BRANCH : 0;
            m_opClassBits |= OpClassCode::IsConditionalBranch( code )   ? MASK_CONDITIONAL_BRANCH : 0;
            m_opClassBits |= OpClassCode::IsUnconditionalBranch( code ) ? MASK_UNCONDITIONAL_BRANCH : 0;
            m_opClassBits |= OpClassCode::IsIndirectJump( code )        ? MASK_INDIRECT_JUMP : 0;
            m_opClassBits |= OpClassCode::IsCall( code )                ? MASK_CALL      : 0;
            m_opClassBits |= OpClassCode::IsReturn( code )              ? MASK_RETURN    : 0;
            m_opClassBits |= OpClassCode::IsSubroutine( code )          ? MASK_SUBROUTINE : 0;
            m_opClassBits |= OpClassCode::IsMem( code )                 ? MASK_MEM      : 0;
            m_opClassBits |= OpClassCode::IsLoad( code )                ? MASK_LOAD     : 0;
            m_opClassBits |= OpClassCode::IsStore( code )               ? MASK_STORE    : 0;
            m_opClassBits |= OpClassCode::IsAddr( code )                ? MASK_ADDR     : 0;
            m_opClassBits |= OpClassCode::IsInt( code )                 ? MASK_INT      : 0;
            m_opClassBits |= OpClassCode::IsFloat( code )               ? MASK_FLOAT    : 0;
            m_opClassBits |= OpClassCode::IsIFConversion( code )        ? MASK_IFCONV   : 0;
            m_opClassBits |= OpClassCode::IsSyscall( code )             ? MASK_SYSCALL  : 0;
            m_opClassBits |= OpClassCode::IsNop( code )                 ? MASK_NOP      : 0;
        }

        ~OpClass() {}

        // ���߂̎�ނ�A�ԂŕԂ�
        OpClassCode::OpClassCode GetCode() const
        {
            return m_code;
        }

        // ����ijump���܂ށj���ǂ���
        bool IsBranch() const
        {   return BitTest( MASK_BRANCH );  }

        // Note: ����Ɋւ��āCConditional, IndirectJump, Call �͔r���ł͂Ȃ�
        //       Return��Call/Jump�͔r�������CConditional Return�͑��݂���
        // �������򂩂ǂ���
        bool IsConditionalBranch()  const   
        {   return BitTest( MASK_CONDITIONAL_BRANCH );      }
        
        bool IsUnconditionalBranch() const  
        {   return BitTest( MASK_UNCONDITIONAL_BRANCH );    }

        // jump ���ǂ���
        bool IsIndirectJump()   const       
        {   return BitTest( MASK_INDIRECT_JUMP );           }

        // call
        bool IsCall() const 
        {   return BitTest( MASK_CALL );        }

        // return
        bool IsReturn()     const 
        {   return BitTest( MASK_RETURN );      }

        bool IsSubroutine() const
        {   return BitTest( MASK_SUBROUTINE );  }

        // ������
        bool IsMem() const  
        {   return BitTest( MASK_MEM );     }

        // ���[�h
        bool IsLoad() const
        {   return BitTest( MASK_LOAD );    }

        // �X�g�A
        bool IsStore() const
        {   return BitTest( MASK_STORE );   }

        // ���[�h
        bool IsAddr() const
        {   return BitTest( MASK_ADDR );    }

        // ����
        bool IsInt() const 
        {   return BitTest( MASK_INT );     }

        // ���������_��
        bool IsFloat() const 
        {   return BitTest( MASK_FLOAT );   }

        // ���� <-> ���������_���ϊ�
        bool IsIFConversion() const 
        {   return BitTest( MASK_IFCONV );  }

        // �V�X�e���R�[���͒P�̂Ŏ��s����B���Ȃ킿�A
        // �V�X�e���R�[�������s����O�ɏ㗬�̖��߂����ׂă��^�C�A�����āA
        // �V�X�e���R�[�������^�C�A����܂ŉ������t�F�b�`���Ȃ�
        bool IsSyscall() const
        {   return BitTest( MASK_SYSCALL ); }

        // NOP
        bool IsNop() const  
        {   return BitTest( MASK_NOP );     }


        static int GetMaxCode() 
        {
            return OpClassCode::Code_MAX;
        }

        // �f�o�b�O�̂��߂ɕ�����ɂ���
        const std::string ToString() const
        {
            return OpClassCode::ToString( m_code );
        }
    };

}; // namespace Onikiri

#endif // __OPCLASS_H__

