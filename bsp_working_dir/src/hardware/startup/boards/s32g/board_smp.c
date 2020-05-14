/*
 * $QNXLicenseC:
 * Copyright 2018, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include "startup.h"
#include <libfdt.h>
#include <aarch64/s32g.h>
#include "board.h"
#include "s32g_startup.h"

unsigned
board_smp_num_cpu()
{
	unsigned numcores;

	if(fdt == NULL) {
		numcores = S32G_A53_NUM;
	} else {
		numcores = fdt_num_cpu();
	}

	if (debug_flag) {
		kprintf("board_smp_num_cpu: %d cores\n", numcores);
	}
	return numcores;
}

void
board_smp_init(struct smp_entry *smp, unsigned num_cpus)
{
	smp->send_ipi = (void *)&sendipi_gic_v3_sr;
}

int
board_smp_start(unsigned cpu, void (*start)(void))
{
	if(fdt == NULL) {
#define CACHE_FLUSH(addr) asm volatile("dc cvac,%0" :: "r"(addr))
#define SIGNAL() asm volatile("sev")

		spin_shim_start = start;
		CACHE_FLUSH(&spin_shim_start);

		uintptr_t spin_addr = CPU_RELEASE_ADDR;
		*(uintptr_t volatile *)spin_addr = (uintptr_t)spin_shim_rtn;
		CACHE_FLUSH(spin_addr);
		__cpu_membarrier();
		SIGNAL();

		switch (cpu)
		{
			case 0: spin_shim_id = 0x000; break;
			case 1: spin_shim_id = 0x001; break;
			case 2: spin_shim_id = 0x100; break;
			case 3: spin_shim_id = 0x101; break;
			default: return 0;
		}

		CACHE_FLUSH(&spin_shim_id);
		__cpu_membarrier();
		SIGNAL();
		return 1;
	} else {
		return fdt_smp_spin_start(cpu, start);
	}
}

unsigned
board_smp_adjust_num(unsigned cpu)
{
	return cpu;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/board_smp.c $ $Rev: 900617 $")
#endif
