#include <pch.h>

#include "../../SysDeps/Endian.h"
#include "../Utility/DecoderUtility.h"
#include "AArch64Decoder.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::AArch64Linux;

namespace {
	/*
		NOTE: Implementation below is copied from GNU Binutils.
		You must re-implement when you release AArch64 to avoid GPL.
	*/
	/* Instruction fields.
   Keep synced with fields.
   src/opcodes/aarch64-opc.h
   */
	enum aarch64_field_kind
	{
		FLD_NIL,
		FLD_cond2,
		FLD_nzcv,
		FLD_defgh,
		FLD_abc,
		FLD_imm19,
		FLD_immhi,
		FLD_immlo,
		FLD_size,
		FLD_vldst_size,
		FLD_op,
		FLD_Q,
		FLD_Rt,
		FLD_Rd,
		FLD_Rn,
		FLD_Rt2,
		FLD_Ra,
		FLD_op2,
		FLD_CRm,
		FLD_CRn,
		FLD_op1,
		FLD_op0,
		FLD_imm3,
		FLD_cond,
		FLD_opcode,
		FLD_cmode,
		FLD_asisdlso_opcode,
		FLD_len,
		FLD_Rm,
		FLD_Rs,
		FLD_option,
		FLD_S,
		FLD_hw,
		FLD_opc,
		FLD_opc1,
		FLD_shift,
		FLD_type,
		FLD_ldst_size,
		FLD_imm6,
		FLD_imm4,
		FLD_imm5,
		FLD_imm7,
		FLD_imm8,
		FLD_imm9,
		FLD_imm12,
		FLD_imm14,
		FLD_imm16,
		FLD_imm26,
		FLD_imms,
		FLD_immr,
		FLD_immb,
		FLD_immh,
		FLD_N,
		FLD_index,
		FLD_index2,
		FLD_sf,
		FLD_H,
		FLD_L,
		FLD_M,
		FLD_b5,
		FLD_b40,
		FLD_scale,
	};

