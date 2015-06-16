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


#ifndef __ALPHALINUX_ALPHA64DECODER_H__
#define __ALPHALINUX_ALPHA64DECODER_H__

#include "Emu/AlphaLinux/Alpha64Info.h"

namespace Onikiri {
    namespace AlphaLinux {

        class Alpha64Decoder
        {
        public:
            struct DecodedInsn
            {
                // ���l
                boost::array<u64, 2> Imm;
                // �I�y�����h�E���W�X�^(dest, src1..3)
                boost::array<int, 4> Reg;

                u32 CodeWord;

                DecodedInsn();
                void clear();
            };
        public:
            Alpha64Decoder();
            // ����codeWord���f�R�[�h���Cout�Ɋi�[����
            void Decode(u32 codeWord, DecodedInsn* out);
        private:

        };

    } // namespace AlphaLinux
} // namespace Onikiri

#endif
