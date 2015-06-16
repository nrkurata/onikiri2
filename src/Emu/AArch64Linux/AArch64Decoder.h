#ifndef __AARCH64LINUX_AARCH64DECODER_H__
#define __AARCH64LINUX_AARCH64DECODER_H__

#include "Emu/AArch64Linux/AArch64Info.h"

namespace Onikiri {
	namespace AArch64Linux {

		class AArch64Decoder
		{
		public:
			struct DecodedInsn
			{
				// ���l
				boost::array<u64, 3> Imm;
				// �I�y�����h�E���W�X�^
				boost::array<int, 5> Reg;

				u32 CodeWord;

				DecodedInsn();
				void clear();
			};

			AArch64Decoder();
			// ����codeWord���f�R�[�h���Cout�Ɋi�[����
			void Decode(u32 codeWord, DecodedInsn* out);
		private:
			void DecodeInt(u32 codeWord, DecodedInsn* out);
			void DecodeFP(u32 codeWord, DecodedInsn* out);
			void DecodeMemAddress(u32 codeWord, DecodedInsn* out);
			void DecodeLoadStore(u32 codeWord, DecodedInsn* out);
			void DecodeBranch(u32 codeWord, DecodedInsn* out);
			void DecodeSystem(u32 codeWord, DecodedInsn* out);
		};
	}
}

#endif
