#ifndef __ARMLINUX_ARMDECODER_H__
#define __ARMLINUX_ARMDECODER_H__

#include "AArch64Info.h"

namespace Onikiri {
	namespace AArch64Linux {

		class AArch64Decoder
		{
		public:
			struct DecodedInsn
			{
				// 即値
				boost::array<u64, 3> Imm;
				// オペランド・レジスタ
				boost::array<int, 5> Reg;

				u32 CodeWord;

				DecodedInsn();
				void clear();
			};

			AArch64Decoder();
			// 命令codeWordをデコードし，outに格納する
			void Decode(u32 codeWord, DecodedInsn* out);
		private:
			void DecodeInt(u32 codeWord, u32 opcode, DecodedInsn* out);
			void DecodeFP(u32 codeWord, DecodedInsn* out);
			void DecodeMemAddress(u32 codeWord, DecodedInsn* out);
			void DecodeLoadStore(u32 codeWord, DecodedInsn* out);
			void DecodeBranch(u32 codeWord, DecodedInsn* out);
			void DecodeSystem(u32 codeWord, DecodedInsn* out);
		};
	}
}

#endif
