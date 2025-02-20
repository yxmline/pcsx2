// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

// This module contains code shared by both the dynarec and interpreter versions
// of the VU0 micro.

#include "Common.h"
#include <cmath>
#include "VUmicro.h"
#include "MTVU.h"

#ifdef PCSX2_DEBUG
u32 vudump = 0;
#endif

// This is called by the COP2 as per the CTC instruction
void vu1ResetRegs()
{
	VU0.VI[REG_VPU_STAT].UL &= ~0xff00; // stop vu1
	VU0.VI[REG_FBRST].UL &= ~0xff00; // stop vu1
	vif1Regs.stat.VEW = false;
}

void vu1Finish(bool add_cycles) {
	if (THREAD_VU1) {
		//if (VU0.VI[REG_VPU_STAT].UL & 0x100) DevCon.Error("MTVU: VU0.VI[REG_VPU_STAT].UL & 0x100");
		if (INSTANT_VU1 || add_cycles)
		{
			vu1Thread.WaitVU();
		}
		vu1Thread.Get_MTVUChanges();
		return;
	}
	u32 vu1cycles = VU1.cycle;
	if(VU0.VI[REG_VPU_STAT].UL & 0x100) {
		VUM_LOG("vu1ExecMicro > Stalling until current microprogram finishes");
		CpuVU1->Execute(vu1RunCycles);
	}
	if (VU0.VI[REG_VPU_STAT].UL & 0x100) {
		DevCon.Warning("Force Stopping VU1, ran for too long");
		VU0.VI[REG_VPU_STAT].UL &= ~0x100;
	}
	if (add_cycles)
	{
		cpuRegs.cycle += VU1.cycle - vu1cycles;
	}
}

void vu1ExecMicro(u32 addr)
{
	if (THREAD_VU1) {
		VU0.VI[REG_VPU_STAT].UL &= ~0xFF00;
		// Okay this is a little bit of a hack, but with good reason.
		// Most of the time with MTVU we want to pretend the VU has finished quickly as to gain the benefit from running another thread
		// however with T-Bit games when the T-Bit is enabled, it needs to wait in case a T-Bit happens, so we need to set "Busy"
		// We shouldn't do this all the time as it negates the extra thread and causes games like Ratchet & Clank to be no faster.
		// if (VU0.VI[REG_FBRST].UL & 0x800)
		// {
		//	VU0.VI[REG_VPU_STAT].UL |= 0x0100;
		// }
		// Update 25/06/2022: Disabled this for now, let games YOLO it, if it breaks MTVU, disable MTVU (it doesn't work properly anyway) - Refraction
		vu1Thread.ExecuteVU(addr, vif1Regs.top, vif1Regs.itop, VU0.VI[REG_FBRST].UL);
		return;
	}
	static int count = 0;
	vu1Finish(false);

	VUM_LOG("vu1ExecMicro %x (count=%d)", addr, count++);
	VU1.cycle = cpuRegs.cycle;
	VU0.VI[REG_VPU_STAT].UL &= ~0xFF00;
	VU0.VI[REG_VPU_STAT].UL |=  0x0100;
	if ((s32)addr != -1) VU1.VI[REG_TPC].UL = addr & 0x7FF;

	CpuVU1->SetStartPC(VU1.VI[REG_TPC].UL << 3);
	_vuExecMicroDebug(VU1);
	if(!INSTANT_VU1)
		CpuVU1->ExecuteBlock(1);
	else
		CpuVU1->Execute(vu1RunCycles);
}

void MTVUInterrupt()
{
	VU0.VI[REG_VPU_STAT].UL &= ~0xFF00;
}
