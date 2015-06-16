#include <pch.h>

#include "SysDeps/Endian.h"
#include "Emu/Utility/DecoderUtility.h"
#include "Emu/AArch64Linux/AArch64Decoder.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::AArch64Linux;

namespace {
	// –½—ß‚ÌŽí—Þ
	enum InsnType {
		UNDEF,				// Undefined or reserved
		OPERATION_INT,		// Integer
		FP_SIMD_CRYPTO,		// SIMD, floating point and crypto
		MEMORY_ADDR,		// Memory address calculation
		MEMORY_LOAD_STORE,	// Load and store
		BRANCH_EXCEPT,		// Branch and exception
		SYSTEM,				// System instruction
	};

	const InsnType UND = UNDEF;
	const InsnType OPI = OPERATION_INT;
	const InsnType OPF = FP_SIMD_CRYPTO;
	const InsnType MEMA = MEMORY_ADDR;
	const InsnType LDST = MEMORY_LOAD_STORE;
	const InsnType BRX = BRANCH_EXCEPT;
	const InsnType SYS = SYSTEM;

	// –½—ßƒR[ƒh‚ÆŽí—Þ‚Ì‘Î‰ž
	InsnType OpCodeToInsnType[16] =
	{
		// 0x0
		MEMA, OPI, OPI, OPI, BRX, SYS, BRX, BRX,
		// 0x8
		LDST, LDST, OPI, OPI, LDST, LDST, OPF, OPF, 
	};
}


AArch64Decoder::DecodedInsn::DecodedInsn()
{
	clear();
}

void AArch64Decoder::DecodedInsn::clear()
{
	CodeWord = 0;

	std::fill(Imm.begin(), Imm.end(), 0);
	std::fill(Reg.begin(), Reg.end(), -1);
}


AArch64Decoder::AArch64Decoder()
{
}

void AArch64Decoder::Decode(u32 codeWord, DecodedInsn* out)
{
	out->clear();

	u32 opcode = ExtractBits(codeWord, 24, 4);
	out->CodeWord = codeWord;

	InsnType type = OpCodeToInsnType[opcode];

	switch (type) {
	case OPI:
	case MEMA:
		DecodeInt(codeWord, out);
		break;
	case OPF:
		ASSERT(0, "FP/SIMD instructions are not supported yet.");
		break;
	case LDST:
		DecodeLoadStore(codeWord, out);
		break;
	case BRX:
	case SYS:
		DecodeBranch(codeWord, out);
		break;
	default:
		ASSERT(0);	// never reached
		break;
	}

}