	/* Field description.  
	src/opcodes/aarch64-opc.c
	*/
	struct aarch64_field
	{
		int lsb;
		int width;
	};
	const aarch64_field fields[] =
	{
		{  0,  0 },	/* NIL.  */
		{  0,  4 },	/* cond2: condition in truly conditional-executed inst.  */
		{  0,  4 },	/* nzcv: flag bit specifier, encoded in the "nzcv" field.  */
		//{  5,  5 },	/* defgh: d:e:f:g:h bits in AdvSIMD modified immediate.  */
		//{ 16,  3 },	/* abc: a:b:c bits in AdvSIMD modified immediate.  */
		{  5, 19 },	/* imm19: e.g. in CBZ.  */
		{  5, 19 },	/* immhi: e.g. in ADRP.  */
		{ 29,  2 },	/* immlo: e.g. in ADRP.  */
		//{ 22,  2 },	/* size: in most AdvSIMD and floating-point instructions.  */
		//{ 10,  2 },	/* vldst_size: size field in the AdvSIMD load/store inst.  */
		//{ 29,  1 },	/* op: in AdvSIMD modified immediate instructions.  */
		//{ 30,  1 },	/* Q: in most AdvSIMD instructions.  */
		{  0,  5 },	/* Rt: in load/store instructions.  */
		{  0,  5 },	/* Rd: in many integer instructions.  */
		{  5,  5 },	/* Rn: in many integer instructions.  */
		{ 10,  5 },	/* Rt2: in load/store pair instructions.  */
		{ 10,  5 },	/* Ra: in fp instructions.  */
		//{  5,  3 },	/* op2: in the system instructions.  */
		//{  8,  4 },	/* CRm: in the system instructions.  */
		//{ 12,  4 },	/* CRn: in the system instructions.  */
		//{ 16,  3 },	/* op1: in the system instructions.  */
		//{ 19,  2 },	/* op0: in the system instructions.  */
		{ 10,  3 },	/* imm3: in add/sub extended reg instructions.  */
		{ 12,  4 },	/* cond: condition flags as a source operand.  */
		//{ 12,  4 },	/* opcode: in advsimd load/store instructions.  */
		//{ 12,  4 },	/* cmode: in advsimd modified immediate instructions.  */
		//{ 13,  3 },	/* asisdlso_opcode: opcode in advsimd ld/st single element.  */
		//{ 13,  2 },	/* len: in advsimd tbl/tbx instructions.  */
		{ 16,  5 },	/* Rm: in ld/st reg offset and some integer inst.  */
		{ 16,  5 },	/* Rs: in load/store exclusive instructions.  */
		{ 13,  3 },	/* option: in ld/st reg offset + add/sub extended reg inst.  */
		{ 12,  1 },	/* S: in load/store reg offset instructions.  */
		{ 21,  2 },	/* hw: in move wide constant instructions.  */
		{ 22,  2 },	/* opc: in load/store reg offset instructions.  */
		{ 23,  1 },	/* opc1: in load/store reg offset instructions.  */
		{ 22,  2 },	/* shift: in add/sub reg/imm shifted instructions.  */
		//{ 22,  2 },	/* type: floating point type field in fp data inst.  */
		{ 30,  2 },	/* ldst_size: size field in ld/st reg offset inst.  */
		{ 10,  6 },	/* imm6: in add/sub reg shifted instructions.  */
		//{ 11,  4 },	/* imm4: in advsimd ext and advsimd ins instructions.  */
		{ 16,  5 },	/* imm5: in conditional compare (immediate) instructions.  */
		{ 15,  7 },	/* imm7: in load/store pair pre/post index instructions.  */
		//{ 13,  8 },	/* imm8: in floating-point scalar move immediate inst.  */
		{ 12,  9 },	/* imm9: in load/store pre/post index instructions.  */
		{ 10, 12 },	/* imm12: in ld/st unsigned imm or add/sub shifted inst.  */
		{  5, 14 },	/* imm14: in test bit and branch instructions.  */
		//{  5, 16 },	/* imm16: in exception instructions.  */
		{  0, 26 },	/* imm26: in unconditional branch instructions.  */
		{ 10,  6 },	/* imms: in bitfield and logical immediate instructions.  */
		{ 16,  6 },	/* immr: in bitfield and logical immediate instructions.  */
		//{ 16,  3 },	/* immb: in advsimd shift by immediate instructions.  */
		//{ 19,  4 },	/* immh: in advsimd shift by immediate instructions.  */
		{ 22,  1 },	/* N: in logical (immediate) instructions.  */
		{ 11,  1 },	/* index: in ld/st inst deciding the pre/post-index.  */
		{ 24,  1 },	/* index2: in ld/st pair inst deciding the pre/post-index.  */
		{ 31,  1 },	/* sf: in integer data processing instructions.  */
		//{ 11,  1 },	/* H: in advsimd scalar x indexed element instructions.  */
		//{ 21,  1 },	/* L: in advsimd scalar x indexed element instructions.  */
		//{ 20,  1 },	/* M: in advsimd scalar x indexed element instructions.  */
		{ 31,  1 },	/* b5: in the test bit and branch instructions.  */
		{ 19,  5 },	/* b40: in the test bit and branch instructions.  */
		//{ 10,  6 },	/* scale: in the fixed-point scalar to fp converting inst.  */
	};
	const aarch64_field field_patterns[] =
	{
		{  0,  0 },	/* NIL.  */
		{  0,  4 },	/* cond2: condition in truly conditional-executed inst.  */
		{  0,  5 },	/* Rt: in load/store instructions.  */
		{  0, 26 },	/* imm26: in unconditional branch instructions.  */
		{  5,  5 },	/* Rn: in many integer instructions.  */
		{  5, 14 },	/* imm14: in test bit and branch instructions.  */
		{  5, 19 },	/* imm19: e.g. in CBZ.  */
		{ 10,  3 },	/* imm3: in add/sub extended reg instructions.  */
		{ 10,  5 },	/* Rt2: in load/store pair instructions.  */
		{ 10,  6 },	/* imm6: in add/sub reg shifted instructions.  */
		{ 10, 12 },	/* imm12: in ld/st unsigned imm or add/sub shifted inst.  */
		{ 11,  1 },	/* index: in ld/st inst deciding the pre/post-index.  */
		{ 12,  1 },	/* S: in load/store reg offset instructions.  */
		{ 12,  4 },	/* cond: condition flags as a source operand.  */
		{ 12,  9 },	/* imm9: in load/store pre/post index instructions.  */
		{ 13,  3 },	/* option: in ld/st reg offset + add/sub extended reg inst.  */
		{ 15,  7 },	/* imm7: in load/store pair pre/post index instructions.  */
		{ 16,  5 },	/* imm5: in conditional compare (immediate) instructions.  */
		{ 16,  6 },	/* immr: in bitfield and logical immediate instructions.  */
		{ 19,  5 },	/* b40: in the test bit and branch instructions.  */
		{ 21,  2 },	/* hw: in move wide constant instructions.  */
		{ 22,  1 },	/* N: in logical (immediate) instructions.  */
		{ 22,  2 },	/* opc: in load/store reg offset instructions.  */
		{ 23,  1 },	/* opc1: in load/store reg offset instructions.  */
		{ 24,  1 },	/* index2: in ld/st pair inst deciding the pre/post-index.  */
		{ 29,  2 },	/* immlo: e.g. in ADRP.  */
		{ 30,  2 },	/* ldst_size: size field in ld/st reg offset inst.  */
		{ 31,  1 },	/* sf: in integer data processing instructions.  */
	};
	/*
		NOTE: Implementation above is copied from GNU Binutils.
		You must re-implement when you release AArch64 to avoid GPL.
	*/


