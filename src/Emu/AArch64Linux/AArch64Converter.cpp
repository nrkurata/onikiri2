#include <pch.h>

#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/OpEmulationState.h"
#include "Emu/AArch64Linux/AArch64Info.h"
#include "Emu/AArch64Linux/AArch64OpInfo.h"
#include "Emu/AArch64Linux/AArch64Decoder.h"
#include "Emu/AArch64Linux/AArch64Converter.h"
#include "Emu/AArch64Linux/AArch64Operation.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::AArch64Linux;
using namespace Onikiri::EmulatorUtility::Operation;
using namespace Onikiri::AArch64Linux::Operation;

//
// 命令の定義
//
//
namespace {
}

namespace {
	// オペランドのテンプレート
	// [RegTemplateBegin, RegTemplateEnd] は，命令中のオペランドレジスタ番号に置き換えられる
	// [ImmTemplateBegin, RegTemplateEnd] は，即値に置き換えられる

	// レジスタ・テンプレートに使用する番号
	// 本物のレジスタ番号を使ってはならない
	static const int RegTemplateBegin = -20;	// 命令中のレジスタ番号に変換 (数値に意味はない．本物のレジスタ番号と重ならずかつ一意であればよい)
	static const int RegTemplateEnd = RegTemplateBegin+5-1;
	static const int ImmTemplateBegin = -30;	// 即値に変換
	static const int ImmTemplateEnd = ImmTemplateBegin+3-1;

	const int R0 = RegTemplateBegin+0;
	const int R1 = RegTemplateBegin+1;
	const int R2 = RegTemplateBegin+2;
	const int R3 = RegTemplateBegin+3;

	const int I0 = ImmTemplateBegin+0;
	const int I1 = ImmTemplateBegin+1;
	const int I2 = ImmTemplateBegin+2;
	const int I3 = ImmTemplateBegin+3;

	const int T0 = AArch64Info::REG_TMP;
	const int CPST = AArch64Info::REG_CPST;
}

#define A64SFDSTOP(n,sf) AArch64DstOperand<n,sf>
#define A64SFSRCOP(n,sf) AArch64SrcOperand<n,sf>

#define AD0(sf) A64SFDSTOP(0,sf)
#define AD1(sf) A64SFDSTOP(1,sf)
#define AS0(sf) A64SFSRCOP(0,sf)
#define AS1(sf) A64SFSRCOP(1,sf)
#define AS2(sf) A64SFSRCOP(2,sf)
#define AS3(sf) A64SFSRCOP(3,sf)
#define AS4(sf) A64SFSRCOP(4,sf)
#define AS5(sf) A64SFSRCOP(5,sf)
#define SF SrcOperand<0>

#define A64DSTOP(n) DstOperand<n>
#define A64SRCOP(n) SrcOperand<n>
#define D0 A64DSTOP(0)
#define D1 A64DSTOP(1)
#define S0 A64SRCOP(0)
#define S1 A64SRCOP(1)
#define S2 A64SRCOP(2)
#define S3 A64SRCOP(3)

#define ARMFLAGFUNC32(func) Set< D0, ##func##< u32, S0, S1, S2>>
#define ARMFLAGFUNC64(func) Set< D0, ##func##< u64, S0, S1, S2>>
#define ARMFUNCWITHCARRY_PSR(func, cflag, vflag) \
	Sequence2< \
	Set< D0,	##func##< u32, S0, S1, S2> >,\
	Set< D1,	AArch64CalcFlagBit< ##func##<u32, S0, S1, S2>, ##cflag##<u32, S0, S1, S2>, ##vflag##<u32, S0, S1, S2> > >\
	>
#define ARMFUNC_PSR_ARITH(func, cflag, vflag) \
	Sequence2< \
	Set< D0,	##func##< u32, S0, S1> >,\
	Set< D1,	AArch64CalcFlagBit< ##func##<u32, S0, S1>, ##cflag##<u32, S0, S1>, ##vflag##<u32, S0, S1> > >\
	>

/*
	NOTE: Implementation below is copied from GNU Binutils.
	You must re-implement when you release AArch64 to avoid GPL.
	src/opcodes/aarch64-tbl.h
*/
AArch64Converter::OpDef AArch64Converter::m_OpDefsBranch[] =
{
	/** Branches, exception and system **/
	/* Compare & branch (immediate).  */
	{ "cbz", 0x34000000, 0x7f000000, 1, {
		{ OpClassCode::iBC, { -1, -1 }, { I0, I1, R0, -1, -1 }, BranchRelCond< S1, Compare< AS2(S0), IntConst<u64, 0>, IntCondEqual<u64> > > }
	} },
	{ "cbnz", 0x35000000, 0x7f000000, 1, {
		{ OpClassCode::iBC, { -1, -1 }, { I0, I1, R0, -1, -1 }, BranchRelCond< S1, Compare< AS2(S0), IntConst<u64, 0>, IntCondNotEqual<u64> > > }
	} },
	/* Conditional branch (immediate).  */
	{ "b.c", 0x54000000, 0xff000010, 1, {
		{ OpClassCode::iBC, { -1, -1 }, { I0, I1, CPST, -1, -1 }, BranchRelCond< S0, AArch64CondCalc<S1, S2> > }
	} },
	/* Exception generation.  */
	//{"svc", 0xd4000001, 0xffe0001f, exception, 0, CORE, OP1 (EXCEPTION), {}, 0},
	//{"hvc", 0xd4000002, 0xffe0001f, exception, 0, CORE, OP1 (EXCEPTION), {}, 0},
	//{"smc", 0xd4000003, 0xffe0001f, exception, 0, CORE, OP1 (EXCEPTION), {}, 0},
	//{"brk", 0xd4200000, 0xffe0001f, exception, 0, CORE, OP1 (EXCEPTION), {}, 0},
	//{"hlt", 0xd4400000, 0xffe0001f, exception, 0, CORE, OP1 (EXCEPTION), {}, 0},
	//{"dcps1", 0xd4a00001, 0xffe0001f, exception, 0, CORE, OP1 (EXCEPTION), {}, F_OPD0_OPT | F_DEFAULT (0)},
	//{"dcps2", 0xd4a00002, 0xffe0001f, exception, 0, CORE, OP1 (EXCEPTION), {}, F_OPD0_OPT | F_DEFAULT (0)},
	//{"dcps3", 0xd4a00003, 0xffe0001f, exception, 0, CORE, OP1 (EXCEPTION), {}, F_OPD0_OPT | F_DEFAULT (0)},
	/* System.  */
	//{"msr", 0xd500401f, 0xfff8f01f, ic_system, 0, CORE, OP2 (PSTATEFIELD, UIMM4), {}, -1},
	//{"hint", 0xd503201f, 0xfffff01f, ic_system, 0, CORE, OP1 (UIMM7), {}, F_HAS_ALIAS},
	{ "nop", 0xd503201f, 0xffffffff, 1, { 
		{ OpClassCode::iNOP, { -1, -1 }, { -1, -1, -1, -1, -1 }, NoOperation }
	} },
	//{"yield", 0xd503203f, 0xffffffff, ic_system, 0, CORE, OP0 (), {}, F_ALIAS},
	//{"wfe", 0xd503205f, 0xffffffff, ic_system, 0, CORE, OP0 (), {}, F_ALIAS},
	//{"wfi", 0xd503207f, 0xffffffff, ic_system, 0, CORE, OP0 (), {}, F_ALIAS},
	//{"sev", 0xd503209f, 0xffffffff, ic_system, 0, CORE, OP0 (), {}, F_ALIAS},
	//{"sevl", 0xd50320bf, 0xffffffff, ic_system, 0, CORE, OP0 (), {}, F_ALIAS},
	//{"clrex", 0xd503305f, 0xfffff0ff, ic_system, 0, CORE, OP1 (UIMM4), {}, F_OPD0_OPT | F_DEFAULT (0xF)},
	//{"dsb", 0xd503309f, 0xfffff0ff, ic_system, 0, CORE, OP1 (BARRIER), {}, 0},
	//{"dmb", 0xd50330bf, 0xfffff0ff, ic_system, 0, CORE, OP1 (BARRIER), {}, 0},
	//{"isb", 0xd50330df, 0xfffff0ff, ic_system, 0, CORE, OP1 (BARRIER_ISB), {}, F_OPD0_OPT | F_DEFAULT (0xF)},
	//{"sys", 0xd5080000, 0xfff80000, ic_system, 0, CORE, OP5 (UIMM3_OP1, Cn, Cm, UIMM3_OP2, Rt), QL_SYS, F_HAS_ALIAS | F_OPD4_OPT | F_DEFAULT (0x1F)},
	//{"at", 0xd5080000, 0xfff80000, ic_system, 0, CORE, OP2 (SYSREG_AT, Rt), QL_SRC_X, F_ALIAS},
	//{"dc", 0xd5080000, 0xfff80000, ic_system, 0, CORE, OP2 (SYSREG_DC, Rt), QL_SRC_X, F_ALIAS},
	//{"ic", 0xd5080000, 0xfff80000, ic_system, 0, CORE, OP2 (SYSREG_IC, Rt_SYS), QL_SRC_X, F_ALIAS | F_OPD1_OPT | F_DEFAULT (0x1F)},
	//{"tlbi", 0xd5080000, 0xfff80000, ic_system, 0, CORE, OP2 (SYSREG_TLBI, Rt_SYS), QL_SRC_X, F_ALIAS | F_OPD1_OPT | F_DEFAULT (0x1F)},
	//{"msr", 0xd5100000, 0xfff00000, ic_system, 0, CORE, OP2 (SYSREG, Rt), QL_SRC_X, 0},
	//{"sysl", 0xd5280000, 0xfff80000, ic_system, 0, CORE, OP5 (Rt, UIMM3_OP1, Cn, Cm, UIMM3_OP2), QL_SYSL, 0},
	//{"mrs", 0xd5300000, 0xfff00000, ic_system, 0, CORE, OP2 (Rt, SYSREG), QL_DST_X, 0},
	/* Test & branch (immediate).  */
	{ "tbz", 0x36000000, 0x7f000000, 1, {
		{ OpClassCode::iBC, { -1, -1 }, { I0, I1, I2, R0, -1 }, BranchRelCond< S2, Compare< AArch64BitTest<u64, S3, AArch64BitConcateNate<u64, S1, IntConst<u64, 5>, S0>>, IntConst<u64, 0>, IntCondEqual<u64> > > }
	} },
	{ "tbnz", 0x37000000, 0x7f000000, 1, { 
		{ OpClassCode::iBC, { -1, -1 }, { I0, I1, I2, R0, -1 }, BranchRelCond< S2, Compare< AArch64BitTest<u64, S3, AArch64BitConcateNate<u64, S1, IntConst<u64, 5>, S0>>, IntConst<u64, 0>, IntCondNotEqual<u64> > > }
	} },
	/* Unconditional branch (immediate).  */
	{ "b", 0x14000000, 0xfc000000, 1, {
		{ OpClassCode::iBU, { -1, -1 }, { I0, -1, -1, -1, -1 }, BranchRelUncond< S0 > } 
	} },
	{ "bl", 0x94000000, 0xfc000000, 1, { 
		{ OpClassCode::CALL, { 30, -1 }, { I0, -1, -1, -1, -1 }, CallRelUncond< D0, S0 > }
	} },
	/* Unconditional branch (register).  */
	{ "br", 0xd61f0000, 0xfffffc1f, 1, {
		{ OpClassCode::iBU, { -1, -1 }, { R0, -1, -1, -1, -1 }, BranchAbsUncond< S0 > } 
	} },
	{ "blr", 0xd63f0000, 0xfffffc1f, 1, { 
		{ OpClassCode::CALL_JUMP, { 30, -1 }, { R0, -1, -1, -1, -1 }, CallAbsUncond< D0, S0 > } // is CALL_JUMP ok??
	} },
	{ "ret", 0xd65f0000, 0xfffffc1f, 1, { 
		{ OpClassCode::RET, { -1, -1 }, { R0, -1, -1, -1, -1 }, BranchAbsUncond< S0 > } 
	} },
	/* Exception return
	{ "eret", 0xd69f0000, 0xfffffc1f, 1, {
		{ OpClassCode::RET, { -1, -1 }, { R0, -1, -1, -1, -1 }, BranchAbsUncond< S0 > } 
	} },
	*/
	/* Debug restore process state
	{ "drps", 0xd6bf0000, 0xfffffc1f, 1, {
		{ OpClassCode::RET, { -1, -1 }, { R0, -1, -1, -1, -1 }, BranchAbsUncond< S0 > } 
	} },
	*/
};

