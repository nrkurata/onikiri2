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


#ifndef __PPC64LINUX_PPC64DECODER_H__
#define __PPC64LINUX_PPC64DECODER_H__

#include "Emu/PPC64Linux/PPC64Info.h"

namespace Onikiri {
    namespace PPC64Linux {

        class PPC64Decoder
        {
        public:
            static const int GP_REG0 = 0;
            static const int FP_REG0 = 32;
            static const int COND_REG0 = 64;
            static const int LINK_REG = 72;
            static const int COUNT_REG = 73;
            static const int XER_REG = 74;
            static const int FPSCR_REG = 75;
            static const int ADDRESS_REG = 76;

            struct DecodedInsn
            {
                // ���l (MSB���猩�Ė��ߒ��Ɍ���鏇�D������ CR Bit Index �͍ŏ��j
                boost::array<u64, 4> Imm;
                // �I�y�����h�E���W�X�^ (dst src��ʂ���MSB���猩�Ė��ߒ��Ɍ���鏇)
                boost::array<int, 4> Reg;
                u32 CodeWord;

                DecodedInsn();
                void clear();
            };
        public:
            PPC64Decoder();

            // ����codeWord���f�R�[�h���Cout�Ɋi�[����
            void Decode(u32 codeWord, DecodedInsn* out);
        private:
        };

    } // namespace PPC64Linux
} // namespace Onikiri

#endif
