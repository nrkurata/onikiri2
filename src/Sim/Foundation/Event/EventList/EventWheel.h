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
// Event wheel
//

#ifndef SIM_FOUNDATION_EVENT_EVENT_WHEEL_H
#define SIM_FOUNDATION_EVENT_EVENT_WHEEL_H

#include "Sim/Foundation/Event/EventList/EventList.h"

namespace Onikiri
{
#if 1

    // �i�C�[�u�Ȏ���
    // ListType �̔z��Ƃ��ē���
    class EventWheel
    {

    protected:
        typedef EventPtr  ListNode;
        typedef EventList ListType;

        // �e�T�C�N���p�̃C�x���g
        std::vector<ListType> m_event;

        // �m�ۂ���T�C�Y
        int m_size;

    public:

        EventWheel()
        {
            m_size = 0;
        }

        void Resize( int size )
        {
            m_size = size;
            m_event.resize( m_size, ListType() );
        }

        INLINE ListType* PeekEventList( int index )
        {
            return &m_event[index];
        }

        INLINE ListType* GetEventList( int index )
        {
            return &m_event[index];
        }

        INLINE void ReleaseEventList( ListType* list, int index )
        {
        }

    };

#elif 1

    // �t���[���X�g��p���ă������e�ʂ��팸
    class EventWheel
    {

    protected:
        typedef EventPtr  ListNode;
        typedef EventList ListType;

        // �e�T�C�N���p�̃C�x���g
        std::vector<ListType*> m_event;

        // �m�ۂ���T�C�Y
        int m_size;

        // �t���[���X�g
        class EventFreeList
        {

        public:
            pool_vector<ListType*> m_eventFreeList; // �|�C���^�̃t���[���X�g�i�X�^�b�N�j


            EventFreeList()
            {
                m_clean = true;
            }

            ~EventFreeList()
            {
                for( size_t i = 0; i < m_eventFreeList.size(); ++i ){
                    delete m_eventFreeList[i];
                }
                m_eventFreeList.clear();
            }

            EventFreeList( const EventFreeList& ref )
            {
                ASSERT( 0 );
            }

            EventFreeList& operator=( const EventFreeList &ref )
            {
                ASSERT( 0 );
                return(*this);
            }

            INLINE ListType* Allocate()
            {
                m_clean = false;
                if( m_eventFreeList.size() == 0 ){
                    return new ListType();
                }

                ListType* eventList = m_eventFreeList.back();
                m_eventFreeList.pop_back();
                return eventList;
            }

            INLINE void Free( ListType* eventList )
            {
                m_eventFreeList.push_back( eventList );
            }

        protected:
            bool m_clean;
        };

        EventFreeList m_eventFreeList;
        EventFreeList& FreeList()
        {
            return m_eventFreeList;
        }
        
    public:

        EventWheel( const EventWheel& ref )
        {
            ASSERT( 0 );
        }

        EventWheel& operator=( const EventWheel &ref )
        {
            ASSERT( 0 );
            return(*this);
        }

        EventWheel()
        {
            m_size = 0;
        }

        ~EventWheel()
        {
            for( size_t i = 0; i < m_event.size(); ++i ){
                if( m_event[i] ){
                    FreeList().Free( m_event[i] );
                    m_event[i] = NULL;
                }
            }
        }

        void Resize( int size )
        {
            m_size = size;
            m_event.resize( m_size, NULL );
        }

        // �|�C���^��Ԃ������ŁC�m�ۂ��s��Ȃ�
        INLINE ListType* PeekEventList( int index ) const
        {
            ASSERT( index < m_size, "The passed index is out of range." );
            return m_event[index];
        }

        // ���X�g�̎擾
        // �܂��m�ۂ���Ă��Ȃ��ꍇ�C���X�g�̗̈���m��
        INLINE ListType* GetEventList( int index )
        {
            ASSERT( index < m_size, "The passed index is out of range." );
            ListType* eventList = m_event[index];
            if( !eventList ){
                eventList = FreeList().Allocate();
                m_event[index] = eventList;
            }
            return eventList;
        }

        // ���X�g�̕ԋp
        INLINE void ReleaseEventList( ListType* list, int index )
        {
            ASSERT( m_event[index], "A not allocated list is released." );
            m_event[index] = NULL;
            FreeList().Free( list );
        }

    };

#elif 1

    // �n�b�V���g�p
    class EventWheel
    {

    protected:
        typedef EventPtr  ListNode;
        typedef EventList ListType;

        // �e�T�C�N���p�̃C�x���g
        class EventHash
        {
        public:
            size_t operator()(const int value) const
            {
                return (size_t)(value ^ (value >> 8));
            }
        };

        pool_unordered_map<int, ListType, EventHash> m_event;

        // �m�ۂ���T�C�Y
        int m_size;

    public:

        EventWheel()
        {
            m_size = 0;
        }

        ~EventWheel()
        {
        }
        void Resize( int size )
        {
            m_size = size;
        }

        // �|�C���^��Ԃ������ŁC�m�ۂ��s��Ȃ�
        INLINE ListType* PeekEventList( int index ) 
        {
            ASSERT( index < m_size, "The passed index is out of range." );
            pool_unordered_map<int, ListType, EventHash>::iterator i = m_event.find(index);
            return i == m_event.end() ? NULL : &i->second;
        }

        // ���X�g�̎擾
        // �܂��m�ۂ���Ă��Ȃ��ꍇ�C���X�g�̗̈���m��
        INLINE ListType* GetEventList( int index )
        {
            ASSERT( index < m_size, "The passed index is out of range." );
            pool_unordered_map<int, ListType, EventHash>::iterator i = m_event.find(index);
            if( i == m_event.end() ){
                m_event[index] = EventList();
                return &m_event[index];
            }
            return &i->second;
        }

        // ���X�g�̕ԋp
        INLINE void ReleaseEventList( ListType* list, int index )
        {
            m_event.erase(index);
        }

    };
#endif
}


#endif

