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


#ifndef SIM_OP_OP_STATUS_H
#define SIM_OP_OP_STATUS_H


namespace Onikiri
{

    class OpStatus
    {
    public:

        // When you change the Status members, you need to add corresponding
        // members to ToString().
        enum Status 
        {
            OS_INVALID = 0,     //
            OS_FLUSHED,         // ����\���~�X�Ńt���b�V�����ꂽ
            OS_FETCH,           // Fetcher��nextPipeline��ɂ�����
            OS_RENAME,          // Renamer��nextPipeline��ɂ�����
            OS_DISPATCHING,     // Dispatcher��nextPipeline��ɂ�����
            OS_DISPATCHED,      // �X�P�W���[���ɓ�����
            OS_ISSUING,         // issue���ꂽ
            OS_EXECUTING,       // ���s���ꂽ
            OS_FINISHED,        // ���s���I������
            OS_WRITING_BACK,    // ���C�g�o�b�N��
            OS_WRITTEN_BACK,    // ���C�g�o�b�N���ꂽ
            OS_COMPLETED,       // ���^�C�A��������X�e�[�^�X�̃I�v�V�����Ƃ��ėp��
            OS_NOP,             // NOP��Dispatcher�ɑ��炸�Ɏ��s�I����Ԃɂ���
            OS_COMITTING,       // in committing
            OS_COMITTED,        // after commit
            OS_RETIRED,         // ���^�C�A����
            OS_MAX
        };

        OpStatus() : 
            m_status( OS_INVALID )
        {
        }

        OpStatus( const OpStatus& status ) : 
            m_status( status.m_status )
        {
        }
        
        OpStatus( const Status& status ) :
            m_status( status )
        {
        }


        bool operator == ( const Status& rhs )  const 
        {   return m_status == rhs; }

        bool operator != ( const Status& rhs )  const 
        {   return m_status != rhs; }

        bool operator < ( const Status& rhs )   const 
        {   return m_status <  rhs; }

        bool operator <= ( const Status& rhs )  const  
        {   return m_status <= rhs; }

        bool operator > ( const Status& rhs )   const  
        {   return m_status > rhs;      }

        bool operator >= ( const Status& rhs )  const  
        {   return m_status >= rhs; }


        bool operator == ( const OpStatus& rhs )    const 
        {   return m_status == rhs.m_status;    }
            
        bool operator != ( const OpStatus& rhs )    const 
        {   return m_status != rhs.m_status;    }
            
        bool operator < ( const OpStatus& rhs )     const 
        {   return m_status <  rhs.m_status;    }

        bool operator <= ( const OpStatus& rhs )    const  
        {   return m_status <= rhs.m_status;    }
            
        bool operator > ( const OpStatus& rhs )     const  
        {   return m_status > rhs.m_status;     }
            
        bool operator >= ( const OpStatus& rhs )    const  
        {   return m_status >= rhs.m_status;    }

        const char* ToString();

    protected:
        Status m_status;
    };

    

}; // namespace Onikiri

#endif // SIM_OP_OP_STATUS_H

