/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */


#include "PrecompiledHeader.h"

#include "Common.h"
#include "R5900OpcodeTables.h"
#include "iR5900LoadStore.h"
#include "iR5900.h"

using namespace x86Emitter;

#define REC_STORES
#define REC_LOADS

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl {

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/
#ifndef LOADSTORE_RECOMPILE

namespace Interp = R5900::Interpreter::OpcodeImpl;

REC_FUNC_DEL(LB,  _Rt_);
REC_FUNC_DEL(LBU, _Rt_);
REC_FUNC_DEL(LH,  _Rt_);
REC_FUNC_DEL(LHU, _Rt_);
REC_FUNC_DEL(LW,  _Rt_);
REC_FUNC_DEL(LWU, _Rt_);
REC_FUNC_DEL(LWL, _Rt_);
REC_FUNC_DEL(LWR, _Rt_);
REC_FUNC_DEL(LD,  _Rt_);
REC_FUNC_DEL(LDR, _Rt_);
REC_FUNC_DEL(LDL, _Rt_);
REC_FUNC_DEL(LQ,  _Rt_);
REC_FUNC(SB);
REC_FUNC(SH);
REC_FUNC(SW);
REC_FUNC(SWL);
REC_FUNC(SWR);
REC_FUNC(SD);
REC_FUNC(SDL);
REC_FUNC(SDR);
REC_FUNC(SQ);
REC_FUNC(LWC1);
REC_FUNC(SWC1);
REC_FUNC(LQC2);
REC_FUNC(SQC2);

#else

__aligned16 u64 retValues[2];

void _eeOnLoadWrite(u32 reg)
{
	int regt;

	if (!reg)
		return;

	_eeOnWriteReg(reg, 1);
	regt = _checkXMMreg(XMMTYPE_GPRREG, reg, MODE_READ);

	if (regt >= 0)
	{
		if (xmmregs[regt].mode & MODE_WRITE)
		{
			if (reg != _Rs_)
			{
				xPUNPCK.HQDQ(xRegisterSSE(regt), xRegisterSSE(regt));
				xMOVQ(ptr[&cpuRegs.GPR.r[reg].UL[2]], xRegisterSSE(regt));
			}
			else
				xMOVH.PS(ptr[&cpuRegs.GPR.r[reg].UL[2]], xRegisterSSE(regt));
		}
		xmmregs[regt].inuse = 0;
	}
}

using namespace Interpreter::OpcodeImpl;

__aligned16 u32 dummyValue[4];

//////////////////////////////////////////////////////////////////////////////////////////
//
void recLoad64(u32 bits, bool sign)
{
	pxAssume(bits == 64 || bits == 128);

	// Load arg2 with the destination.
	// 64/128 bit modes load the result directly into the cpuRegs.GPR struct.

	if (_Rt_)
		xLEA(arg2reg, ptr[&cpuRegs.GPR.r[_Rt_].UL[0]]);
	else
		xLEA(arg2reg, ptr[&dummyValue[0]]);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		if (bits == 128)
			srcadr &= ~0x0f;

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		vtlb_DynGenRead64_Const(bits, srcadr);
	}
	else
	{
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);
		if (bits == 128) // force 16 byte alignment on 128 bit reads
			xAND(arg1regd, ~0x0F);

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);
		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenRead64(bits);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void recLoad32(u32 bits, bool sign)
{
	pxAssume(bits <= 32);

	// 8/16/32 bit modes return the loaded value in EAX.

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		vtlb_DynGenRead32_Const(bits, sign, srcadr);
	}
	else
	{
		// Load arg1 with the source memory address that we're reading from.
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		iFlushCall(FLUSH_FULLVTLB);
		vtlb_DynGenRead32(bits, sign);
	}

	if (_Rt_)
	{
		// EAX holds the loaded value, so sign extend as needed:
		if (sign)
			xCDQ();

		xMOV(ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]], eax);
		if (sign)
			xMOV(ptr32[&cpuRegs.GPR.r[_Rt_].UL[1]], edx);
		else
			xMOV(ptr32[&cpuRegs.GPR.r[_Rt_].UL[1]], 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//

void recStore(u32 bits)
{
	// Performance note: Const prop for the store address is good, always.
	// Constprop for the value being stored is not really worthwhile (better to use register
	// allocation -- simpler code and just as fast)

	// Load EDX first with the value being written, or the address of the value
	// being written (64/128 bit modes).

	if (bits < 64)
	{
		_eeMoveGPRtoR(arg2regd, _Rt_);
	}
	else if (bits == 128 || bits == 64)
	{
		_flushEEreg(_Rt_); // flush register to mem
		xLEA(arg2reg, ptr[&cpuRegs.GPR.r[_Rt_].UL[0]]);
	}

	// Load ECX with the destination address, or issue a direct optimized write
	// if the address is a constant propagation.

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 dstadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		if (bits == 128)
			dstadr &= ~0x0f;

		vtlb_DynGenWrite_Const(bits, dstadr);
	}
	else
	{
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);
		if (bits == 128)
			xAND(arg1regd, ~0x0F);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenWrite(bits);
	}
}


