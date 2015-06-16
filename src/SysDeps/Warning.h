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
// Environmentally dependent warnings
//

#ifndef SYSDEPS_WARNING_H
#define SYSDEPS_WARNING_H

#include "SysDeps/host_type.h"

#ifdef COMPILER_IS_MSVC

#pragma warning( disable: 4100 )    // �����͊֐��̖{�̕��� 1 �x���Q�Ƃ���܂���
#pragma warning( disable: 4127 )    // ���������萔�ł�
#pragma warning( disable: 4512 )    // ������Z�q�𐶐��ł��܂���
#pragma warning( disable: 4702 )    // ���䂪�n��Ȃ��R�[�h�ł�
#pragma warning( disable: 4714 )    // �C�����C���֐��ł͂Ȃ��A__forceinline �Ƃ��ċL�q����Ă��܂�


// �ȉ��̌x����C++�W���ʂ�̓�����s�����Ƃ������Ă���ɂ����Ȃ��̂�disable
#pragma warning( disable: 4345 )    // POD�^�̃f�t�H���g�R���X�g���N�^�Ɋւ���x��

#endif


#endif