	// –½—ß‚ÌŽí—Þ
	enum InsnType {
		UNDEF,				// Undefined or reserved
		OPERATION_INT,		// Integer
		FP_SIMD_CRYPTO,		// SIMD, floating point and crypto
		MEMORY_ADDR,		// Memory address calculation
		MEMORY_LOAD_STORE,	// Load and store
		BRANCH_EXCEPT,		// Branch and exception
		SYSTEM,				// System instruction

		//UNDEF,				// Undefined or reserved
		//MEMORY_ADDR,		// Memory address calculation
		//INT_ARITH_IMM,		// Arithmetic immediate
		//INT_LOG_IMM,		// Logical immediate
		//BIT_FIELD,			// Bitfield
		//BRANCH_EXCEPT,		// Branch and exception
		//SYSTEM,				// System instruction
		//BRANCH,				// Branch
		//TEST_BRANCH,		// Test branch
		//LOAD_STORE,			// Load and store
		//INT_ARITHREGCARRY_COND_LOGREG,	// Arithmetic register with carry, logical and conditional
		//INT_ARITHREG,		// Arithmetic register
		//SIMD_FP_CRYPTO,		// SIMD, floating point and crypto
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
		//// 0x0
		//MEMORY_ADDR, INT_ARITH_IMM, INT_LOG_IMM, BIT_FIELD, BRANCH_EXCEPT, SYSTEM, BRANCH, TEST_BRANCH,
		//// 0x8
		//LOAD_STORE, LOAD_STORE, INT_ARITHREGCARRY_COND_LOGREG, INT_ARITHREG, LOAD_STORE, LOAD_STORE,  SIMD_FP_CRYPTO, SIMD_FP_CRYPTO, 
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
		
		break;
	case OPF:
		break;
	case MEMA:
		out->Imm[0] = ExtractBits<u64>(codeWord, 5, 19); // ADDR_PCREL21; immhi
		out->Imm[1] = ExtractBits<u64>(codeWord, 29, 2); // ADDR_PCREL21; immlo
		break;
	case LDST:
		break;
	case BRX:
		break;
	case SYS:
		break;
	default:
		ASSERT(0);	// never reached
		break;
	}

}