//////////////////////////////////////////////////////////////////////////////////////////
//
void recLB()  { recLoad32(  8, true);  EE::Profiler.EmitOp(eeOpcode::LB); }
void recLBU() { recLoad32(  8, false); EE::Profiler.EmitOp(eeOpcode::LBU); }
void recLH()  { recLoad32( 16, true);  EE::Profiler.EmitOp(eeOpcode::LH); }
void recLHU() { recLoad32( 16, false); EE::Profiler.EmitOp(eeOpcode::LHU); }
void recLW()  { recLoad32( 32, true);  EE::Profiler.EmitOp(eeOpcode::LW); }
void recLWU() { recLoad32( 32, false); EE::Profiler.EmitOp(eeOpcode::LWU); }
void recLD()  { recLoad64( 64, false); EE::Profiler.EmitOp(eeOpcode::LD); }
void recLQ()  { recLoad64(128, false); EE::Profiler.EmitOp(eeOpcode::LQ); }

void recSB()  { recStore(  8); EE::Profiler.EmitOp(eeOpcode::SB); }
void recSH()  { recStore( 16); EE::Profiler.EmitOp(eeOpcode::SH); }
void recSW()  { recStore( 32); EE::Profiler.EmitOp(eeOpcode::SW); }
void recSD()  { recStore( 64); EE::Profiler.EmitOp(eeOpcode::SD); }
void recSQ()  { recStore(128); EE::Profiler.EmitOp(eeOpcode::SQ); }

////////////////////////////////////////////////////

void recLWL()
{
#ifdef REC_LOADS
	iFlushCall(FLUSH_FULLVTLB);
	_deleteEEreg(_Rt_, 1);

	_eeMoveGPRtoR(arg1regd, _Rs_);
	if (_Imm_ != 0)
		xADD(arg1regd, _Imm_);

	// calleeSavedReg1 = bit offset in word
	xMOV(calleeSavedReg1d, arg1regd);
	xAND(calleeSavedReg1d, 3);
	xSHL(calleeSavedReg1d, 3);

	xAND(arg1regd, ~3);
	vtlb_DynGenRead32(32, false);

	if (!_Rt_)
		return;

	// mask off bytes loaded
	xMOV(ecx, calleeSavedReg1d);
	xMOV(edx, 0xffffff);
	xSHR(edx, cl);
	xAND(ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]], edx);

	// OR in bytes loaded
	xNEG(ecx);
	xADD(ecx, 24);
	xSHL(eax, cl);
	xOR(ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]], eax);

	// eax will always have the sign bit
	xCDQ();
	xMOV(ptr32[&cpuRegs.GPR.r[_Rt_].UL[1]], edx);
#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);

	recCall(LWL);
#endif

	EE::Profiler.EmitOp(eeOpcode::LWL);
}

////////////////////////////////////////////////////
void recLWR()
{
#ifdef REC_LOADS
	iFlushCall(FLUSH_FULLVTLB);
	_deleteEEreg(_Rt_, 1);

	_eeMoveGPRtoR(arg1regd, _Rs_);
	if (_Imm_ != 0)
		xADD(arg1regd, _Imm_);

	// edi = bit offset in word
	xMOV(calleeSavedReg1d, arg1regd);
	xAND(calleeSavedReg1d, 3);
	xSHL(calleeSavedReg1d, 3);

	xAND(arg1regd, ~3);
	vtlb_DynGenRead32(32, false);

	if (!_Rt_)
		return;

	// mask off bytes loaded
	xMOV(ecx, 24);
	xSUB(ecx, calleeSavedReg1d);
	xMOV(edx, 0xffffff00);
	xSHL(edx, cl);
	xAND(ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]], edx);

	// OR in bytes loaded
	xMOV(ecx, calleeSavedReg1d);
	xSHR(eax, cl);
	xOR(ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]], eax);

	xCMP(ecx, 0);
	xForwardJump8 nosignextend(Jcc_NotEqual);
	// if ((addr & 3) == 0)
	xCDQ();
	xMOV(ptr32[&cpuRegs.GPR.r[_Rt_].UL[1]], edx);
	nosignextend.SetTarget();
