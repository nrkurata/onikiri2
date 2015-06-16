#ifndef __AARCH64LINUX_AARCH64OPERATION_H__
#define __AARCH64LINUX_AARCH64OPERATION_H__

#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/ProcessState.h"
#include "SysDeps/fenv.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"


namespace Onikiri {
namespace AArch64Linux {
namespace Operation {

using namespace EmulatorUtility::Operation;

template <int OperandIndex, typename SFType>
struct AArch64SrcOperand : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	typedef RegisterType type;
	RegisterType operator()(EmulatorUtility::OpEmulationState* opState)
	{
		type src = opState->GetSrc(OperandIndex);
		return SFType()(opState) ? src : static_cast<u64>(static_cast<u32>(src));
	}
};

template <int OperandIndex, typename SFType>
class AArch64DstOperand
{
public:
	typedef RegisterType type;
	static void SetOperand(EmulatorUtility::OpEmulationState* opState, RegisterType value)
	{
		value = SFType()(opState) ? value : static_cast<u64>(static_cast<u32>(value));
		opState->SetDst(OperandIndex, value);
	}
};


template <typename Type, typename TSrc, typename TBegin, typename TConcat>
struct AArch64BitConcateNate : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState)
	{
		Type value = static_cast<Type>(TSrc()(opState));
		Type concat = static_cast<Type>(TConcat()(opState));

		return (value | (concat << (size_t)TBegin()(opState)));
	}
};


template <typename Type, typename TSrc, typename TPos>
struct AArch64BitTest : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState)
	{
		Type value = static_cast<Type>(TSrc()(opState));
		return (value & ((Type)1 << (size_t)TPos()(opState)));
	}
};

template <typename Type>
struct AArch64CurrentPC : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState)
	{
		return static_cast<Type>current_pc(opState);
	}
};

template <typename TSrc, typename TFrom, typename TTo>
struct AArch64Sext : public std::unary_function<EmulatorUtility::OpEmulationState*, TTo>
{
	TTo operator()(EmulatorUtility::OpEmulationState* opState)
	{
		return static_cast<TTo>(cast_to_signed(TSrc()(opState)));
	}
};

// calculate condition

template <typename Type>
inline Type AArch64CondN(Type CPSR)
{
	return (CPSR >> 31) & 1;
}

template <typename Type>
inline Type AArch64CondZ(Type CPSR)
{
	return (CPSR >> 30) & 1;
}

template <typename Type>
inline Type AArch64CondC(Type CPSR)
{
	return (CPSR >> 29) & 1;
}

template <typename Type>
inline Type AArch64CondV(Type CPSR)
{
	return (CPSR >> 28) & 1;
}

template <typename TCond, typename TCPSR>
struct AArch64CondCalc : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	enum{
		EQ = 0, NE, CS, CC,
		MI, PL, VS, VC,
		HI, LS, GE, LT,
		GT, LE, AL, NV
	};

	bool operator()(EmulatorUtility::OpEmulationState* opState) const
	{
		switch (TCond()(opState)){
		case EQ:
			return AArch64CondZ(TCPSR()(opState)) == 1;
		case NE:
			return !AArch64CondZ(TCPSR()(opState));
		case CS:
			return AArch64CondC(TCPSR()(opState)) == 1;
		case CC:
			return !AArch64CondC(TCPSR()(opState));
		case MI:
			return AArch64CondN(TCPSR()(opState)) == 1;
		case PL:
			return !AArch64CondN(TCPSR()(opState));
		case VS:
			return AArch64CondV(TCPSR()(opState)) == 1;
		case VC:
			return !AArch64CondV(TCPSR()(opState));
		case HI:
			return AArch64CondC(TCPSR()(opState)) && !AArch64CondZ(TCPSR()(opState));
		case LS:
			return !AArch64CondC(TCPSR()(opState)) || AArch64CondZ(TCPSR()(opState));
		case GE:
			return AArch64CondN(TCPSR()(opState)) == AArch64CondV(TCPSR()(opState));
		case LT:
			return AArch64CondN(TCPSR()(opState)) != AArch64CondV(TCPSR()(opState));
		case GT:
			return !AArch64CondZ(TCPSR()(opState)) &&
				(AArch64CondN(TCPSR()(opState)) == AArch64CondV(TCPSR()(opState)));
		case LE:
			return AArch64CondZ(TCPSR()(opState)) ||
				(AArch64CondN(TCPSR()(opState)) != AArch64CondV(TCPSR()(opState)));
		case AL:
			return true;
		case NV:
			// never reached
			return false;
		default:
			// never reached
			return false;
		}
	}
};

