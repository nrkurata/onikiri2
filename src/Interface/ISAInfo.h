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
// ISA information
// 


#ifndef INTERFACE_ISA_INFO_H
#define INTERFACE_ISA_INFO_H

namespace Onikiri
{
<<<<<<< HEAD
	// Instruction Set Architecture Type
	enum ISA_TYPE
	{
		ISA_ALPHA,	// Alpha
		ISA_PPC64,	// PowerPC 64bit
		ISA_AARCH64,// ARMv8 64bit
	};
=======
    // Instruction Set Architecture Type
    enum ISA_TYPE
    {
        ISA_ALPHA,  // Alpha
        ISA_PPC64,  // PowerPC 64bit
    };
>>>>>>> master

    class ISAInfoIF
    {
    public:
        virtual ~ISAInfoIF()
        {
        };

        virtual ISA_TYPE GetISAType() = 0;

        virtual int GetInstructionWordBitSize() = 0;
        virtual int GetRegisterWordBitSize() = 0;
        virtual int GetAddressSpaceBitSize() = 0;

        // �_�����W�X�^�̐��𓾂�
        virtual int GetRegisterCount() = 0;
        virtual int GetMaxSrcRegCount() = 0;
        virtual int GetMaxDstRegCount() = 0;

        // �_�����W�X�^ regNum �̃��W�X�^�Z�O�����g (�����������W�X�^�Z�b�g���g�p����_�����W�X�^�̏W��) ID�𓾂�
        virtual int GetRegisterSegmentID(int regNum) = 0;
    
        // �_�����W�X�^�̃Z�O�����g����Ԃ�
        virtual int GetRegisterSegmentCount() = 0;

        // GetOp��������1��PC�ɑ΂��Đ��������OpInfo�̍ő吔 
        virtual int GetMaxOpInfoCountPerPC() = 0;

        // Endian
        virtual bool IsLittleEndian() = 0;

        // The maximum width(bytes) of memory accesses.
        virtual int GetMaxMemoryAccessByteSize() = 0;
    };
}

#endif