#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);

	recCall(LWR);
#endif

	EE::Profiler.EmitOp(eeOpcode::LWR);
}

////////////////////////////////////////////////////
void recSWL()
{
#ifdef REC_STORES
	iFlushCall(FLUSH_FULLVTLB);

	_eeMoveGPRtoR(arg1regd, _Rs_);
	if (_Imm_ != 0)
		xADD(arg1regd, _Imm_);

	// edi = bit offset in word
	xMOV(calleeSavedReg1d, arg1regd);
	xAND(calleeSavedReg1d, 3);
	xSHL(calleeSavedReg1d, 3);

	xAND(arg1regd, ~3);
	vtlb_DynGenRead32(32, false);

	// mask read -> arg2
	xMOV(ecx, calleeSavedReg1d);
	xMOV(arg2regd, 0xffffff00);
	xSHL(arg2regd, cl);
	xAND(arg2regd, eax);

	if (_Rt_)
	{
		// mask write and OR -> edx
		xNEG(ecx);
		xADD(ecx, 24);
		_eeMoveGPRtoR(eax, _Rt_);
		xSHR(eax, cl);
		xOR(arg2regd, eax);
	}

	_eeMoveGPRtoR(arg1regd, _Rs_);
	if (_Imm_ != 0)
		xADD(arg1regd, _Imm_);
	xAND(arg1regd, ~3);

	vtlb_DynGenWrite(32);
#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SWL);
#endif

	EE::Profiler.EmitOp(eeOpcode::SWL);
}

////////////////////////////////////////////////////
void recSWR()
{
#ifdef REC_STORES
	iFlushCall(FLUSH_FULLVTLB);

	_eeMoveGPRtoR(arg1regd, _Rs_);
	if (_Imm_ != 0)
		xADD(arg1regd, _Imm_);

	// edi = bit offset in word
	xMOV(calleeSavedReg1d, arg1regd);
	xAND(calleeSavedReg1d, 3);
	xSHL(calleeSavedReg1d, 3);

	xAND(arg1regd, ~3);
	vtlb_DynGenRead32(32, false);

	// mask read -> edx
	xMOV(ecx, 24);
	xSUB(ecx, calleeSavedReg1d);
	xMOV(arg2regd, 0xffffff);
	xSHR(arg2regd, cl);
	xAND(arg2regd, eax);

	if (_Rt_)
	{
		// mask write and OR -> edx
		xMOV(ecx, calleeSavedReg1d);
		_eeMoveGPRtoR(eax, _Rt_);
		xSHL(eax, cl);
		xOR(arg2regd, eax);
	}

	_eeMoveGPRtoR(arg1regd, _Rs_);
	if (_Imm_ != 0)
		xADD(arg1regd, _Imm_);
	xAND(arg1regd, ~3);

	vtlb_DynGenWrite(32);
#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SWR);
#endif

	EE::Profiler.EmitOp(eeOpcode::SWR);
}

////////////////////////////////////////////////////