void AArch64Decoder::DecodeInt( u32 codeWord, DecodedInsn* out )
{
	if (ExtractBits(codeWord, 27, 1))
	{
		// register
		if (ExtractBits(codeWord, 28, 1))
		{
			// non-shifted
			u32 opcode = ExtractBits(codeWord, 22, 3);
			switch (opcode)
			{
			case 0:
				// Add/subtract (with carry)
				out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
				out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rm
				out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
				out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rd
				break;
			case 1:
				// Conditional compare
				if (ExtractBits(codeWord, 11, 1))
				{
					// (immediate)
					out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
					out->Imm[1] = ExtractBits<u64>(codeWord, 16, 5); // imm5
					out->Imm[2] = ExtractBits<u64>(codeWord, 12, 4); // cond
					out->Imm[3] = ExtractBits<u64>(codeWord, 12, 4); // nzcv
					out->Reg[0] = ExtractBits(codeWord, 5, 5); // Rn
				}
				else
				{
					// (register)
					out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
					out->Imm[1] = ExtractBits<u64>(codeWord, 12, 4); // cond
					out->Imm[2] = ExtractBits<u64>(codeWord, 12, 4); // nzcv
					out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rm
					out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
				}
				break;
			case 2:
				// Conditional select
				out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
				out->Imm[1] = ExtractBits<u64>(codeWord, 12, 4); // cond
				out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rm
				out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
				out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rd
				break;
			case 3:
				// Data-processing
				if (ExtractBits(codeWord, 30, 1))
				{
					// (1 source)
					out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
					out->Reg[0] = ExtractBits(codeWord, 5, 5); // Rn
					out->Reg[1] = ExtractBits(codeWord, 0, 5); // Rd
				}
				else
				{
					// (2 sources)
					out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
					out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rn
					out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
					out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rd
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
				// Data-processing (3 sources)
				out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
				out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rn
				out->Reg[1] = ExtractBits(codeWord, 10, 5); // Ra
				out->Reg[2] = ExtractBits(codeWord, 5, 5); // Rn
				out->Reg[3] = ExtractBits(codeWord, 0, 5); // Rd
				break;
			default:
				ASSERT(0);	// never reached
				break;
			}
		}
		else
		{
			// shifted
			if (ExtractBits(codeWord, 24, 1))
			{
				// Add/subtract
				if (ExtractBits(codeWord, 21, 1))
				{
					// (extended register)
					out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
					out->Imm[1] = ExtractBits<u64>(codeWord, 13, 3); // option
					out->Imm[2] = ExtractBits<u64>(codeWord, 10, 3); // imm3
					out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rm
					out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
					out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rd
				}
				else
				{
					// (shifted register)
					out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
					out->Imm[1] = ExtractBits<u64>(codeWord, 22, 2); // shift
					out->Imm[2] = ExtractBits<u64>(codeWord, 10, 6); // imm6
					out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rm
					out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
					out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rd
				}
			}
			else
			{
				// Logical (shifted register)
				out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
				out->Imm[1] = ExtractBits<u64>(codeWord, 22, 2); // shift
				out->Imm[2] = ExtractBits<u64>(codeWord, 10, 6); // imm6
				out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rm
				out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
				out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rd
			}
		}
	}
	else {
		// immediate
		u32 opcode = ExtractBits(codeWord, 23, 3);
		switch (opcode)
		{
		case 0:
		case 1:
			// PC-rel. addressing
			out->Imm[0] = ExtractBits<u64>(codeWord, 29, 2); // immlo
			out->Imm[1] = ExtractBits<u64>(codeWord, 5, 19); // immhi
			out->Reg[0] = ExtractBits(codeWord, 0, 5); // Rd
			break;
		case 2:
		case 3:
			// Add/subtract (immediate)
			out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
			out->Imm[1] = ExtractBits<u64>(codeWord, 22, 2); // shift
			out->Imm[2] = ExtractBits<u64>(codeWord, 10, 12); // imm12
			out->Reg[0] = ExtractBits(codeWord, 5, 5); // Rn
			out->Reg[1] = ExtractBits(codeWord, 0, 5); // Rd
			break;
		case 4:
			// Logical (immediate)
		case 6:
			// Bitfield
			out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
			out->Imm[1] = ExtractBits<u64>(codeWord, 16, 6); // immr
			out->Imm[2] = ExtractBits<u64>(codeWord, 10, 6); // imms
			out->Reg[0] = ExtractBits(codeWord, 5, 5); // Rn
			out->Reg[1] = ExtractBits(codeWord, 0, 5); // Rd
			break;
		case 5:
			// Move wide (immediate)
			out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
			out->Imm[1] = ExtractBits<u64>(codeWord, 21, 2); // hw
			out->Imm[2] = ExtractBits<u64>(codeWord, 5, 16); // imm16
			out->Reg[0] = ExtractBits(codeWord, 0, 5); // Rd
			break;
		case 7:
			// Extract
			out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
			out->Imm[1] = ExtractBits<u64>(codeWord, 10, 6); // imms
			out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rm
			out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
			out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rd
			break;
		default:
			ASSERT(0);	// never reached
			break;
		}

	}
}

void AArch64Decoder::DecodeLoadStore(u32 codeWord, DecodedInsn* out)
{
	u32 opcode = ExtractBits(codeWord, 28, 2);
	u32 exop;
	switch (opcode){
	case 0x0:
		u32 exop = ExtractBits(codeWord, 26, 1);
		if (exop)
		{
			// Advanced SIMD Load/store
		}
		else
		{
			// Load/store exclusive
			out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rs
			out->Reg[1] = ExtractBits(codeWord, 10, 5); // Rt2
			out->Reg[2] = ExtractBits(codeWord, 5, 5); // Rn
			out->Reg[3] = ExtractBits(codeWord, 0, 5); // Rt
		}
		break;
	case 0x1:
		// Load register (literal)
		out->Reg[0] = ExtractBits(codeWord, 0, 5); // Rt(==Rd)
		out->Imm[0] = ExtractBits<u64>(codeWord, 5, 19, true); // imm19
		break;
	case 0x2:
		u32 exop = ExtractBits(codeWord, 23, 2);
		switch (exop){
		case 0x0:
			// Load/store no-allocate pair (offset)
			out->Imm[0] = ExtractBits<u64>(codeWord, 15, 7); // imm7
			out->Reg[0] = ExtractBits(codeWord, 10, 5); // Rt2
			out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
			out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rt
			break;
		case 0x1:
			// Load/store register pair (post-indexed)
		case 0x3:
			// Load/store register pair (pre-indexed)
			out->Imm[0] = ExtractBits<u64>(codeWord, 15, 7); // imm7
			out->Reg[0] = ExtractBits(codeWord, 10, 5); // Rt2
			out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
			out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rt

			break;
		case 0x2:
			// Load/store register pair (offset)
			out->Imm[0] = ExtractBits<u64>(codeWord, 15, 7); // imm7
			out->Reg[0] = ExtractBits(codeWord, 10, 5); // Rt2
			out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
			out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rt
			break;
		default:
			ASSERT(0);	// never reached
			break;
		}
		break;
	case 0x3:
		if (ExtractBits(codeWord, 24, 1))
		{
			// Load/store register (unsigned immediate)
			out->Imm[0] = ExtractBits<u64>(codeWord, 10, 12); // imm12
			out->Reg[0] = ExtractBits(codeWord, 5, 5); // Rn
			out->Reg[1] = ExtractBits(codeWord, 0, 5); // Rt
		}
		else
		{
			u32 exop = ExtractBits(codeWord, 10, 2);
			switch (exop){
			case 0x0:
				// Load/store register (unscaled immediate)
				out->Imm[0] = ExtractBits<u64>(codeWord, 12, 9); // imm9
				out->Reg[0] = ExtractBits(codeWord, 5, 5); // Rn
				out->Reg[1] = ExtractBits(codeWord, 0, 5); // Rt
				break;
			case 0x1:
				// Load/store register (immediate post-indexed)
			case 0x3:
				// Load/store register (immediate pre-indexed)
				out->Imm[0] = ExtractBits<u64>(codeWord, 12, 9, true); // imm9
				out->Reg[0] = ExtractBits(codeWord, 5, 5); // Rn
				out->Reg[1] = ExtractBits(codeWord, 0, 5); // Rt
				break;
			case 0x2:
				if (ExtractBits(codeWord, 21, 1))
				{
					// Load/store register (register offset)
					out->Imm[0] = ExtractBits<u64>(codeWord, 13, 3); // option
					out->Imm[1] = ExtractBits<u64>(codeWord, 12, 1); // S
					out->Reg[0] = ExtractBits(codeWord, 16, 5); // Rm
					out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
					out->Reg[2] = ExtractBits(codeWord, 0, 5); // Rt
				}
				else
				{
					// Load/store register (unprivileged)
					out->Imm[0] = ExtractBits<u64>(codeWord, 12, 9); // imm9
					out->Reg[0] = ExtractBits(codeWord, 5, 5); // Rn
					out->Reg[1] = ExtractBits(codeWord, 0, 5); // Rt
				}
				break;
			default:
				ASSERT(0);	// never reached
				break;
			}
		}
		break;
	default:
		ASSERT(0);	// never reached
		break;
	}
}

void AArch64Decoder::DecodeBranch( u32 codeWord, DecodedInsn* out )
{
	u32 opcode = ExtractBits(codeWord, 29, 3);
	u32 exop;
	switch(opcode){
	case 0x0:
	case 0x4:
		// Unconditional branch (immediate)
		out->Imm[0] = ExtractBits<u64>(codeWord, 0, 26, true); // imm26, sign extended
		break;
	case 0x1:
	case 0x5:
		exop = ExtractBits(codeWord, 25, 1);
		if (exop)
		{
			// Test & branch
			out->Reg[0] = ExtractBits(codeWord, 0, 5); // Rt(==Rd)
			out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // b5
			out->Imm[1] = ExtractBits<u64>(codeWord, 19, 5); // b40
			out->Imm[2] = ExtractBits<u64>(codeWord, 5, 14, true); // imm14, sign extended
		}
		else
		{
			// Compare & branch
			out->Reg[0] = ExtractBits(codeWord, 0, 5); // Rt(==Rd)
			out->Imm[0] = ExtractBits<u64>(codeWord, 31, 1); // sf
			out->Imm[1] = ExtractBits<u64>(codeWord, 5, 19); // imm19
		}
		break;
	case 0x2:
		// Conditional branch
		out->Imm[0] = ExtractBits<u64>(codeWord, 5, 19); // imm19
		out->Imm[1] = ExtractBits<u64>(codeWord, 0, 4); // cond
		break;
	case 0x6:
		exop = ExtractBits(codeWord, 24, 2);
		switch (exop)
		{
		case 0x0:
			// Exception
			out->Imm[0] = ExtractBits<u64>(codeWord, 5, 16); // imm16
			break;
		case 0x1:
			// System
			out->Reg[0] = ExtractBits(codeWord, 0, 5); // Rt
			break;
		case 0x2:
		case 0x3:
			// Unconditional branch (register)
			out->Reg[0] = ExtractBits(codeWord, 5, 5); // Rn
			break;
		default:
			ASSERT(0);	// never reached
			break;
		}
		break;
	default:
		ASSERT(0);	// never reached
		break;
	}
}