// 投機的にフェッチされたときにはエラーにせず，実行されたときにエラーにする
// syscallにすることにより，直前までの命令が完了してから実行される (実行は投機的でない)
AArch64Converter::OpDef AArch64Converter::m_OpDefUnknown = 
	{"unknown",	0xffffffff, 0,	1, {{OpClassCode::UNDEF,	{ -1}, {I0, -1, -1, -1, -1, -1},	AArch64Converter::AArch64UnknownOperation}}};

AArch64Converter::OpDef AArch64Converter::m_OpDefsLoadStore[] =
{
	/* Load register (literal).  */
	{ "ldr", 0x18000000, 0xff000000, 1, {
		{ OpClassCode::iLD, { R0, -1 }, { I0, -1, -1, -1, -1 }, Set<D0, Load<u32, IntAdd<u64, S0, AArch64CurrentPC<u64>>>> }
	} },
	{ "ldr", 0x58000000, 0xff000000, 1, {
		{ OpClassCode::iLD, { R0, -1 }, { I0, -1, -1, -1, -1 }, Set<D0, Load<u64, IntAdd<u64, S0, AArch64CurrentPC<u64>>>> }
	} },
	{ "ldrsw", 0x98000000, 0xff000000, 1, {
		{ OpClassCode::iLD, { R0, -1 }, { I0, -1, -1, -1, -1 }, Set<D0, Load<u32, S0>> }
	} },
	// SIMD/FP
	//{ "ldr", 0x1c000000, 0xff000000, 1, {
	//	{ OpClassCode::iLD, { R0, -1 }, { I0, -1, -1, -1, -1 }, Set<D0, Load<u32, S0>> }
	//} },
	//{ "ldr", 0x5c000000, 0xff000000, 1, {
	//	{ OpClassCode::iLD, { R0, -1 }, { I0, -1, -1, -1, -1 }, Set<D0, Load<u32, S0>> }
	//} },
	//{ "ldr", 0x9c000000, 0xff000000, 1, {
	//	{ OpClassCode::iLD, { R0, -1 }, { I0, -1, -1, -1, -1 }, Set<D0, Load<u32, S0>> }
	//} },
	// Prefetch
	//{ "prfm", 0xd8000000, 0xff000000, 1, {
	//	{ OpClassCode::iLD, { R0, -1 }, { I0, -1, -1, -1, -1 }, Set<D0, Load<u32, S0>> }
	//} },
	/* Load/store exclusive.  */
	{ "stxrb", 0x08007c00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u8, S0, S1 > } 
	} },
	{ "stlxrb", 0x0800fc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u8, S0, S1 > }
	} },
	{ "ldxrb", 0x085f7c00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u8, S0, S1 > }
	} },
	{ "ldaxrb", 0x085ffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u8, S0, S1 > }
	} },
	{ "stlrb", 0x089ffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u8, S0, S1 > }
	} },
	{ "ldarb", 0x08dffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u8, S0, S1 > }
	} },
	{ "stxrh", 0x48007c00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stlxrh", 0x4800fc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldxrh", 0x485f7c00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldaxrh", 0x485ffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stlrh", 0x489ffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldarh", 0x48dffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stxr", 0x88007c00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stxr", 0xc8007c00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stlxr", 0x8800fc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stlxr", 0xc800fc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stxp", 0x88200000, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stxp", 0xc8200000, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stlxp", 0xc8208000, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldxr", 0x885f7c00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldxr", 0xc85f7c00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldaxr", 0x885ffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldaxr", 0xc85ffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldxp", 0x887f0000, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldxp", 0xc87f0000, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldaxp", 0x887f8000, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldaxp", 0xc87f8000, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stlr", 0x889ffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stlr", 0xc89ffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldar", 0x88dffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldar", 0xc8dffc00, 0xffe08000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	/* Load/store no-allocate pair (offset).  */
	{ "stnp", 0x28000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stnp", 0x2c000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stnp", 0x6c000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stnp", 0xa8000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stnp", 0xac000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldnp", 0x28400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldnp", 0x2c400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldnp", 0x6c400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldnp", 0xa8400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldnp", 0xac400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	/* Load/store register (immediate post-indexed).  */
	{ "strb", 0x38000400, 0xffe00c00, 2, {
		{ OpClassCode::iST, { -1, -1 }, { R0, R1, -1, -1, -1 }, Store< u8, S1, S0 > },
		{ OpClassCode::iALU, { R0, -1}, { R0, I1, -1, -1, -1 }, Set<D0, IntAdd< u64, S1, S0 >> }
	} },
	{ "ldrb", 0x38400400, 0xffe00c00, 2, {
		{ OpClassCode::iLD, { R1, -1 }, { R0, -1, -1, -1, -1 }, Set<D0, Load< u8, S0>> },
		{ OpClassCode::iALU, { R0, -1 }, { R0, I1, -1, -1, -1 }, Set<D0, IntAdd< u64, S1, S0 >> }
	} },
	{ "ldrsb", 0x38800400, 0xffe00c00, 1, {
		{ OpClassCode::iLD, { R1, -1 }, { R0, -1, -1, -1, -1 }, SetSext<D0, Load< u8, S0>> },
		{ OpClassCode::iALU, { R0, -1 }, { R0, I1, -1, -1, -1 }, Set<D0, IntAdd< u64, S1, S0 >> }
	} },
	{ "ldrsb", 0x38c00400, 0xffe00c00, 1, {
		{ OpClassCode::iLD, { R1, -1 }, { R0, -1, -1, -1, -1 }, Set<D0, SetSext<s32, Load<u8, S0>>> },
		{ OpClassCode::iALU, { R0, -1 }, { R0, I1, -1, -1, -1 }, Set<D0, IntAdd< u64, S1, S0 >> }
	} },
	{ "str", 0x3c000400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x3c800400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x7c000400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xb8000400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xbc000400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xf8000400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xfc000400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x3c400400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x3cc00400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x7c400400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xb8400400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xbc400400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xf8400400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xfc400400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "strh", 0x78000400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrh", 0x78400400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsh", 0x78800400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsh", 0x78c00400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsw", 0xb8800400, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	/* Load/store register (immediate pre-indexed).  */
	{ "strb", 0x38000c00, 0xffe00c00, 2, {
		{ OpClassCode::iALU, { R0, -1 }, { R0, I1, -1, -1, -1 }, Set<D0, IntAdd< u64, S1, S0 >> },
		{ OpClassCode::iST, { -1, -1 }, { R0, R1, -1, -1, -1 }, Store< u8, S1, S0 > }
	} },
	{ "ldrb", 0x38400c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsb", 0x38800c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsb", 0x38800c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsb", 0x38c00c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x3c000c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x3c800c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x7c000c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xb8000c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xbc000c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xf8000c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xfc000c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x3c400c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x3cc00c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x7c400c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xb8400c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xbc400c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xf8400c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xfc400c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "strh", 0x78000c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrh", 0x78400c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsh", 0x78800c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsh", 0x78c00c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsw", 0xb8800c00, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	/* Load/store register (register offset).  */
	{ "strb", 0x38200800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrb", 0x38600800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsb", 0x38a00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsb", 0x38e00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x3c200800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x3ca00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x7c200800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xb8200800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xbc200800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xf8200800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xf8c00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x3c600800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x3ce00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x7c600800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xb8600800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xbc600800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xf8600800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xf8e00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "strh", 0x78200800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrh", 0x78600800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsh", 0x78a00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsh", 0x78e00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsw", 0xb8a00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "prfm", 0xf8a00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	/* Load/store register (unprivileged).  */
	{ "sttrb", 0x38000800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldtrb", 0x38400800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldtrsb", 0x38800800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldtrsb", 0x38c00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "sttrh", 0x78000800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldtrh", 0x78400800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldtrsh", 0x78800800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldtrsh", 0x78c00800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "sttr", 0xb8000800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "sttr", 0xf8000800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldtr", 0xb8400800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldtr", 0xf8400800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldtrsw", 0xb8800800, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	/* Load/store register (unscaled immediate).  */
	{ "sturb", 0x38000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldurb", 0x38400000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldursb", 0x38800000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldursb", 0x38c00000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stur", 0x3c000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stur", 0x3c800000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stur", 0x7c000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stur", 0xb8000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stur", 0xbc000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stur", 0xf8000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stur", 0xfc000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldur", 0x3c000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldur", 0x3c800000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldur", 0x7c000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldur", 0xb8000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldur", 0xbc000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldur", 0xf8000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldur", 0xfc000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "sturh", 0x78000000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldurh", 0x78400000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldursh", 0x78800000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldursh", 0x78c00000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldursw", 0xb8800000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "prfum", 0xf8800000, 0xffe00c00, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	/* Load/store register (unsigned immediate).  */
	{ "strb", 0x39000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrb", 0x39400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsb", 0x39800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsb", 0x39c00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x3d000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x3d800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0x7d000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xb9000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xbd000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xf9000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "str", 0xfd000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x3d400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x3dc00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0x7d400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xb9400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xbd400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xf9400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldr", 0xfd400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "strh", 0x79000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrh", 0x79400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsh", 0x79800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsh", 0x79c00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldrsw", 0xb9800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "prfm", 0xf9800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	/* Load/store register pair (offset).  */
	{ "stp", 0x29000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0x2d000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0x6d000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0xa9000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0xad000000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0x29400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0x2d400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0x6d400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0xa9400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0xad400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldpsw", 0x69400000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	/* Load/store register pair (post-indexed).  */
	{ "stp", 0x28800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0x2c800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0x6c800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0xa8800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0xac800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0x28c00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0x2cc00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0x6cc00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0xa8c00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0xacc00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldpsw", 0x68c00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	/* Load/store register pair (pre-indexed).  */
	{ "stp", 0x29800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0x2d800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0x6d800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0xa9800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "stp", 0xad800000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0x29c00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0x2dc00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0x6dc00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0xa9c00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldp", 0xadc00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
	{ "ldpsw", 0x69c00000, 0xffc00000, 1, {
		{ OpClassCode::iST, { -1, -1 }, { R1, R2, -1, -1, -1 }, Store< u16, S0, S1 > }
	} },
};

AArch64Converter::OpDef AArch64Converter::m_OpDefsDataProcess[] =
{
	/** Immediate **/
	/* Add/subtract (immediate).  */
	{ "add", 0x11000000, 0x7f000000, 1, { { OpClassCode::iALU, { R0, -1 }, { I0, I1, I2, R1, -1, -1 }, Set< D0, IntAdd< u32, S0, S1> > } } },
	//{ "adds", 0x31000000, 0x7f000000, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, I0, CPST, -1, -1, -1 }, ARMFUNC_PSR_ARITH(IntAdd, CarryOfAdd, AArch64AddOverflowFrom) } } },
	{ "sub", 0x51000000, 0x7f000000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, I0, -1, -1, -1, -1 }, Set< D0, IntSub< u32, S0, S1> > } } },
	//{ "subs", 0x71000000, 0x7f000000, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, I0, CPST, -1, -1, -1 }, ARMFUNC_PSR_ARITH(IntSub, AArch64NotBorrowFrom, AArch64SubOverflowFrom) } } },
	/* Bitfield.  */
	{ "sbfm", 0x13000000, 0x7f800000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, S0 > } } },
	{ "bfm", 0x33000000, 0x7f800000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, S0 > } } },
	{ "ubfm", 0x53000000, 0x7f800000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, S0 > } } },
	/* Extract.  */
	{ "extr", 0x13800000, 0x7fa00000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, I0, -1, -1 }, Set< D0, S0 > } } },
	/* Logical (immediate).  */
	{ "and", 0x12000000, 0x7f800000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, I0, -1, -1, -1 }, Set< D0, BitAnd<u64, S0, S1> > } } },
	{ "orr", 0x32000000, 0x7f800000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, I0, -1, -1, -1 }, Set< D0, BitOr<u64, S0, S1> > } } },
	{ "eor", 0x52000000, 0x7f800000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, I0, -1, -1, -1 }, Set< D0, BitXor<u64, S0, S1> > } } },
	{ "ands", 0x72000000, 0x7f800000, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, I0, -1, -1, -1 }, Set< D0, S0 > } } },
	/* Move wide (immediate).  */
	{ "movn", 0x12800000, 0x7f800000, 1, { { OpClassCode::iALU, { R0, -1 }, { I0, I1, -1, -1, -1 }, Set< D0, BitNot<u64, LShiftL<u64, S0, S1, 0x3f>> > } } },
	{ "movz", 0x52800000, 0x7f800000, 1, { { OpClassCode::iALU, { R0, -1 }, { I0, I1, -1, -1, -1 }, Set< D0, LShiftL<u64, S0, S1, 0x3f> > } } },
	{ "movk", 0x72800000, 0x7f800000, 1, { { OpClassCode::iALU, { R0, -1 }, { R0, I0, I1, -1, -1, -1 }, Set< D0, S0 > } } },
	/* PC-rel. addressing.  */
	{ "adr", 0x10000000, 0x9f000000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, S0 > } } },
	{ "adrp", 0x90000000, 0x9f000000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, S0 > } } },

	/** Register **/
	/* Add/subtract (extended register).  */
	{ "add", 0x0b200000, 0x7fe00000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, -1, -1, -1, -1 }, Set< D0, IntAdd< u32, S0, S1> > } } },
	//{ "adds", 0x2b200000, 0x7fe00000, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, R2, CPST, -1, -1, -1 }, ARMFUNC_PSR_ARITH(IntAdd, CarryOfAdd, AArch64AddOverflowFrom) } } },
	{ "sub", 0x4b200000, 0x7fe00000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, -1, -1, -1, -1 }, Set< D0, IntSub< u32, S0, S1> > } } },
	//{ "subs", 0x6b200000, 0x7fe00000, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, R2, CPST, -1, -1, -1 }, ARMFUNC_PSR_ARITH(IntSub, AArch64NotBorrowFrom, AArch64SubOverflowFrom) } } },
	/* Add/subtract (shifted register).  */
	{ "add", 0x0b000000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, I0, -1, -1, -1, -1 }, Set< D0, IntAdd< u32, S0, S1> > } } },
	//{ "adds", 0x2b000000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, R2, CPST, -1, -1, -1 }, ARMFUNC_PSR_ARITH(IntAdd, CarryOfAdd, AArch64AddOverflowFrom) } } },
	{ "sub", 0x4b000000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, I0, -1, -1, -1, -1 }, Set< D0, IntSub< u32, S0, S1> > } } },
	//{ "subs", 0x6b000000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, R2, CPST, -1, -1, -1 }, ARMFUNC_PSR_ARITH(IntSub, AArch64NotBorrowFrom, AArch64SubOverflowFrom) } } },
	/* Add/subtract (with carry).  */
	//{ "adc", 0x1a000000, 0x7fe0fc00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, CPST, -1, -1, -1 }, ARMFLAGFUNC32(AArch64IntAddwithCarry) } } },
	//{ "adcs", 0x3a000000, 0x7fe0fc00, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, R2, CPST, -1, -1, -1 }, ARMFUNCWITHCARRY_PSR(AArch64IntAddwithCarry, AArch64CarryOfAddWithCarry, AArch64AddOverflowFromWithCarry) } } },
	//{ "sbc", 0x5a000000, 0x7fe0fc00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, CPST, -1, -1, -1 }, ARMFLAGFUNC32(AArch64IntSubwithBorrow) } } },
	//{ "sbcs", 0x7a000000, 0x7fe0fc00, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, R2, CPST, -1, -1, -1 }, ARMFUNCWITHCARRY_PSR(AArch64IntSubwithBorrow, AArch64NotBorrowFromWithBorrow, AArch64SubOverflowFromWithBorrow) } } },
	/* Conditional compare (immediate).  */
	{ "ccmn", 0x3a400800, 0x7fe00c10, 1, { { OpClassCode::iALU, { CPST, -1 }, { R1, I0, I1, -1, -1 }, Set< D0, S0 > } } },
	{ "ccmp", 0x7a400800, 0x7fe00c10, 1, { { OpClassCode::iALU, { CPST, -1 }, { R1, I0, I1, -1, -1 }, Set< D0, S0 > } } },
	/* Conditional compare (register).  */
	{ "ccmn", 0x3a400000, 0x7fe00c10, 1, { { OpClassCode::iALU, { CPST, -1 }, { R1, R2, I0, -1, -1 }, Set< D0, S0 > } } },
	{ "ccmp", 0x7a400000, 0x7fe00c10, 1, { { OpClassCode::iALU, { CPST, -1 }, { R1, R2, I0, -1, -1 }, Set< D0, S0 > } } },
	/* Conditional select.  */
	{ "csel", 0x1a800000, 0x7fe00c00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, CPST, I0, -1 }, Set< D0, S0 > } } },
	{ "csinc", 0x1a800400, 0x7fe00c00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, CPST, I0, -1 }, Set< D0, S0 > } } },
	{ "csinv", 0x5a800000, 0x7fe00c00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, CPST, -1, -1 }, Set< D0, S0 > } } },
	{ "csneg", 0x5a800400, 0x7fe00c00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, CPST, -1, -1 }, Set< D0, S0 > } } },
	/* Data-processing (1 source).  */
	{ "rbit", 0x5ac00000, 0x7ffffc00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, -1, -1, -1, -1 }, Set< D0, S0 > } } },
	{ "rev16", 0x5ac00400, 0x7ffffc00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, -1, -1, -1, -1 }, Set< D0, S0 > } } },
	{ "rev", 0x5ac00800, 0xfffffc00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, -1, -1, -1, -1 }, Set< D0, S0 > } } },
	{ "rev", 0xdac00c00, 0x7ffffc00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, -1, -1, -1, -1 }, Set< D0, S0 > } } },
	{ "clz", 0x5ac01000, 0x7ffffc00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, -1, -1, -1, -1 }, Set< D0, S0 > } } },
	{ "cls", 0x5ac01400, 0x7ffffc00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, -1, -1, -1, -1 }, Set< D0, S0 > } } },
	{ "rev32", 0xdac00800, 0xfffffc00, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, -1, -1, -1, -1 }, Set< D0, S0 > } } },
	/* Data-processing (2 sources).  */
	{ "udiv", 0x1ac00800, 0x7fe0fc00, 1, { { OpClassCode::iDIV, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, IntDiv<u64, S0, S1> > } } },
	{ "sdiv", 0x1ac00c00, 0x7fe0fc00, 1, { { OpClassCode::iDIV, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, IntDiv<s64, S0, S1> > } } },
	{ "lsl", 0x1ac02000, 0x7fe0fc00, 1, { { OpClassCode::iSFT, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, LShiftL<u64, S0, S1, 0x3f> > } } },
	{ "lsr", 0x1ac02400, 0x7fe0fc00, 1, { { OpClassCode::iSFT, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, LShiftR<u64, S0, S1, 0x3f> > } } },
	{ "asr", 0x1ac02800, 0x7fe0fc00, 1, { { OpClassCode::iSFT, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, AShiftR<u64, S0, S1, 0x3f>> } } },
	{ "ror", 0x1ac02c00, 0x7fe0fc00, 1, { { OpClassCode::iSFT, { R0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, RotateR<u64, S0, S1> > } } },

	/* Data-processing (3 sources).  */
	{ "madd", 0x1b000000, 0x7fe08000, 2, {
		{ OpClassCode::iALU, { T0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, IntMul<u64, S0, S1> > },
		{ OpClassCode::iALU, { R0, -1 }, { T0, R3, -1, -1, -1 }, Set< D0, IntAdd<u64, S0, S1> > }
	} },
	{ "msub", 0x1b008000, 0x7fe08000, 2, {
		{ OpClassCode::iALU, { T0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, IntMul<u64, S0, S1> > },
		{ OpClassCode::iALU, { R0, -1 }, { T0, R3, -1, -1, -1 }, Set< D0, IntSub<u64, S0, S1> > }
	} },
	{ "smaddl", 0x9b200000, 0xffe08000, 2, {
		{ OpClassCode::iALU, { T0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, IntMul<s64, S0, S1> > },
		{ OpClassCode::iALU, { R0, -1 }, { T0, R3, -1, -1, -1 }, Set< D0, IntAdd<s64, S0, S1> > }
	} },
	{ "smsubl", 0x9b208000, 0xffe08000, 2, {
		{ OpClassCode::iALU, { T0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, IntMul<s64, S0, S1> > },
		{ OpClassCode::iALU, { R0, -1 }, { T0, R3, -1, -1, -1 }, Set< D0, IntAdd<s64, S0, S1> > }
	} },
	{ "smulh", 0x9b407c00, 0xffe08000, 1, { { OpClassCode::iALU, { T0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, IntSMulh64<S0, S1> > } } },
	{ "umaddl", 0x9ba00000, 0xffe08000, 2, {
		{ OpClassCode::iALU, { T0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, IntMul<u64, S0, S1> > },
		{ OpClassCode::iALU, { R0, -1 }, { T0, R3, -1, -1, -1 }, Set< D0, IntAdd<u64, S0, S1> > }
	} },
	{ "umsubl", 0x9ba08000, 0xffe08000, 2, {
		{ OpClassCode::iALU, { T0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, IntMul<u64, S0, S1> > },
		{ OpClassCode::iALU, { R0, -1 }, { T0, R3, -1, -1, -1 }, Set< D0, IntAdd<u64, S0, S1> > }
	} },
	{ "umulh", 0x9bc07c00, 0xffe08000, 1, { { OpClassCode::iALU, { T0, -1 }, { R1, R2, -1, -1, -1 }, Set< D0, IntUMulh64<S0, S1> > } } },
	/* Logical (shifted register).  */
	//{ "and", 0xa000000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, I0, I1, -1 }, Set< D0, BitAnd<u64, S0, AArch64ShiftOperation<u64, S1, S2, S3>> > } } },
	//{ "bic", 0xa200000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, I0, -1, -1 }, Set< D0, BitAnd<u64, S0, AArch64ShiftOperation<u64, S1, S2, S3>> > } } },
	//{ "orr", 0x2a000000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, I0, -1, -1 }, Set< D0, BitOr<u64, S0, AArch64ShiftOperation<u64, S1, S2, S3>> > } } },
	//{ "orn", 0x2a200000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, I0, -1, -1 }, Set< D0, BitOrNot<u64, S0, AArch64ShiftOperation<u64, S1, S2, S3>> > } } },
	//{ "eor", 0x4a000000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, I0, -1, -1 }, Set< D0, BitXor<u64, S0, AArch64ShiftOperation<u64, S1, S2, S3>> > } } },
	//{ "eon", 0x4a200000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, -1 }, { R1, R2, I0, -1, -1 }, Set< D0, BitXorNot<u64, S0, AArch64ShiftOperation<u64, S1, S2, S3>> > } } },
	{ "ands", 0x6a000000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, R2, I0, -1, -1 }, Set< D0, S0 > } } },
	{ "bics", 0x6a200000, 0x7f200000, 1, { { OpClassCode::iALU, { R0, CPST }, { R1, R2, I0, -1, -1 }, Set< D0, S0 > } } },

};


AArch64Converter::OpDef AArch64Converter::m_OpDefsSIMD[] =
{
	///* AdvSIMD across lanes.  */
	//{"saddlv", 0xe303800, 0xbf3ffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES_L, F_SIZEQ},
	//{"smaxv", 0xe30a800, 0xbf3ffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES, F_SIZEQ},
	//{"sminv", 0xe31a800, 0xbf3ffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES, F_SIZEQ},
	//{"addv", 0xe31b800, 0xbf3ffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES, F_SIZEQ},
	//{"uaddlv", 0x2e303800, 0xbf3ffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES_L, F_SIZEQ},
	//{"umaxv", 0x2e30a800, 0xbf3ffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES, F_SIZEQ},
	//{"uminv", 0x2e31a800, 0xbf3ffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES, F_SIZEQ},
	//{"fmaxnmv", 0x2e30c800, 0xbfbffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES_FP, F_SIZEQ},
	//{"fmaxv", 0x2e30f800, 0xbfbffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES_FP, F_SIZEQ},
	//{"fminnmv", 0x2eb0c800, 0xbfbffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES_FP, F_SIZEQ},
	//{"fminv", 0x2eb0f800, 0xbfbffc00, asimdall, 0, SIMD, OP2 (Fd, Vn), QL_XLANES_FP, F_SIZEQ},
	///* AdvSIMD three different.  */
	//{"saddl", 0x0e200000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"saddl2", 0x4e200000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"saddw", 0x0e201000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS, F_SIZEQ},
	//{"saddw2", 0x4e201000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS2, F_SIZEQ},
	//{"ssubl", 0x0e202000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"ssubl2", 0x4e202000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"ssubw", 0x0e203000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS, F_SIZEQ},
	//{"ssubw2", 0x4e203000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS2, F_SIZEQ},
	//{"addhn", 0x0e204000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3NARRBHS, F_SIZEQ},
	//{"addhn2", 0x4e204000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3NARRBHS2, F_SIZEQ},
	//{"sabal", 0x0e205000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"sabal2", 0x4e205000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"subhn", 0x0e206000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3NARRBHS, F_SIZEQ},
	//{"subhn2", 0x4e206000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3NARRBHS2, F_SIZEQ},
	//{"sabdl", 0x0e207000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"sabdl2", 0x4e207000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"smlal", 0x0e208000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"smlal2", 0x4e208000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"sqdmlal", 0x0e209000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGHS, F_SIZEQ},
	//{"sqdmlal2", 0x4e209000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGHS2, F_SIZEQ},
	//{"smlsl", 0x0e20a000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"smlsl2", 0x4e20a000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"sqdmlsl", 0x0e20b000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGHS, F_SIZEQ},
	//{"sqdmlsl2", 0x4e20b000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGHS2, F_SIZEQ},
	//{"smull", 0x0e20c000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"smull2", 0x4e20c000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"sqdmull", 0x0e20d000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGHS, F_SIZEQ},
	//{"sqdmull2", 0x4e20d000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGHS2, F_SIZEQ},
	//{"pmull", 0x0e20e000, 0xffe0fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGB, 0},
	//{"pmull", 0x0ee0e000, 0xffe0fc00, asimddiff, 0, CRYPTO, OP3 (Vd, Vn, Vm), QL_V3LONGD, 0},
	//{"pmull2", 0x4e20e000, 0xffe0fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGB2, 0},
	//{"pmull2", 0x4ee0e000, 0xffe0fc00, asimddiff, 0, CRYPTO, OP3 (Vd, Vn, Vm), QL_V3LONGD2, 0},
	//{"uaddl", 0x2e200000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"uaddl2", 0x6e200000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"uaddw", 0x2e201000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS, F_SIZEQ},
	//{"uaddw2", 0x6e201000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS2, F_SIZEQ},
	//{"usubl", 0x2e202000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"usubl2", 0x6e202000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"usubw", 0x2e203000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS, F_SIZEQ},
	//{"usubw2", 0x6e203000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS2, F_SIZEQ},
	//{"raddhn", 0x2e204000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3NARRBHS, F_SIZEQ},
	//{"raddhn2", 0x6e204000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3NARRBHS2, F_SIZEQ},
	//{"uabal", 0x2e205000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"uabal2", 0x6e205000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"rsubhn", 0x2e206000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3NARRBHS, F_SIZEQ},
	//{"rsubhn2", 0x6e206000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3NARRBHS2, F_SIZEQ},
	//{"uabdl", 0x2e207000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"uabdl2", 0x6e207000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"umlal", 0x2e208000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"umlal2", 0x6e208000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"umlsl", 0x2e20a000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"umlsl2", 0x6e20a000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	//{"umull", 0x2e20c000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS, F_SIZEQ},
	//{"umull2", 0x6e20c000, 0xff20fc00, asimddiff, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ},
	///* AdvSIMD vector x indexed element.  */
	//{"smlal", 0x0f002000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L, F_SIZEQ},
	//{"smlal2", 0x4f002000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L2, F_SIZEQ},
	//{"sqdmlal", 0x0f003000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L, F_SIZEQ},
	//{"sqdmlal2", 0x4f003000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L2, F_SIZEQ},
	//{"smlsl", 0x0f006000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L, F_SIZEQ},
	//{"smlsl2", 0x4f006000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L2, F_SIZEQ},
	//{"sqdmlsl", 0x0f007000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L, F_SIZEQ},
	//{"sqdmlsl2", 0x4f007000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L2, F_SIZEQ},
	//{"mul", 0xf008000, 0xbf00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT, F_SIZEQ},
	//{"smull", 0x0f00a000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L, F_SIZEQ},
	//{"smull2", 0x4f00a000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L2, F_SIZEQ},
	//{"sqdmull", 0x0f00b000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L, F_SIZEQ},
	//{"sqdmull2", 0x4f00b000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L2, F_SIZEQ},
	//{"sqdmulh", 0xf00c000, 0xbf00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT, F_SIZEQ},
	//{"sqrdmulh", 0xf00d000, 0xbf00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT, F_SIZEQ},
	//{"fmla", 0xf801000, 0xbf80f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_FP, F_SIZEQ},
	//{"fmls", 0xf805000, 0xbf80f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_FP, F_SIZEQ},
	//{"fmul", 0xf809000, 0xbf80f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_FP, F_SIZEQ},
	//{"mla", 0x2f000000, 0xbf00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT, F_SIZEQ},
	//{"umlal", 0x2f002000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L, F_SIZEQ},
	//{"umlal2", 0x6f002000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L2, F_SIZEQ},
	//{"mls", 0x2f004000, 0xbf00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT, F_SIZEQ},
	//{"umlsl", 0x2f006000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L, F_SIZEQ},
	//{"umlsl2", 0x6f006000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L2, F_SIZEQ},
	//{"umull", 0x2f00a000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L, F_SIZEQ},
	//{"umull2", 0x6f00a000, 0xff00f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_L2, F_SIZEQ},
	//{"fmulx", 0x2f809000, 0xbf80f400, asimdelem, 0, SIMD, OP3 (Vd, Vn, Em), QL_ELEMENT_FP, F_SIZEQ},
	///* AdvSIMD EXT.  */
	//{"ext", 0x2e000000, 0xbfe0c400, asimdext, 0, SIMD, OP4 (Vd, Vn, Vm, IDX), QL_VEXT, F_SIZEQ},
	///* AdvSIMD modified immediate.  */
	//{"movi", 0xf000400, 0xbff89c00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0W, F_SIZEQ},
	//{"orr", 0xf001400, 0xbff89c00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0W, F_SIZEQ},
	//{"movi", 0xf008400, 0xbff8dc00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0H, F_SIZEQ},
	//{"orr", 0xf009400, 0xbff8dc00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0H, F_SIZEQ},
	//{"movi", 0xf00c400, 0xbff8ec00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S1W, F_SIZEQ},
	//{"movi", 0xf00e400, 0xbff8fc00, asimdimm, OP_V_MOVI_B, SIMD, OP2 (Vd, SIMD_IMM), QL_SIMD_IMM_B, F_SIZEQ},
	//{"fmov", 0xf00f400, 0xbff8fc00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_FPIMM), QL_SIMD_IMM_S, F_SIZEQ},
	//{"mvni", 0x2f000400, 0xbff89c00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0W, F_SIZEQ},
	//{"bic", 0x2f001400, 0xbff89c00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0W, F_SIZEQ},
	//{"mvni", 0x2f008400, 0xbff8dc00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0H, F_SIZEQ},
	//{"bic", 0x2f009400, 0xbff8dc00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0H, F_SIZEQ},
	//{"mvni", 0x2f00c400, 0xbff8ec00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S1W, F_SIZEQ},
	//{"movi", 0x2f00e400, 0xfff8fc00, asimdimm, 0, SIMD, OP2 (Sd, SIMD_IMM), QL_SIMD_IMM_D, F_SIZEQ},
	//{"movi", 0x6f00e400, 0xfff8fc00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_IMM), QL_SIMD_IMM_V2D, F_SIZEQ},
	//{"fmov", 0x6f00f400, 0xfff8fc00, asimdimm, 0, SIMD, OP2 (Vd, SIMD_FPIMM), QL_SIMD_IMM_V2D, F_SIZEQ},
	///* AdvSIMD copy.  */
	//{"dup", 0xe000400, 0xbfe0fc00, asimdins, 0, SIMD, OP2 (Vd, En), QL_DUP_VX, F_T},
	//{"dup", 0xe000c00, 0xbfe0fc00, asimdins, 0, SIMD, OP2 (Vd, Rn), QL_DUP_VR, F_T},
	//{"smov", 0xe002c00, 0xbfe0fc00, asimdins, 0, SIMD, OP2 (Rd, En), QL_SMOV, F_GPRSIZE_IN_Q},
	//{"umov", 0xe003c00, 0xbfe0fc00, asimdins, 0, SIMD, OP2 (Rd, En), QL_UMOV, F_HAS_ALIAS | F_GPRSIZE_IN_Q},
	//{"mov", 0xe003c00, 0xbfe0fc00, asimdins, 0, SIMD, OP2 (Rd, En), QL_MOV, F_ALIAS | F_GPRSIZE_IN_Q},
	//{"ins", 0x4e001c00, 0xffe0fc00, asimdins, 0, SIMD, OP2 (Ed, Rn), QL_INS_XR, F_HAS_ALIAS},
	//{"mov", 0x4e001c00, 0xffe0fc00, asimdins, 0, SIMD, OP2 (Ed, Rn), QL_INS_XR, F_ALIAS},
	//{"ins", 0x6e000400, 0xffe08400, asimdins, 0, SIMD, OP2 (Ed, En), QL_S_2SAME, F_HAS_ALIAS},
	//{"mov", 0x6e000400, 0xffe08400, asimdins, 0, SIMD, OP2 (Ed, En), QL_S_2SAME, F_ALIAS},
	///* AdvSIMD two-reg misc.  */
	//{"rev64", 0xe200800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMEBHS, F_SIZEQ},
	//{"rev16", 0xe201800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMEB, F_SIZEQ},
	//{"saddlp", 0xe202800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2PAIRWISELONGBHS, F_SIZEQ},
	//{"suqadd", 0xe203800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ},
	//{"cls", 0xe204800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMEBHS, F_SIZEQ},
	//{"cnt", 0xe205800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMEB, F_SIZEQ},
	//{"sadalp", 0xe206800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2PAIRWISELONGBHS, F_SIZEQ},
	//{"sqabs", 0xe207800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ},
	//{"cmgt", 0xe208800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, IMM0), QL_V2SAME, F_SIZEQ},
	//{"cmeq", 0xe209800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, IMM0), QL_V2SAME, F_SIZEQ},
	//{"cmlt", 0xe20a800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, IMM0), QL_V2SAME, F_SIZEQ},
	//{"abs", 0xe20b800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ},
	//{"xtn", 0xe212800, 0xff3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2NARRBHS, F_SIZEQ},
	//{"xtn2", 0x4e212800, 0xff3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2NARRBHS2, F_SIZEQ},
	//{"sqxtn", 0xe214800, 0xff3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2NARRBHS, F_SIZEQ},
	//{"sqxtn2", 0x4e214800, 0xff3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2NARRBHS2, F_SIZEQ},
	//{"fcvtn", 0xe216800, 0xffbffc00, asimdmisc, OP_FCVTN, SIMD, OP2 (Vd, Vn), QL_V2NARRHS, F_MISC},
	//{"fcvtn2", 0x4e216800, 0xffbffc00, asimdmisc, OP_FCVTN2, SIMD, OP2 (Vd, Vn), QL_V2NARRHS2, F_MISC},
	//{"fcvtl", 0xe217800, 0xffbffc00, asimdmisc, OP_FCVTL, SIMD, OP2 (Vd, Vn), QL_V2LONGHS, F_MISC},
	//{"fcvtl2", 0x4e217800, 0xffbffc00, asimdmisc, OP_FCVTL2, SIMD, OP2 (Vd, Vn), QL_V2LONGHS2, F_MISC},
	//{"frintn", 0xe218800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"frintm", 0xe219800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcvtns", 0xe21a800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcvtms", 0xe21b800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcvtas", 0xe21c800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"scvtf", 0xe21d800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcmgt", 0xea0c800, 0xbfbffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, IMM0), QL_V2SAMESD, F_SIZEQ},
	//{"fcmeq", 0xea0d800, 0xbfbffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, IMM0), QL_V2SAMESD, F_SIZEQ},
	//{"fcmlt", 0xea0e800, 0xbfbffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, IMM0), QL_V2SAMESD, F_SIZEQ},
	//{"fabs", 0xea0f800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"frintp", 0xea18800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"frintz", 0xea19800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcvtps", 0xea1a800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcvtzs", 0xea1b800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"urecpe", 0xea1c800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMES, F_SIZEQ},
	//{"frecpe", 0xea1d800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"rev32", 0x2e200800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMEBH, F_SIZEQ},
	//{"uaddlp", 0x2e202800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2PAIRWISELONGBHS, F_SIZEQ},
	//{"usqadd", 0x2e203800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ},
	//{"clz", 0x2e204800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMEBHS, F_SIZEQ},
	//{"uadalp", 0x2e206800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2PAIRWISELONGBHS, F_SIZEQ},
	//{"sqneg", 0x2e207800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ},
	//{"cmge", 0x2e208800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, IMM0), QL_V2SAME, F_SIZEQ},
	//{"cmle", 0x2e209800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, IMM0), QL_V2SAME, F_SIZEQ},
	//{"neg", 0x2e20b800, 0xbf3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ},
	//{"sqxtun", 0x2e212800, 0xff3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2NARRBHS, F_SIZEQ},
	//{"sqxtun2", 0x6e212800, 0xff3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2NARRBHS2, F_SIZEQ},
	//{"shll", 0x2e213800, 0xff3ffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, SHLL_IMM), QL_V2LONGBHS, F_SIZEQ},
	//{"shll2", 0x6e213800, 0xff3ffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, SHLL_IMM), QL_V2LONGBHS2, F_SIZEQ},
	//{"uqxtn", 0x2e214800, 0xff3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2NARRBHS, F_SIZEQ},
	//{"uqxtn2", 0x6e214800, 0xff3ffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2NARRBHS2, F_SIZEQ},
	//{"fcvtxn", 0x2e616800, 0xfffffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2NARRS, 0},
	//{"fcvtxn2", 0x6e616800, 0xfffffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2NARRS2, 0},
	//{"frinta", 0x2e218800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"frintx", 0x2e219800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcvtnu", 0x2e21a800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcvtmu", 0x2e21b800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcvtau", 0x2e21c800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"ucvtf", 0x2e21d800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"not", 0x2e205800, 0xbffffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMEB, F_SIZEQ | F_HAS_ALIAS},
	//{"mvn", 0x2e205800, 0xbffffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMEB, F_SIZEQ | F_ALIAS},
	//{"rbit", 0x2e605800, 0xbffffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMEB, F_SIZEQ},
	//{"fcmge", 0x2ea0c800, 0xbfbffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, IMM0), QL_V2SAMESD, F_SIZEQ},
	//{"fcmle", 0x2ea0d800, 0xbfbffc00, asimdmisc, 0, SIMD, OP3 (Vd, Vn, IMM0), QL_V2SAMESD, F_SIZEQ},
	//{"fneg", 0x2ea0f800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"frinti", 0x2ea19800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcvtpu", 0x2ea1a800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fcvtzu", 0x2ea1b800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"ursqrte", 0x2ea1c800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMES, F_SIZEQ},
	//{"frsqrte", 0x2ea1d800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	//{"fsqrt", 0x2ea1f800, 0xbfbffc00, asimdmisc, 0, SIMD, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ},
	///* AdvSIMD ZIP/UZP/TRN.  */
	//{"uzp1", 0xe001800, 0xbf20fc00, asimdperm, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"trn1", 0xe002800, 0xbf20fc00, asimdperm, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"zip1", 0xe003800, 0xbf20fc00, asimdperm, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"uzp2", 0xe005800, 0xbf20fc00, asimdperm, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"trn2", 0xe006800, 0xbf20fc00, asimdperm, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"zip2", 0xe007800, 0xbf20fc00, asimdperm, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	///* AdvSIMD three same.  */
	//{"shadd", 0xe200400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"sqadd", 0xe200c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"srhadd", 0xe201400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"shsub", 0xe202400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"sqsub", 0xe202c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"cmgt", 0xe203400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"cmge", 0xe203c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"sshl", 0xe204400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"sqshl", 0xe204c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"srshl", 0xe205400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"sqrshl", 0xe205c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"smax", 0xe206400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"smin", 0xe206c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"sabd", 0xe207400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"saba", 0xe207c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"add", 0xe208400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"cmtst", 0xe208c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"mla", 0xe209400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"mul", 0xe209c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"smaxp", 0xe20a400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"sminp", 0xe20ac00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"sqdmulh", 0xe20b400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEHS, F_SIZEQ},
	//{"addp", 0xe20bc00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"fmaxnm", 0xe20c400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fmla", 0xe20cc00, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fadd", 0xe20d400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fmulx", 0xe20dc00, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fcmeq", 0xe20e400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fmax", 0xe20f400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"frecps", 0xe20fc00, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"and", 0xe201c00, 0xbfe0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ},
	//{"bic", 0xe601c00, 0xbfe0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ},
	//{"fminnm", 0xea0c400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fmls", 0xea0cc00, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fsub", 0xea0d400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fmin", 0xea0f400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"frsqrts", 0xea0fc00, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"orr", 0xea01c00, 0xbfe0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_HAS_ALIAS | F_SIZEQ},
	//{"mov", 0xea01c00, 0xbfe0fc00, asimdsame, OP_MOV_V, SIMD, OP2 (Vd, Vn), QL_V2SAMEB, F_ALIAS | F_CONV},
	//{"orn", 0xee01c00, 0xbfe0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ},
	//{"uhadd", 0x2e200400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"uqadd", 0x2e200c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"urhadd", 0x2e201400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"uhsub", 0x2e202400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"uqsub", 0x2e202c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"cmhi", 0x2e203400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"cmhs", 0x2e203c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"ushl", 0x2e204400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"uqshl", 0x2e204c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"urshl", 0x2e205400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"uqrshl", 0x2e205c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"umax", 0x2e206400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"umin", 0x2e206c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"uabd", 0x2e207400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"uaba", 0x2e207c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"sub", 0x2e208400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"cmeq", 0x2e208c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ},
	//{"mls", 0x2e209400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"pmul", 0x2e209c00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ},
	//{"umaxp", 0x2e20a400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"uminp", 0x2e20ac00, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ},
	//{"sqrdmulh", 0x2e20b400, 0xbf20fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEHS, F_SIZEQ},
	//{"fmaxnmp", 0x2e20c400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"faddp", 0x2e20d400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fmul", 0x2e20dc00, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fcmge", 0x2e20e400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"facge", 0x2e20ec00, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fmaxp", 0x2e20f400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fdiv", 0x2e20fc00, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"eor", 0x2e201c00, 0xbfe0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ},
	//{"bsl", 0x2e601c00, 0xbfe0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ},
	//{"fminnmp", 0x2ea0c400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fabd", 0x2ea0d400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fcmgt", 0x2ea0e400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"facgt", 0x2ea0ec00, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"fminp", 0x2ea0f400, 0xbfa0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ},
	//{"bit", 0x2ea01c00, 0xbfe0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ},
	//{"bif", 0x2ee01c00, 0xbfe0fc00, asimdsame, 0, SIMD, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ},
	///* AdvSIMD shift by immediate.  */
	//{"sshr", 0xf000400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0},
	//{"ssra", 0xf001400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0},
	//{"srshr", 0xf002400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0},
	//{"srsra", 0xf003400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0},
	//{"shl", 0xf005400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFT, 0},
	//{"sqshl", 0xf007400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFT, 0},
	//{"shrn", 0xf008400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0},
	//{"shrn2", 0x4f008400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0},
	//{"rshrn", 0xf008c00, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0},
	//{"rshrn2", 0x4f008c00, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0},
	//{"sqshrn", 0xf009400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0},
	//{"sqshrn2", 0x4f009400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0},
	//{"sqrshrn", 0xf009c00, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0},
	//{"sqrshrn2", 0x4f009c00, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0},
	//{"sshll", 0xf00a400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFTL, F_HAS_ALIAS},
	//{"sxtl", 0xf00a400, 0xff87fc00, asimdshf, OP_SXTL, SIMD, OP2 (Vd, Vn), QL_V2LONGBHS, F_ALIAS | F_CONV},
	//{"sshll2", 0x4f00a400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFTL2, F_HAS_ALIAS},
	//{"sxtl2", 0x4f00a400, 0xff87fc00, asimdshf, OP_SXTL2, SIMD, OP2 (Vd, Vn), QL_V2LONGBHS2, F_ALIAS | F_CONV},
	//{"scvtf", 0xf00e400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_SD, 0},
	//{"fcvtzs", 0xf00fc00, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_SD, 0},
	//{"ushr", 0x2f000400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0},
	//{"usra", 0x2f001400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0},
	//{"urshr", 0x2f002400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0},
	//{"ursra", 0x2f003400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0},
	//{"sri", 0x2f004400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0},
	//{"sli", 0x2f005400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFT, 0},
	//{"sqshlu", 0x2f006400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFT, 0},
	//{"uqshl", 0x2f007400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFT, 0},
	//{"sqshrun", 0x2f008400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0},
	//{"sqshrun2", 0x6f008400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0},
	//{"sqrshrun", 0x2f008c00, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0},
	//{"sqrshrun2", 0x6f008c00, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0},
	//{"uqshrn", 0x2f009400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0},
	//{"uqshrn2", 0x6f009400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0},
	//{"uqrshrn", 0x2f009c00, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0},
	//{"uqrshrn2", 0x6f009c00, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0},
	//{"ushll", 0x2f00a400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFTL, F_HAS_ALIAS},
	//{"uxtl", 0x2f00a400, 0xff87fc00, asimdshf, OP_UXTL, SIMD, OP2 (Vd, Vn), QL_V2LONGBHS, F_ALIAS | F_CONV},
	//{"ushll2", 0x6f00a400, 0xff80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFTL2, F_HAS_ALIAS},
	//{"uxtl2", 0x6f00a400, 0xff87fc00, asimdshf, OP_UXTL2, SIMD, OP2 (Vd, Vn), QL_V2LONGBHS2, F_ALIAS | F_CONV},
	//{"ucvtf", 0x2f00e400, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_SD, 0},
	//{"fcvtzu", 0x2f00fc00, 0xbf80fc00, asimdshf, 0, SIMD, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_SD, 0},
	///* AdvSIMD TBL/TBX.  */
	//{"tbl", 0xe000000, 0xbfe09c00, asimdtbl, 0, SIMD, OP3 (Vd, LVn, Vm), QL_TABLE, F_SIZEQ},
	//{"tbx", 0xe001000, 0xbfe09c00, asimdtbl, 0, SIMD, OP3 (Vd, LVn, Vm), QL_TABLE, F_SIZEQ},
	///* AdvSIMD scalar three different.  */
	//{"sqdmlal", 0x5e209000, 0xff20fc00, asisddiff, 0, SIMD, OP3 (Sd, Sn, Sm), QL_SISDL_HS, F_SSIZE},
	//{"sqdmlsl", 0x5e20b000, 0xff20fc00, asisddiff, 0, SIMD, OP3 (Sd, Sn, Sm), QL_SISDL_HS, F_SSIZE},
	//{"sqdmull", 0x5e20d000, 0xff20fc00, asisddiff, 0, SIMD, OP3 (Sd, Sn, Sm), QL_SISDL_HS, F_SSIZE},
	///* AdvSIMD scalar x indexed element.  */
	//{"sqdmlal", 0x5f003000, 0xff00f400, asisdelem, 0, SIMD, OP3 (Sd, Sn, Em), QL_SISDL_HS, F_SSIZE},
	//{"sqdmlsl", 0x5f007000, 0xff00f400, asisdelem, 0, SIMD, OP3 (Sd, Sn, Em), QL_SISDL_HS, F_SSIZE},
	//{"sqdmull", 0x5f00b000, 0xff00f400, asisdelem, 0, SIMD, OP3 (Sd, Sn, Em), QL_SISDL_HS, F_SSIZE},
	//{"sqdmulh", 0x5f00c000, 0xff00f400, asisdelem, 0, SIMD, OP3 (Sd, Sn, Em), QL_SISD_HS, F_SSIZE},
	//{"sqrdmulh", 0x5f00d000, 0xff00f400, asisdelem, 0, SIMD, OP3 (Sd, Sn, Em), QL_SISD_HS, F_SSIZE},
	//{"fmla", 0x5f801000, 0xff80f400, asisdelem, 0, SIMD, OP3 (Sd, Sn, Em), QL_FP3, F_SSIZE},
	//{"fmls", 0x5f805000, 0xff80f400, asisdelem, 0, SIMD, OP3 (Sd, Sn, Em), QL_FP3, F_SSIZE},
	//{"fmul", 0x5f809000, 0xff80f400, asisdelem, 0, SIMD, OP3 (Sd, Sn, Em), QL_FP3, F_SSIZE},
	//{"fmulx", 0x7f809000, 0xff80f400, asisdelem, 0, SIMD, OP3 (Sd, Sn, Em), QL_FP3, F_SSIZE},
	///* AdvSIMD load/store multiple structures.  */
	//{"st4", 0xc000000, 0xbfff0000, asisdlse, 0, SIMD, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(4)},
	//{"st1", 0xc000000, 0xbfff0000, asisdlse, 0, SIMD, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)},
	//{"st2", 0xc000000, 0xbfff0000, asisdlse, 0, SIMD, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(2)},
	//{"st3", 0xc000000, 0xbfff0000, asisdlse, 0, SIMD, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(3)},
	//{"ld4", 0xc400000, 0xbfff0000, asisdlse, 0, SIMD, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(4)},
	//{"ld1", 0xc400000, 0xbfff0000, asisdlse, 0, SIMD, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)},
	//{"ld2", 0xc400000, 0xbfff0000, asisdlse, 0, SIMD, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(2)},
	//{"ld3", 0xc400000, 0xbfff0000, asisdlse, 0, SIMD, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(3)},
	///* AdvSIMD load/store multiple structures (post-indexed).  */
	//{"st4", 0xc800000, 0xbfe00000, asisdlsep, 0, SIMD, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(4)},
	//{"st1", 0xc800000, 0xbfe00000, asisdlsep, 0, SIMD, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)},
	//{"st2", 0xc800000, 0xbfe00000, asisdlsep, 0, SIMD, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(2)},
	//{"st3", 0xc800000, 0xbfe00000, asisdlsep, 0, SIMD, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(3)},
	//{"ld4", 0xcc00000, 0xbfe00000, asisdlsep, 0, SIMD, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(4)},
	//{"ld1", 0xcc00000, 0xbfe00000, asisdlsep, 0, SIMD, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)},
	//{"ld2", 0xcc00000, 0xbfe00000, asisdlsep, 0, SIMD, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(2)},
	//{"ld3", 0xcc00000, 0xbfe00000, asisdlsep, 0, SIMD, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(3)},
	///* AdvSIMD load/store single structure.  */
	//{"st1", 0xd000000, 0xbfff2000, asisdlso, 0, SIMD, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(1)},
	//{"st3", 0xd002000, 0xbfff2000, asisdlso, 0, SIMD, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(3)},
	//{"st2", 0xd200000, 0xbfff2000, asisdlso, 0, SIMD, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(2)},
	//{"st4", 0xd202000, 0xbfff2000, asisdlso, 0, SIMD, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(4)},
	//{"ld1", 0xd400000, 0xbfff2000, asisdlso, 0, SIMD, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(1)},
	//{"ld3", 0xd402000, 0xbfff2000, asisdlso, 0, SIMD, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(3)},
	//{"ld1r", 0xd40c000, 0xbfffe000, asisdlso, 0, SIMD, OP2 (LVt_AL, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)},
	//{"ld3r", 0xd40e000, 0xbfffe000, asisdlso, 0, SIMD, OP2 (LVt_AL, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(3)},
	//{"ld2", 0xd600000, 0xbfff2000, asisdlso, 0, SIMD, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(2)},
	//{"ld4", 0xd602000, 0xbfff2000, asisdlso, 0, SIMD, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(4)},
	//{"ld2r", 0xd60c000, 0xbfffe000, asisdlso, 0, SIMD, OP2 (LVt_AL, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(2)},
	//{"ld4r", 0xd60e000, 0xbfffe000, asisdlso, 0, SIMD, OP2 (LVt_AL, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(4)},
	///* AdvSIMD load/store single structure (post-indexed).  */
	//{"st1", 0xd800000, 0xbfe02000, asisdlsop, 0, SIMD, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(1)},
	//{"st3", 0xd802000, 0xbfe02000, asisdlsop, 0, SIMD, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(3)},
	//{"st2", 0xda00000, 0xbfe02000, asisdlsop, 0, SIMD, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(2)},
	//{"st4", 0xda02000, 0xbfe02000, asisdlsop, 0, SIMD, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(4)},
	//{"ld1", 0xdc00000, 0xbfe02000, asisdlsop, 0, SIMD, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(1)},
	//{"ld3", 0xdc02000, 0xbfe02000, asisdlsop, 0, SIMD, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(3)},
	//{"ld1r", 0xdc0c000, 0xbfe0e000, asisdlsop, 0, SIMD, OP2 (LVt_AL, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)},
	//{"ld3r", 0xdc0e000, 0xbfe0e000, asisdlsop, 0, SIMD, OP2 (LVt_AL, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(3)},
	//{"ld2", 0xde00000, 0xbfe02000, asisdlsop, 0, SIMD, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(2)},
	//{"ld4", 0xde02000, 0xbfe02000, asisdlsop, 0, SIMD, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(4)},
	//{"ld2r", 0xde0c000, 0xbfe0e000, asisdlsop, 0, SIMD, OP2 (LVt_AL, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(2)},
	//{"ld4r", 0xde0e000, 0xbfe0e000, asisdlsop, 0, SIMD, OP2 (LVt_AL, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(4)},
	///* AdvSIMD scalar two-reg misc.  */
	//{"suqadd", 0x5e203800, 0xff3ffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAME, F_SSIZE},
	//{"sqabs", 0x5e207800, 0xff3ffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAME, F_SSIZE},
	//{"cmgt", 0x5e208800, 0xff3ffc00, asisdmisc, 0, SIMD, OP3 (Sd, Sn, IMM0), QL_SISD_CMP_0, F_SSIZE},
	//{"cmeq", 0x5e209800, 0xff3ffc00, asisdmisc, 0, SIMD, OP3 (Sd, Sn, IMM0), QL_SISD_CMP_0, F_SSIZE},
	//{"cmlt", 0x5e20a800, 0xff3ffc00, asisdmisc, 0, SIMD, OP3 (Sd, Sn, IMM0), QL_SISD_CMP_0, F_SSIZE},
	//{"abs", 0x5e20b800, 0xff3ffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_2SAMED, F_SSIZE},
	//{"sqxtn", 0x5e214800, 0xff3ffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_SISD_NARROW, F_SSIZE},
	//{"fcvtns", 0x5e21a800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"fcvtms", 0x5e21b800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"fcvtas", 0x5e21c800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"scvtf", 0x5e21d800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"fcmgt", 0x5ea0c800, 0xffbffc00, asisdmisc, 0, SIMD, OP3 (Sd, Sn, IMM0), QL_SISD_FCMP_0, F_SSIZE},
	//{"fcmeq", 0x5ea0d800, 0xffbffc00, asisdmisc, 0, SIMD, OP3 (Sd, Sn, IMM0), QL_SISD_FCMP_0, F_SSIZE},
	//{"fcmlt", 0x5ea0e800, 0xffbffc00, asisdmisc, 0, SIMD, OP3 (Sd, Sn, IMM0), QL_SISD_FCMP_0, F_SSIZE},
	//{"fcvtps", 0x5ea1a800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"fcvtzs", 0x5ea1b800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"frecpe", 0x5ea1d800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"frecpx", 0x5ea1f800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"usqadd", 0x7e203800, 0xff3ffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAME, F_SSIZE},
	//{"sqneg", 0x7e207800, 0xff3ffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAME, F_SSIZE},
	//{"cmge", 0x7e208800, 0xff3ffc00, asisdmisc, 0, SIMD, OP3 (Sd, Sn, IMM0), QL_SISD_CMP_0, F_SSIZE},
	//{"cmle", 0x7e209800, 0xff3ffc00, asisdmisc, 0, SIMD, OP3 (Sd, Sn, IMM0), QL_SISD_CMP_0, F_SSIZE},
	//{"neg", 0x7e20b800, 0xff3ffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_2SAMED, F_SSIZE},
	//{"sqxtun", 0x7e212800, 0xff3ffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_SISD_NARROW, F_SSIZE},
	//{"uqxtn", 0x7e214800, 0xff3ffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_SISD_NARROW, F_SSIZE},
	//{"fcvtxn", 0x7e216800, 0xffbffc00, asisdmisc, OP_FCVTXN_S, SIMD, OP2 (Sd, Sn), QL_SISD_NARROW_S, F_MISC},
	//{"fcvtnu", 0x7e21a800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"fcvtmu", 0x7e21b800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"fcvtau", 0x7e21c800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"ucvtf", 0x7e21d800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"fcmge", 0x7ea0c800, 0xffbffc00, asisdmisc, 0, SIMD, OP3 (Sd, Sn, IMM0), QL_SISD_FCMP_0, F_SSIZE},
	//{"fcmle", 0x7ea0d800, 0xffbffc00, asisdmisc, 0, SIMD, OP3 (Sd, Sn, IMM0), QL_SISD_FCMP_0, F_SSIZE},
	//{"fcvtpu", 0x7ea1a800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"fcvtzu", 0x7ea1b800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	//{"frsqrte", 0x7ea1d800, 0xffbffc00, asisdmisc, 0, SIMD, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE},
	///* AdvSIMD scalar copy.  */
	//{"dup", 0x5e000400, 0xffe0fc00, asisdone, 0, SIMD, OP2 (Sd, En), QL_S_2SAME, F_HAS_ALIAS},
	//{"mov", 0x5e000400, 0xffe0fc00, asisdone, 0, SIMD, OP2 (Sd, En), QL_S_2SAME, F_ALIAS},
	///* AdvSIMD scalar pairwise.  */
	//{"addp", 0x5e31b800, 0xff3ffc00, asisdpair, 0, SIMD, OP2 (Sd, Vn), QL_SISD_PAIR_D, F_SIZEQ},
	//{"fmaxnmp", 0x7e30c800, 0xffbffc00, asisdpair, 0, SIMD, OP2 (Sd, Vn), QL_SISD_PAIR, F_SIZEQ},
	//{"faddp", 0x7e30d800, 0xffbffc00, asisdpair, 0, SIMD, OP2 (Sd, Vn), QL_SISD_PAIR, F_SIZEQ},
	//{"fmaxp", 0x7e30f800, 0xffbffc00, asisdpair, 0, SIMD, OP2 (Sd, Vn), QL_SISD_PAIR, F_SIZEQ},
	//{"fminnmp", 0x7eb0c800, 0xffbffc00, asisdpair, 0, SIMD, OP2 (Sd, Vn), QL_SISD_PAIR, F_SIZEQ},
	//{"fminp", 0x7eb0f800, 0xffbffc00, asisdpair, 0, SIMD, OP2 (Sd, Vn), QL_SISD_PAIR, F_SIZEQ},
	///* AdvSIMD scalar three same.  */
	//{"sqadd", 0x5e200c00, 0xff20fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE},
	//{"sqsub", 0x5e202c00, 0xff20fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE},
	//{"sqshl", 0x5e204c00, 0xff20fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE},
	//{"sqrshl", 0x5e205c00, 0xff20fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE},
	//{"sqdmulh", 0x5e20b400, 0xff20fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_SISD_HS, F_SSIZE},
	//{"fmulx", 0x5e20dc00, 0xffa0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE},
	//{"fcmeq", 0x5e20e400, 0xffa0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE},
	//{"frecps", 0x5e20fc00, 0xffa0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE},
	//{"frsqrts", 0x5ea0fc00, 0xffa0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE},
	//{"cmgt", 0x5ee03400, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"cmge", 0x5ee03c00, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"sshl", 0x5ee04400, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"srshl", 0x5ee05400, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"add", 0x5ee08400, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"cmtst", 0x5ee08c00, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"uqadd", 0x7e200c00, 0xff20fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE},
	//{"uqsub", 0x7e202c00, 0xff20fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE},
	//{"uqshl", 0x7e204c00, 0xff20fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE},
	//{"uqrshl", 0x7e205c00, 0xff20fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE},
	//{"sqrdmulh", 0x7e20b400, 0xff20fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_SISD_HS, F_SSIZE},
	//{"fcmge", 0x7e20e400, 0xffa0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE},
	//{"facge", 0x7e20ec00, 0xffa0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE},
	//{"fabd", 0x7ea0d400, 0xffa0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE},
	//{"fcmgt", 0x7ea0e400, 0xffa0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE},
	//{"facgt", 0x7ea0ec00, 0xffa0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE},
	//{"cmhi", 0x7ee03400, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"cmhs", 0x7ee03c00, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"ushl", 0x7ee04400, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"urshl", 0x7ee05400, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"sub", 0x7ee08400, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	//{"cmeq", 0x7ee08c00, 0xffe0fc00, asisdsame, 0, SIMD, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE},
	///* AdvSIMD scalar shift by immediate.  */
	//{"sshr", 0x5f000400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0},
	//{"ssra", 0x5f001400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0},
	//{"srshr", 0x5f002400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0},
	//{"srsra", 0x5f003400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0},
	//{"shl", 0x5f005400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSL), QL_SSHIFT_D, 0},
	//{"sqshl", 0x5f007400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSL), QL_SSHIFT, 0},
	//{"sqshrn", 0x5f009400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0},
	//{"sqrshrn", 0x5f009c00, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0},
	//{"scvtf", 0x5f00e400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_SD, 0},
	//{"fcvtzs", 0x5f00fc00, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_SD, 0},
	//{"ushr", 0x7f000400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0},
	//{"usra", 0x7f001400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0},
	//{"urshr", 0x7f002400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0},
	//{"ursra", 0x7f003400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0},
	//{"sri", 0x7f004400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0},
	//{"sli", 0x7f005400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSL), QL_SSHIFT_D, 0},
	//{"sqshlu", 0x7f006400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSL), QL_SSHIFT, 0},
	//{"uqshl", 0x7f007400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSL), QL_SSHIFT, 0},
	//{"sqshrun", 0x7f008400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0},
	//{"sqrshrun", 0x7f008c00, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0},
	//{"uqshrn", 0x7f009400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0},
	//{"uqrshrn", 0x7f009c00, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0},
	//{"ucvtf", 0x7f00e400, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_SD, 0},
	//{"fcvtzu", 0x7f00fc00, 0xff80fc00, asisdshf, 0, SIMD, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_SD, 0},

	{"nop", 0xd503201f, 0xffffffff, 1, {{OpClassCode::iNOP, {-1, -1}, {-1, -1, -1, -1, -1}, NoOperation}}}, // dummy
};

AArch64Converter::OpDef AArch64Converter::m_OpDefsFP[] =
{
	{ "nop", 0xd503201f, 0xffffffff, 1, { { OpClassCode::iNOP, { -1, -1 }, { -1, -1, -1, -1, -1 }, NoOperation } } }, // dummy
	///* Crypto AES.  */
	//{"aese", 0x4e284800, 0xfffffc00, cryptoaes, 0, CRYPTO, OP2 (Vd, Vn), QL_V2SAME16B, 0},
	//{"aesd", 0x4e285800, 0xfffffc00, cryptoaes, 0, CRYPTO, OP2 (Vd, Vn), QL_V2SAME16B, 0},
	//{"aesmc", 0x4e286800, 0xfffffc00, cryptoaes, 0, CRYPTO, OP2 (Vd, Vn), QL_V2SAME16B, 0},
	//{"aesimc", 0x4e287800, 0xfffffc00, cryptoaes, 0, CRYPTO, OP2 (Vd, Vn), QL_V2SAME16B, 0},
	///* Crypto three-reg SHA.  */
	//{"sha1c", 0x5e000000, 0xffe0fc00, cryptosha3, 0, CRYPTO, OP3 (Fd, Fn, Vm), QL_SHAUPT, 0},
	//{"sha1p", 0x5e001000, 0xffe0fc00, cryptosha3, 0, CRYPTO, OP3 (Fd, Fn, Vm), QL_SHAUPT, 0},
	//{"sha1m", 0x5e002000, 0xffe0fc00, cryptosha3, 0, CRYPTO, OP3 (Fd, Fn, Vm), QL_SHAUPT, 0},
	//{"sha1su0", 0x5e003000, 0xffe0fc00, cryptosha3, 0, CRYPTO, OP3 (Vd, Vn, Vm), QL_V3SAME4S, 0},
	//{"sha256h", 0x5e004000, 0xffe0fc00, cryptosha3, 0, CRYPTO, OP3 (Fd, Fn, Vm), QL_SHA256UPT, 0},
	//{"sha256h2", 0x5e005000, 0xffe0fc00, cryptosha3, 0, CRYPTO, OP3 (Fd, Fn, Vm), QL_SHA256UPT, 0},
	//{"sha256su1", 0x5e006000, 0xffe0fc00, cryptosha3, 0, CRYPTO, OP3 (Vd, Vn, Vm), QL_V3SAME4S, 0},
	///* Crypto two-reg SHA.  */
	//{"sha1h", 0x5e280800, 0xfffffc00, cryptosha2, 0, CRYPTO, OP2 (Fd, Fn), QL_2SAMES, 0},
	//{"sha1su1", 0x5e281800, 0xfffffc00, cryptosha2, 0, CRYPTO, OP2 (Vd, Vn), QL_V2SAME4S, 0},
	//{"sha256su0", 0x5e282800, 0xfffffc00, cryptosha2, 0, CRYPTO, OP2 (Vd, Vn), QL_V2SAME4S, 0},


	///* Floating-point<->fixed-point conversions.  */
	//{"scvtf", 0x1e020000, 0x7f3f0000, float2fix, 0, FP, OP3 (Fd, Rn, FBITS), QL_FIX2FP, F_FPTYPE | F_SF},
	//{"ucvtf", 0x1e030000, 0x7f3f0000, float2fix, 0, FP, OP3 (Fd, Rn, FBITS), QL_FIX2FP, F_FPTYPE | F_SF},
	//{"fcvtzs", 0x1e180000, 0x7f3f0000, float2fix, 0, FP, OP3 (Rd, Fn, FBITS), QL_FP2FIX, F_FPTYPE | F_SF},
	//{"fcvtzu", 0x1e190000, 0x7f3f0000, float2fix, 0, FP, OP3 (Rd, Fn, FBITS), QL_FP2FIX, F_FPTYPE | F_SF},
	///* Floating-point<->integer conversions.  */
	//{"fcvtns", 0x1e200000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"fcvtnu", 0x1e210000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"scvtf", 0x1e220000, 0x7f3ffc00, float2int, 0, FP, OP2 (Fd, Rn), QL_INT2FP, F_FPTYPE | F_SF},
	//{"ucvtf", 0x1e230000, 0x7f3ffc00, float2int, 0, FP, OP2 (Fd, Rn), QL_INT2FP, F_FPTYPE | F_SF},
	//{"fcvtas", 0x1e240000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"fcvtau", 0x1e250000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"fmov", 0x1e260000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"fmov", 0x1e270000, 0x7f3ffc00, float2int, 0, FP, OP2 (Fd, Rn), QL_INT2FP, F_FPTYPE | F_SF},
	//{"fcvtps", 0x1e280000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"fcvtpu", 0x1e290000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"fcvtms", 0x1e300000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"fcvtmu", 0x1e310000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"fcvtzs", 0x1e380000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"fcvtzu", 0x1e390000, 0x7f3ffc00, float2int, 0, FP, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF},
	//{"fmov", 0x9eae0000, 0xfffffc00, float2int, 0, FP, OP2 (Rd, VnD1), QL_XVD1, 0},
	//{"fmov", 0x9eaf0000, 0xfffffc00, float2int, 0, FP, OP2 (VdD1, Rn), QL_VD1X, 0},
	///* Floating-point conditional compare.  */
	//{"fccmp", 0x1e200400, 0xff200c10, floatccmp, 0, FP, OP4 (Fn, Fm, NZCV, COND), QL_FCCMP, F_FPTYPE},
	//{"fccmpe", 0x1e200410, 0xff200c10, floatccmp, 0, FP, OP4 (Fn, Fm, NZCV, COND), QL_FCCMP, F_FPTYPE},
	///* Floating-point compare.  */
	//{"fcmp", 0x1e202000, 0xff20fc1f, floatcmp, 0, FP, OP2 (Fn, Fm), QL_FP2, F_FPTYPE},
	//{"fcmpe", 0x1e202010, 0xff20fc1f, floatcmp, 0, FP, OP2 (Fn, Fm), QL_FP2, F_FPTYPE},
	//{"fcmp", 0x1e202008, 0xff20fc1f, floatcmp, 0, FP, OP2 (Fn, FPIMM0), QL_DST_SD, F_FPTYPE},
	//{"fcmpe", 0x1e202018, 0xff20fc1f, floatcmp, 0, FP, OP2 (Fn, FPIMM0), QL_DST_SD, F_FPTYPE},
	///* Floating-point data-processing (1 source).  */
	//{"fmov", 0x1e204000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	//{"fabs", 0x1e20c000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	//{"fneg", 0x1e214000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	//{"fsqrt", 0x1e21c000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	//{"fcvt", 0x1e224000, 0xff3e7c00, floatdp1, OP_FCVT, FP, OP2 (Fd, Fn), QL_FCVT, F_FPTYPE | F_MISC},
	//{"frintn", 0x1e244000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	//{"frintp", 0x1e24c000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	//{"frintm", 0x1e254000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	//{"frintz", 0x1e25c000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	//{"frinta", 0x1e264000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	//{"frintx", 0x1e274000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	//{"frinti", 0x1e27c000, 0xff3ffc00, floatdp1, 0, FP, OP2 (Fd, Fn), QL_FP2, F_FPTYPE},
	///* Floating-point data-processing (2 source).  */
	//{"fmul", 0x1e200800, 0xff20fc00, floatdp2, 0, FP, OP3 (Fd, Fn, Fm), QL_FP3, F_FPTYPE},
	//{"fdiv", 0x1e201800, 0xff20fc00, floatdp2, 0, FP, OP3 (Fd, Fn, Fm), QL_FP3, F_FPTYPE},
	//{"fadd", 0x1e202800, 0xff20fc00, floatdp2, 0, FP, OP3 (Fd, Fn, Fm), QL_FP3, F_FPTYPE},
	//{"fsub", 0x1e203800, 0xff20fc00, floatdp2, 0, FP, OP3 (Fd, Fn, Fm), QL_FP3, F_FPTYPE},
	//{"fmax", 0x1e204800, 0xff20fc00, floatdp2, 0, FP, OP3 (Fd, Fn, Fm), QL_FP3, F_FPTYPE},
	//{"fmin", 0x1e205800, 0xff20fc00, floatdp2, 0, FP, OP3 (Fd, Fn, Fm), QL_FP3, F_FPTYPE},
	//{"fmaxnm", 0x1e206800, 0xff20fc00, floatdp2, 0, FP, OP3 (Fd, Fn, Fm), QL_FP3, F_FPTYPE},
	//{"fminnm", 0x1e207800, 0xff20fc00, floatdp2, 0, FP, OP3 (Fd, Fn, Fm), QL_FP3, F_FPTYPE},
	//{"fnmul", 0x1e208800, 0xff20fc00, floatdp2, 0, FP, OP3 (Fd, Fn, Fm), QL_FP3, F_FPTYPE},
	///* Floating-point data-processing (3 source).  */
	//{"fmadd", 0x1f000000, 0xff208000, floatdp3, 0, FP, OP4 (Fd, Fn, Fm, Fa), QL_FP4, F_FPTYPE},
	//{"fmsub", 0x1f008000, 0xff208000, floatdp3, 0, FP, OP4 (Fd, Fn, Fm, Fa), QL_FP4, F_FPTYPE},
	//{"fnmadd", 0x1f200000, 0xff208000, floatdp3, 0, FP, OP4 (Fd, Fn, Fm, Fa), QL_FP4, F_FPTYPE},
	//{"fnmsub", 0x1f208000, 0xff208000, floatdp3, 0, FP, OP4 (Fd, Fn, Fm, Fa), QL_FP4, F_FPTYPE},
	///* Floating-point immediate.  */
	//{"fmov", 0x1e201000, 0xff201fe0, floatimm, 0, FP, OP2 (Fd, FPIMM), QL_DST_SD, F_FPTYPE},
	///* Floating-point conditional select.  */
	//{"fcsel", 0x1e200c00, 0xff200c00, floatsel, 0, FP, OP4 (Fd, Fn, Fm, COND), QL_FP_COND, F_FPTYPE},
};

/*
	NOTE: Implementation above is copied from GNU Binutils.
	You must re-implement when you release AArch64 to avoid GPL.
*/

//
// ARMConverter
//

AArch64Converter::AArch64Converter()
{
	AddToOpMap(m_OpDefsBranch, sizeof(m_OpDefsBranch) / sizeof(OpDef));
	AddToOpMap(m_OpDefsLoadStore, sizeof(m_OpDefsLoadStore)/sizeof(OpDef));
	AddToOpMap(m_OpDefsDataProcess, sizeof(m_OpDefsDataProcess) / sizeof(OpDef));
	//AddToOpMap(m_OpDefsSIMD, sizeof(m_OpDefsSIMD) / sizeof(OpDef));
	//AddToOpMap(m_OpDefsFP, sizeof(m_OpDefsFP) / sizeof(OpDef));
	AddToOpMap(m_CreatedOpDefs, m_CreatedOpDefsSize);
	if (IsSplitLoadStoreEnabled()) {
		// <TODO> split load/store
		cerr << "warning: split load/store not yet implemented" << endl;
		//AddToOpMap(m_OpDefsNonSplitLoadStore, sizeof(m_OpDefsNonSplitLoadStore)/sizeof(OpDef));
		//AddToOpMap(m_OpDefsSplitLoadStore, sizeof(m_OpDefsSplitLoadStore)/sizeof(OpDef));
	}
	else{
		//AddToOpMap(m_OpDefsNonSplitLoadStore, sizeof(m_OpDefsNonSplitLoadStore)/sizeof(OpDef));
	}
}

AArch64Converter::~AArch64Converter()
{
}

// srcTemplate に対応するオペランドの種類と，レジスタならば番号を，即値ならばindexを返す
std::pair<AArch64Converter::OperandType, int> AArch64Converter::GetActualSrcOperand(int srcTemplate, const DecodedInsn& decoded) const
{
	typedef std::pair<OperandType, int> RetType;
	if (srcTemplate == -1) {
		return RetType(OpInfoType::NONE, 0);
	}
	else if (ImmTemplateBegin <= srcTemplate && srcTemplate <= ImmTemplateEnd) {
		return RetType(OpInfoType::IMM, srcTemplate - ImmTemplateBegin);
	}
	else  {
		return RetType(OpInfoType::REG, GetActualRegNumber(srcTemplate, decoded) );
	}
}

// regTemplate から実際のレジスタ番号を取得する
int AArch64Converter::GetActualRegNumber(int regTemplate, const DecodedInsn& decoded) const
{
	if (regTemplate == -1) {
		return -1;
	}
	else if (RegTemplateBegin <= regTemplate && regTemplate <= RegTemplateEnd) {
		return decoded.Reg[regTemplate - RegTemplateBegin];
	}
	else if (0 <= regTemplate && regTemplate < AArch64Info::RegisterCount) {
		return regTemplate;
	}
	else {
		ASSERT(0, "ARMConverter Logic Error : invalid regTemplate");
		return -1;
	}
}

// レジスタ番号regNumがゼロレジスタかどうかを判定する
bool AArch64Converter::IsZeroReg(int regNum) const
{
	return regNum == 31;
}


void AArch64Converter::AArch64UnknownOperation(OpEmulationState* opState)
{
	u32 codeWord = static_cast<u32>( SrcOperand<0>()(opState) );

	DecoderType decoder;
	DecodedInsn decoded;
	decoder.Decode( codeWord, &decoded);

	stringstream ss;
	u32 opcode = (codeWord >> 21) & 0x7f;
	ss << "unknown instruction : " << hex << setw(8) << codeWord << endl;
	ss << "\topcode : " << hex << opcode << endl;
	ss << "\tregs : " << hex;
	copy(decoded.Reg.begin(), decoded.Reg.end(), ostream_iterator<int>(ss, ", "));
	ss << endl;
	ss << "\timms : " << hex;
	copy(decoded.Imm.begin(), decoded.Imm.end(), ostream_iterator<u64>(ss, ", "));
	ss << endl;

	THROW_RUNTIME_ERROR(ss.str().c_str());
}

const AArch64Converter::OpDef* AArch64Converter::GetOpDefUnknown() const
{
	return &m_OpDefUnknown;
}

void AArch64Converter::CreateOpDefs(OpDef* partialBase, int baseSize, OpDef* partialExtra, int extraSize)
{
	for(int i = 0; i < baseSize; i++){
		for(int j = 0; j < extraSize; j++){
			OpDef opDef;
			string name = string(string(partialBase[i].Name) + string(partialExtra[j].Name));
			m_OpDefName.push_back(name);
			opDef.Name = m_OpDefName.back().c_str();
			opDef.Mask = partialBase[i].Mask | partialExtra[j].Mask;
			opDef.Opcode = partialBase[i].Opcode | partialExtra[j].Opcode;
			opDef.nOpInfoDefs = partialBase[i].nOpInfoDefs + partialExtra[j].nOpInfoDefs;
			int nDefs = 0;
			for(int k = 0; k < partialExtra[j].nOpInfoDefs; k++, nDefs++){
				opDef.OpInfoDefs[nDefs] = partialExtra[j].OpInfoDefs[k];
			}
			for(int k = 0; k < partialBase[i].nOpInfoDefs; k++, nDefs++){
				opDef.OpInfoDefs[nDefs] = partialBase[i].OpInfoDefs[k];
			}
			m_OpDefVector.push_back(opDef);
		}
	}
}

AArch64Converter::OpDef* AArch64Converter::FinalizeOpDefs()
{
	int size = (int)m_OpDefVector.size();
	m_CreatedOpDefsSize = size;
	m_CreatedOpDefs = new OpDef[size];
	for(unsigned int i = 0; i < m_OpDefVector.size(); i++){
		m_CreatedOpDefs[i] = m_OpDefVector[i];
	}
	return m_CreatedOpDefs;
}