alignas(16) const u32 SHIFT_MASKS[2][4] = {
	{ 0xffffffff, 0xffffffff, 0x00000000, 0x00000000 },
	{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
};

void recLDL()
{
	if (!_Rt_)
		return;

#ifdef LOADSTORE_RECOMPILE
	xLEA(arg2reg, ptr128[&dummyValue[0]]);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		srcadr &= ~0x07;

		vtlb_DynGenRead64_Const(64, srcadr);
	}
	else
	{
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		xAND(arg1regd, ~0x07);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenRead64(64);
	}
	
	int rtreg = _allocGPRtoXMMreg(-1, _Rt_, MODE_READ | MODE_WRITE);
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);
	int t2reg = _allocTempXMMreg(XMMT_INT, -1);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 shiftval = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		shiftval &= 0x7;
		xMOV(eax, shiftval + 1);
	}
	else
	{
		_eeMoveGPRtoR(eax, _Rs_);
		if (_Imm_ != 0)
			xADD(eax, _Imm_);
		xAND(eax, 0x7);
		xADD(eax, 1);
	}

	xCMP(eax, 8);
	xForwardJE32 skip;
	//Calculate the shift from top bit to lowest
	xMOV(edx, 64);
	xSHL(eax, 3);
	xSUB(edx, eax);

	xMOVDZX(xRegisterSSE(t1reg), eax);
	xMOVQZX(xRegisterSSE(t0reg), ptr128[&SHIFT_MASKS[0][0]]);
	xPSRL.Q(xRegisterSSE(t0reg), xRegisterSSE(t1reg));
	xPAND(xRegisterSSE(t0reg), xRegisterSSE(rtreg));
	xMOVDQA(xRegisterSSE(t2reg), xRegisterSSE(t0reg));

	xMOVDZX(xRegisterSSE(t1reg), edx);
	xMOVQZX(xRegisterSSE(t0reg), ptr128[&dummyValue[0]]);
	xPSLL.Q(xRegisterSSE(t0reg), xRegisterSSE(t1reg));
	xPOR(xRegisterSSE(t0reg), xRegisterSSE(t2reg));
	xForwardJump32 full;
	skip.SetTarget();

	xMOVQZX(xRegisterSSE(t0reg), ptr128[&dummyValue[0]]);
	full.SetTarget();

	xBLEND.PS(xRegisterSSE(rtreg), xRegisterSSE(t0reg), 0x3);

	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
	_freeXMMreg(t2reg);

#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(LDL);
#endif

	EE::Profiler.EmitOp(eeOpcode::LDL);
}

////////////////////////////////////////////////////
void recLDR()
{
	if (!_Rt_)
		return;

#ifdef LOADSTORE_RECOMPILE
	xLEA(arg2reg, ptr128[&dummyValue[0]]);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		srcadr &= ~0x07;

		vtlb_DynGenRead64_Const(64, srcadr);
	}
	else
	{
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		xAND(arg1regd, ~0x07);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenRead64(64);
	}

	int rtreg = _allocGPRtoXMMreg(-1, _Rt_, MODE_READ | MODE_WRITE);
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);
	int t2reg = _allocTempXMMreg(XMMT_INT, -1);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 shiftval = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		shiftval &= 0x7;
		xMOV(eax, shiftval);
	}
	else
	{
		_eeMoveGPRtoR(eax, _Rs_);
		if (_Imm_ != 0)
			xADD(eax, _Imm_);
		xAND(eax, 0x7);
	}

	xCMP(eax, 0);
	xForwardJE32 skip;
	//Calculate the shift from top bit to lowest
	xMOV(edx, 64);
	xSHL(eax, 3);
	xSUB(edx, eax);

	xMOVDZX(xRegisterSSE(t1reg), edx); //64-shift*8
	xMOVQZX(xRegisterSSE(t0reg), ptr128[&SHIFT_MASKS[0][0]]);
	xPSLL.Q(xRegisterSSE(t0reg), xRegisterSSE(t1reg));
	xPAND(xRegisterSSE(t0reg), xRegisterSSE(rtreg));
	xMOVQZX(xRegisterSSE(t2reg), xRegisterSSE(t0reg));

	xMOVDZX(xRegisterSSE(t1reg), eax); //shift*8
	xMOVQZX(xRegisterSSE(t0reg), ptr128[&dummyValue[0]]);
	xPSRL.Q(xRegisterSSE(t0reg), xRegisterSSE(t1reg));
	xPOR(xRegisterSSE(t0reg), xRegisterSSE(t2reg));
	xForwardJump32 full;
	skip.SetTarget();

	xMOVQZX(xRegisterSSE(t0reg), ptr128[&dummyValue[0]]);
	full.SetTarget();

	xBLEND.PS(xRegisterSSE(rtreg), xRegisterSSE(t0reg), 0x3);

	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
	_freeXMMreg(t2reg);

#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(LDR);
#endif

	EE::Profiler.EmitOp(eeOpcode::LDR);
}

////////////////////////////////////////////////////

