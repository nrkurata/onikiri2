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


#ifndef INTERFACE_ADDR_H
#define INTERFACE_ADDR_H

#include "Types.h"
#include "Interface/ResourceIF.h"


namespace Onikiri 
{
    static const u64 ADDR_INVALID = ~((u64)0);

    struct Addr : public LogicalData
    {
        u64 address;

        Addr() : address( ADDR_INVALID ) 
        {
        }

        Addr( int pid, int tid, u64 address ) : 
            LogicalData( pid, tid ),
            address( address )
        {
        }

        // Addr
        bool operator==(const Addr& rhv) const
        {
            // Should not compare tid
            return pid == rhv.pid && address == rhv.address;
        }

        bool operator!=(const Addr& rhv) const
        {
            return !(*this == rhv);
        }

        bool operator<(const Addr& rhv) const
        {
            // Should not compare TIDs
            if( pid == rhv.pid )
                return address < rhv.address;
            return pid < rhv.pid;
        }

        // For debug
        const std::string ToString() const;
    };
    
    typedef Addr PC;

}; // namespace Onikiri

#endif // __ADDR_H__