/*
// N, Zフラグを計算する
template <typename Type>
inline Type AArch64CalcFlagNZ(Type result)
{
	u32 CPSR = 0;
	CPSR = (result & (1 << 31)) | ((result ? 0 : 1) << 30); 

	return CPSR;
}

template <typename Type, typename TSrc1, typename TSrc2>
struct AArch64NotBorrowFrom : public std::unary_function<EmulatorUtility::OpEmulationState*, RegisterType>
{
	u32 operator()(EmulatorUtility::OpEmulationState* opState)
	{
		u32 lhs = static_cast<u32>( TSrc1()(opState) );
		u32 rhs = static_cast<u32>( TSrc2()(opState) );

		if (lhs < rhs)
			return 0;
		else
			return 1;
	}
};

template <typename Type, typename TSrc1, typename TSrc2, typename TPSR>
struct AArch64NotBorrowFromWithBorrow : public std::unary_function<EmulatorUtility::OpEmulationState*, RegisterType>
{
	u32 operator()(EmulatorUtility::OpEmulationState* opState)
	{
		u32 lhs = static_cast<u32>( TSrc1()(opState) );
		u32 rhs = static_cast<u32>( TSrc2()(opState) );
		Type borrow = static_cast<Type>( ((TPSR()(opState) >> 29) & 1) ^ 1 );

		if (lhs < rhs + borrow)
			return 0;
		else
			return 1;
	}
};

template <typename Type, typename TSrc1, typename TSrc2>
struct AArch64SubOverflowFrom : public std::unary_function<EmulatorUtility::OpEmulationState*, RegisterType>
{
	u32 operator()(EmulatorUtility::OpEmulationState* opState)
	{
		u32 lhs = static_cast<u32>( TSrc1()(opState) );
		u32 rhs = static_cast<u32>( TSrc2()(opState) );

		rhs = ~(rhs) + 1;

		u32 overflow = (lhs + rhs) >> 31;
		if (lhs + rhs < rhs){
			if(overflow) return 1;
			else return 0;
		}
		else{
			if(overflow) return 0;
			else return 1;
		}
	}
};

template <typename Type, typename TSrc1, typename TSrc2, typename TPSR>
struct AArch64SubOverflowFromWithBorrow : public std::unary_function<EmulatorUtility::OpEmulationState*, RegisterType>
{
	u32 operator()(EmulatorUtility::OpEmulationState* opState)
	{
		u32 lhs = static_cast<u32>( TSrc1()(opState) );
		u32 rhs = static_cast<u32>( TSrc2()(opState) );
		Type borrow = static_cast<Type>( ((TPSR()(opState) >> 29) & 1) ^ 1 );

		rhs = ~(rhs) + 1;
		borrow = ~(borrow) + 1;

		u32 overflow = (lhs + rhs + borrow) >> 31;
		if (lhs + rhs + borrow < rhs){
			if(overflow) return 1;
			else return 0;
		}
		else{
			if(overflow) return 0;
			else return 1;
		}
	}
};

template <typename Type, typename TSrc1, typename TSrc2>
struct AArch64AddOverflowFrom : public std::unary_function<EmulatorUtility::OpEmulationState*, RegisterType>
{
	u32 operator()(EmulatorUtility::OpEmulationState* opState)
	{
		u32 lhs = static_cast<u32>( TSrc1()(opState) );
		u32 rhs = static_cast<u32>( TSrc2()(opState) );

		u32 overflow = (lhs + rhs) >> 31;
		if (lhs + rhs < rhs){
			if(overflow) return 1;
			else return 0;
		}
		else{
			if(overflow) return 0;
			else return 1;
		}
	}
};

template <typename Type, typename TSrc1, typename TSrc2, typename TPSR>
struct AArch64AddOverflowFromWithCarry : public std::unary_function<EmulatorUtility::OpEmulationState*, RegisterType>
{
	u32 operator()(EmulatorUtility::OpEmulationState* opState)
	{
		u32 lhs = static_cast<u32>( TSrc1()(opState) );
		u32 rhs = static_cast<u32>( TSrc2()(opState) );
		Type carry = static_cast<Type>( (TPSR()(opState) >> 29) & 1 );

		u32 overflow = (lhs + rhs + carry) >> 31;
		if (lhs + rhs + carry < rhs){
			if(overflow) return 1;
			else return 0;
		}
		else{
			if(overflow) return 0;
			else return 1;
		}
	}
};


// Type must be unsigned
template <typename Type, typename TSrc1, typename TSrc2, typename TPSR>
struct AArch64CarryOfAddWithCarry : public std::unary_function<EmulatorUtility::OpEmulationState*, RegisterType>
{
	RegisterType operator()(EmulatorUtility::OpEmulationState* opState)
	{
		Type lhs = static_cast<Type>( TSrc1()(opState) );
		Type rhs = static_cast<Type>( TSrc2()(opState) );
		Type carry = static_cast<Type>( (TPSR()(opState) >> 29) & 1 );

		if (lhs + rhs < rhs || lhs + rhs + carry < rhs)	// lhs, rhs がともに~0の場合 lhs + rhs + 1 < rhs は成立しない (ex. 255 + 255 + 1 = 255)
			return 1;
		else
			return 0;
	}
};

template <typename TShifterOperand>
struct AArch64GetCarryOut : public std::unary_function<EmulatorUtility::OpEmulationState*, u32>
{
	u32 operator()(EmulatorUtility::OpEmulationState* opState)
	{
		return static_cast<u32>(TShifterOperand()(opState) >> 32 & 1);
	}
};

template <typename TPSR>
struct AArch64GetVFlag : public std::unary_function<EmulatorUtility::OpEmulationState*, u32>
{
	u32 operator()(EmulatorUtility::OpEmulationState* opState)
	{
		return static_cast<u32>(TPSR()(opState) >> 28 & 1);
	}
};

template <typename TDest, typename TDestFlag, typename TFunc1, typename TFunc2>
struct AArch64CalcFlagCV : public std::unary_function<EmulatorUtility::OpEmulationState*, u32>
{
	u32 operator()(EmulatorUtility::OpEmulationState* opState)
	{
		return static_cast<u32>((TFunc1()(opState) << 29) | (TFunc2()(opState) << 28));
	}
};

template <typename TFunc, typename TCFlag, typename TVFlag>
struct AArch64CalcFlagBit : public std::unary_function<EmulatorUtility::OpEmulationState*, u32>
{
	u32 operator()(EmulatorUtility::OpEmulationState* opState)
	{
		u32 result = static_cast<u32>(TFunc()(opState));
		return AArch64CalcFlagNZ(result) | (TCFlag()(opState) & (1 << 29)) | (TVFlag()(opState) & (1 << 28));
	}
};

template <typename Type, typename TCond, typename TValue, typename TAddr>
inline void AArch64ConditionalStore(EmulatorUtility::OpEmulationState* opState)
{
	Type writeData;
	if ( TCond()(opState) ) {
		writeData = static_cast<Type>( TValue()(opState) );
	}
	else{
		// シミュレータ的には書き込んだことにしておかないと
		// おかしなことになるので同じ値を書き込む
		writeData = ReadMemory<Type>(opState, TAddr()(opState));
	}
	WriteMemory<Type>(opState, TAddr()(opState), writeData);
};

// TFuncの戻り値を TDest にセットし，戻り値に応じてTDestFlagにフラグを設定
template <typename TDest, typename TDestFlag, typename TFunc, typename TFlagCVFunc>
inline void AArch64SetF(EmulatorUtility::OpEmulationState* opState)
{
	typename TFunc::result_type result = TFunc()(opState);	// 結果のビット数を維持する

	TDest::SetOperand(opState, result);
	TDestFlag::SetOperand(opState, AArch64CalcFlagNZ(result) | TFlagCVFunc()(opState));
}

template <typename Type, typename TCond, typename TTrueValue, typename TFalseValue>
struct AArch64Select : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState)
	{
		Type trueValue = TTrueValue()(opState);
		Type falseValue = TFalseValue()(opState);
		if ( TCond()(opState) ) {
			return trueValue;
		}
		else {
			return falseValue;
		}
	}
};

template <typename TSrcDisp, typename TCond>
inline void AArch64BranchRel(EmulatorUtility::OpEmulationState* opState)
{
	u32 target = static_cast<u32>((current_pc(opState) + 8)
		+ (u32)(TSrcDisp()(opState) << 2));
	opState->SetTakenPC( target );

	if( TCond()(opState) ){
		do_branch(opState, (u64)target);
	}
}

template <typename TDest, typename TSrcDisp, typename TCond>
inline void AArch64CallRel(EmulatorUtility::OpEmulationState* opState)
{
	u32 target = static_cast<u32>((current_pc(opState) + 8)
		+ (u32)(TSrcDisp()(opState) << 2));
	opState->SetTakenPC( target );

	if( TCond()(opState) ){
		do_branch(opState, (u64)target);
		TDest::SetOperand(opState, next_pc(opState));
	}
}

template<typename Type, typename TSrc1, typename TSrc2>
struct AArch64Mov : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState) const
	{
		return static_cast<Type>(TSrc2()(opState));
	}
};

template<typename Type, typename TSrc1, typename TSrc2, typename TPSR>
struct AArch64Cmp : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState) const
	{
		Type result = static_cast<Type>(TSrc1()(opState) - TSrc2()(opState));
		return AArch64CalcFlagNZ(result) | AArch64NotBorrowFrom<u32,TSrc1,TSrc2>()(opState) << 29 | AArch64SubOverflowFrom<u32,TSrc1,TSrc2>()(opState) << 28;
	}
};

template<typename Type, typename TSrc1, typename TSrc2, typename TPSR>
struct AArch64Cmn : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState) const
	{
		Type result = static_cast<Type>(TSrc1()(opState) + TSrc2()(opState));
		return static_cast<Type>(AArch64CalcFlagNZ(result) | CarryOfAdd<Type,TSrc1,TSrc2>()(opState) << 29 | AArch64AddOverflowFrom<Type,TSrc1,TSrc2>()(opState) << 28);
	}
};

template<typename Type, typename TSrc1, typename TSrc2, typename TPSR>
struct AArch64Tst : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState) const
	{
		Type result = static_cast<Type>(TSrc1()(opState) & TSrc2()(opState));
		return AArch64CalcFlagNZ(result) | (AArch64GetCarryOut<TSrc2>()(opState) << 29) | (AArch64GetVFlag<TPSR>()(opState) << 28);
	}
};

template<typename Type, typename TSrc1, typename TSrc2, typename TPSR>
struct AArch64Teq : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState) const
	{
		Type result = static_cast<Type>(TSrc1()(opState) ^ TSrc2()(opState));
		return AArch64CalcFlagNZ(result) | (AArch64GetCarryOut<TSrc2>()(opState) << 29) | (AArch64GetVFlag<TPSR>()(opState) << 28);
	}
};

// Load/Store
template<typename TSrc1, typename TSrc2>
struct AArch64AddrAdd : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	RegisterType operator()(EmulatorUtility::OpEmulationState* opState) const
	{
		return TSrc1()(opState) + TSrc2()(opState);
	}
};

template<typename TSrc1, typename TSrc2>
struct AArch64AddrSub : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	RegisterType operator()(EmulatorUtility::OpEmulationState* opState) const
	{
		return TSrc1()(opState) - TSrc2()(opState);
	}
};


template <typename Type, typename TSrc1, typename TSrc2, typename TPSR>
struct AArch64IntAddwithCarry : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState)
	{
		return static_cast<Type>(TSrc1()(opState)) + static_cast<Type>(TSrc2()(opState)) + static_cast<Type>((TPSR()(opState) >> 29) & 1);
	}
};

template <typename Type, typename TSrc1, typename TSrc2, typename TPSR>
struct AArch64IntSubwithBorrow : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState)
	{
		return static_cast<Type>(TSrc1()(opState)) - static_cast<Type>(TSrc2()(opState)) - static_cast<Type>(((TPSR()(opState) >> 29) & 1) ^ 1);
	}
};


template <int OperandIndex>
struct AArch64SrcOperand : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	typedef RegisterType type;
	RegisterType operator()(EmulatorUtility::OpEmulationState* opState)
	{
		CommonOpInfo<AArch64Info> *info = dynamic_cast<CommonOpInfo<AArch64Info> *>(opState->GetOpInfo());
		for(int i = 0; i < info->GetSrcRegNum(); i++){
			if( info->GetSrcRegOpMap(i) == OperandIndex ){
				return (info->GetSrcOperand(OperandIndex) == 0xf) ?
					opState->GetPC() + 8 : opState->GetSrc(OperandIndex);
			}
		}
		return opState->GetSrc(OperandIndex);
	}
};

template <typename Type, typename TSrc1, typename TSrc2, typename TSftType>
struct AArch64ShiftOperation : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
	Type operator()(EmulatorUtility::OpEmulationState* opState)
	{
		Type result = 0;
		switch (TSftType()(opState))
		{
		case 0:
			result = LShiftL<Type, TSrc1, TSrc2, 0x3f>()(opState);
			break;
		case 1:
			result = LShiftR<Type, TSrc1, TSrc2, 0x3f>()(opState);
			break;
		case 2:
			result = AShiftR<Type, TSrc1, TSrc2, 0x3f>()(opState);
			break;
		case 3:
			result = RotateR<Type, TSrc1, TSrc2>()(opState);
			break;
		}
		return result;
	}
};

*/
} // namespace Operation {
} // namespace ARMLinux {
} // namespace Onikiri


#endif