void recSDL()
{
#ifdef LOADSTORE_RECOMPILE
	xLEA(arg2reg, ptr128[&dummyValue[0]]);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		srcadr &= ~0x07;

		vtlb_DynGenRead64_Const(64, srcadr);
	}
	else
	{
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		xAND(arg1regd, ~0x07);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenRead64(64);
	}
	_flushEEreg(_Rt_); // flush register to mem

	int rtreg = _allocGPRtoXMMreg(-1, _Rt_, MODE_READ);
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 shiftval = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		shiftval &= 0x7;
		xMOV(eax, shiftval + 1);
	}
	else
	{
		_eeMoveGPRtoR(eax, _Rs_);
		if (_Imm_ != 0)
			xADD(eax, _Imm_);
		xAND(eax, 0x7);
		xADD(eax, 1);
	}

	xCMP(eax, 8);
	xForwardJE32 skip;
	//Calculate the shift from top bit to lowest
	xMOV(edx, 64);
	xSHL(eax, 3);
	xSUB(edx, eax);
	// Generate mask 128-(shiftx8) xPSRA.W does bit for bit
	xMOVDZX(xRegisterSSE(t1reg), eax);
	xMOVQZX(xRegisterSSE(t0reg), ptr128[&SHIFT_MASKS[0][0]]);
	xPSLL.Q(xRegisterSSE(t0reg), xRegisterSSE(t1reg));
	xMOVQZX(xRegisterSSE(t1reg), ptr128[&dummyValue[0]]); // This line is super slow, but using MOVDQA/MOVAPS is even slower!
	xPAND(xRegisterSSE(t0reg), xRegisterSSE(t1reg));

	// Shift over reg value (shift, PSLL.Q multiplies by 8)
	xMOVDZX(xRegisterSSE(t1reg), edx);
	xPSRL.Q(xRegisterSSE(rtreg), xRegisterSSE(t1reg));
	xPOR(xRegisterSSE(rtreg), xRegisterSSE(t0reg));
	skip.SetTarget();

	xMOVQ(ptr128[&dummyValue[0]], xRegisterSSE(rtreg));

	_deleteGPRtoXMMreg(_Rt_, 3);
	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);

	xLEA(arg2reg, ptr128[&dummyValue[0]]);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 dstadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		dstadr &= ~0x07;

		vtlb_DynGenWrite_Const(64, dstadr);
	}
	else
	{
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		xAND(arg1regd, ~0x7);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenWrite(64);
	}
#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SDL);
#endif
	EE::Profiler.EmitOp(eeOpcode::SDL);
}

////////////////////////////////////////////////////
void recSDR()
{
#ifdef LOADSTORE_RECOMPILE
	xLEA(arg2reg, ptr128[&dummyValue[0]]);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		srcadr &= ~0x07;

		vtlb_DynGenRead64_Const(64, srcadr);
	}
	else
	{
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		xAND(arg1regd, ~0x07);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenRead64(64);
	}
	_flushEEreg(_Rt_); // flush register to mem

	int rtreg = _allocGPRtoXMMreg(-1, _Rt_, MODE_READ);
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 shiftval = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		shiftval &= 0x7;
		xMOV(eax, shiftval);
	}
	else
	{
		_eeMoveGPRtoR(eax, _Rs_);
		if (_Imm_ != 0)
			xADD(eax, _Imm_);
		xAND(eax, 0x7);
	}

	xCMP(eax, 0);
	xForwardJE32 skip;
	//Calculate the shift from top bit to lowest
	xMOV(edx, 64);
	xSHL(eax, 3);
	xSUB(edx, eax);
	// Generate mask 128-(shiftx8) xPSRA.W does bit for bit
	xMOVDZX(xRegisterSSE(t1reg), edx);
	xMOVQZX(xRegisterSSE(t0reg), ptr128[&SHIFT_MASKS[0][0]]);
	xPSRL.Q(xRegisterSSE(t0reg), xRegisterSSE(t1reg));
	xMOVQZX(xRegisterSSE(t1reg), ptr128[&dummyValue[0]]); // This line is super slow, but using MOVDQA/MOVAPS is even slower!
	xPAND(xRegisterSSE(t0reg), xRegisterSSE(t1reg));

	// Shift over reg value (shift, PSLL.Q multiplies by 8)
	xMOVDZX(xRegisterSSE(t1reg), eax);
	xPSLL.Q(xRegisterSSE(rtreg), xRegisterSSE(t1reg));
	xPOR(xRegisterSSE(rtreg), xRegisterSSE(t0reg));
	skip.SetTarget();

	xMOVQ(ptr128[&dummyValue[0]], xRegisterSSE(rtreg));
	
	_deleteGPRtoXMMreg(_Rt_, 3);
	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);

	xLEA(arg2reg, ptr128[&dummyValue[0]]);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 dstadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		dstadr &= ~0x07;

		vtlb_DynGenWrite_Const(64, dstadr);
	}
	else
	{
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		xAND(arg1regd, ~0x7);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenWrite(64);
	}