void AArch64Decoder::DecodeInt( u32 codeWord, u32 opcode, DecodedInsn* out )
{
	out->Reg[0] = ExtractBits(codeWord, 0, 5); // Rd
	out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
	out->Reg[2] = ExtractBits(codeWord, 16, 5); // Rm

	u32 exop;
	switch(opcode){
	case 0x1:
		out->Imm[0] = ExtractBits<u64>(codeWord, 10, 12); // AIMM; imm12
		out->Imm[1] = ExtractBits<u64>(codeWord, 22, 2); // AIMM; shift
		break;
	case 0x2:
		exop = ExtractBits(codeWord, 23, 1);
		if (exop)
		{
			out->Imm[0] = ExtractBits<u64>(codeWord, 5, 16); // HALF; imm16
		}
		else
		{
			out->Imm[0] = ExtractBits<u64>(codeWord, 16, 6); // LIMM; immr
			out->Imm[1] = ExtractBits<u64>(codeWord, 10, 6); // LIMM; imms
			out->Imm[2] = ExtractBits<u64>(codeWord, 22, 1); // LIMM; N
		}
		break;
	case 0x3:
		out->Imm[0] = ExtractBits<u64>(codeWord, 10, 6); // IMMS; imms
		out->Imm[1] = ExtractBits<u64>(codeWord, 16, 6); // IMMR; immr
		break;
	case 0xa:
		exop = ExtractBits(codeWord, 22, 2);
		if (exop == 0x1 || exop == 0x2)
		{
			out->Imm[0] = ExtractBits<u64>(codeWord, 0, 4); // NZCV; nzcv
			out->Imm[1] = ExtractBits<u64>(codeWord, 16, 5); // CCMP_IMM; imm5
		}
		else
		{
			// TODO: Shifted register
			out->Imm[0] = ExtractBits<u64>(codeWord, 0, 0); 
		}
		break;
	case 0xb:
		exop = ExtractBits(codeWord, 28, 1);
		if (exop)
		{
			out->Reg[3] = ExtractBits(codeWord, 10, 5); // Ra
		} 
		else
		{
			// TODO: Shifted and Extended register
			out->Imm[0] = ExtractBits<u64>(codeWord, 0, 0); 
		}
		break;
	default:
		ASSERT(0);	// never reached
		break;
	}
}

void AArch64Decoder::DecodeMemAddress( u32 codeWord, DecodedInsn* out )
{
	out->Imm[0] = ExtractBits<u64>(codeWord, 5, 19); // ADDR_PCREL21; immhi
	out->Imm[1] = ExtractBits<u64>(codeWord, 29, 2); // ADDR_PCREL21; immlo
}

void AArch64Decoder::DecodeBranch( u32 codeWord, DecodedInsn* out )
{
	u32 opcode = ExtractBits(codeWord, 25, 3);
	u32 exop;
	switch(opcode){
	case 0x4:
		exop = ExtractBits(codeWord, 28, 4);
		switch (exop)
		{
		case 0x0:
		case 0x4:
			out->Imm[0] = ExtractBits<u64>(codeWord, 0, 26); // ADDR_PCREL26; imm26
			break;
		case 0x1:
		case 0x2:
		case 0x5:
			out->Reg[0] = ExtractBits(codeWord, 0, 5); // Rt(==Rd)
			out->Imm[0] = ExtractBits<u64>(codeWord, 5, 19); // ADDR_PCREL19; imm19
			break;
		case 0x6:
			out->Imm[0] = ExtractBits<u64>(codeWord, 5, 16); // EXCEPTION; imm16
			break;
		default:
			ASSERT(0);
			break;
		}
		break;
	case 0x6:
	case 0x7:
		exop = ExtractBits(codeWord, 29, 1);
		if (exop)
		{
			out->Reg[0] = ExtractBits(codeWord, 0, 5); // Rt(==Rd)
			out->Imm[0] = ExtractBits<u64>(codeWord, 5, 14); // ADDR_PCREL19; imm19
			out->Imm[1] = ExtractBits<u64>(codeWord, 31, 1); // BIT_NUM; b5
			out->Imm[2] = ExtractBits<u64>(codeWord, 19, 5); // BIT_NUM; b40
		}
		else
		{
			out->Reg[1] = ExtractBits(codeWord, 5, 5); // Rn
		}
		break;
	default:
		ASSERT(0);	// never reached
		break;
	}
}