#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SDR);
#endif
	EE::Profiler.EmitOp(eeOpcode::SDR);
}

//////////////////////////////////////////////////////////////////////////////////////////
/*********************************************************
* Load and store for COP1                                *
* Format:  OP rt, offset(base)                           *
*********************************************************/

////////////////////////////////////////////////////

void recLWC1()
{
#ifndef FPU_RECOMPILE
	recCall(::R5900::Interpreter::OpcodeImpl::LWC1);
#else
	_deleteFPtoXMMreg(_Rt_, 2);

	if (GPR_IS_CONST1(_Rs_))
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenRead32_Const(32, false, addr);
	}
	else
	{
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenRead32(32, false);
	}

	xMOV(ptr32[&fpuRegs.fpr[_Rt_].UL], eax);

	EE::Profiler.EmitOp(eeOpcode::LWC1);
#endif
}

//////////////////////////////////////////////////////

void recSWC1()
{
#ifndef FPU_RECOMPILE
	recCall(::R5900::Interpreter::OpcodeImpl::SWC1);
#else
	_deleteFPtoXMMreg(_Rt_, 1);

	xMOV(arg2regd, ptr32[&fpuRegs.fpr[_Rt_].UL]);

	if (GPR_IS_CONST1(_Rs_))
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenWrite_Const(32, addr);
	}
	else
	{
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenWrite(32);
	}

	EE::Profiler.EmitOp(eeOpcode::SWC1);
#endif
}

////////////////////////////////////////////////////

/*********************************************************
* Load and store for COP2 (VU0 unit)                     *
* Format:  OP rt, offset(base)                           *
*********************************************************/

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_



void recLQC2()
{
	iFlushCall(FLUSH_EVERYTHING);

	xMOV(eax, ptr32[&cpuRegs.cycle]);
	xADD(eax, scaleblockcycles_clear());
	xMOV(ptr32[&cpuRegs.cycle], eax); // update cycles
	xTEST(ptr32[&VU0.VI[REG_VPU_STAT].UL], 0x1);
	xForwardJZ32 skipvuidle;
	xSUB(eax, ptr32[&VU0.cycle]);
	xSUB(eax, ptr32[&VU0.nextBlockCycles]);
	xCMP(eax, 8);
	xForwardJL32 skip;
	xLoadFarAddr(arg1reg, CpuVU0);
	xFastCall((void*)BaseVUmicroCPU::ExecuteBlockJIT, arg1reg);
	skip.SetTarget();
	skipvuidle.SetTarget();

	if (_Rt_)
		xLEA(arg2reg, ptr[&VU0.VF[_Ft_].UD[0]]);
	else
		xLEA(arg2reg, ptr[&dummyValue[0]]);

	if (GPR_IS_CONST1(_Rs_))
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;

		vtlb_DynGenRead64_Const(128, addr);
	}
	else
	{
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenRead64(128);
	}

	EE::Profiler.EmitOp(eeOpcode::LQC2);
}

////////////////////////////////////////////////////

void recSQC2()
{
	iFlushCall(FLUSH_EVERYTHING);

	xMOV(eax, ptr32[&cpuRegs.cycle]);
	xADD(eax, scaleblockcycles_clear());
	xMOV(ptr32[&cpuRegs.cycle], eax); // update cycles
	xTEST(ptr32[&VU0.VI[REG_VPU_STAT].UL], 0x1);
	xForwardJZ32 skipvuidle;
	xSUB(eax, ptr32[&VU0.cycle]);
	xSUB(eax, ptr32[&VU0.nextBlockCycles]);
	xCMP(eax, 8);
	xForwardJL32 skip;
	xLoadFarAddr(arg1reg, CpuVU0);
	xFastCall((void*)BaseVUmicroCPU::ExecuteBlockJIT, arg1reg);
	skip.SetTarget();
	skipvuidle.SetTarget();

	xLEA(arg2reg, ptr[&VU0.VF[_Ft_].UD[0]]);

	if (GPR_IS_CONST1(_Rs_))
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenWrite_Const(128, addr);
	}
	else
	{
		_eeMoveGPRtoR(arg1regd, _Rs_);
		if (_Imm_ != 0)
			xADD(arg1regd, _Imm_);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenWrite(128);
	}

	EE::Profiler.EmitOp(eeOpcode::SQC2);
}

#endif

} // namespace OpcodeImpl
} // namespace Dynarec
} // namespace R5900
