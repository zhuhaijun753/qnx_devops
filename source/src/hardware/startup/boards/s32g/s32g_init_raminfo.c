/*
 * $QNXLicenseC:
 * Copyright 2018,2019, QNX Software Systems.
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



/*
 * Read FDT then update syspage RAM section
 */
#include "startup.h"
#include <libfdt.h>
#include "board.h"

#define	S32G_DRAM0_BASE		0x80000000
#define	S32G_DRAM1_BASE		0xC0000000
#define	S32G_DRAM_SIZE		0x40000000

void s32g_init_raminfo()
{

	if(fdt == NULL) {
		if (debug_flag) {
			kprintf("Unable to find FDT, use MDASP to determine the memory configuration,\n");
		}
		avoid_ram(S32G_MEMRSV_BASE, S32G_MEMRSV_SIZE);
		add_ram(S32G_DRAM0_BASE,S32G_DRAM_SIZE);
		add_ram(S32G_DRAM1_BASE,S32G_DRAM_SIZE	);
		alloc_ram(S32G_MEMRSV_BASE, S32G_MEMRSV_SIZE, 1);
	} else {
		int nrsv,onrsv;

		// put fdt reserved memory to the avoid list first
		// otherwise, it could potentially overlap with those allocated during add_rams.

		onrsv = nrsv = fdt_num_mem_rsv(fdt);
		uint64_t start, size;
		while(onrsv-- > 0) {
			if(fdt_get_mem_rsv(fdt, onrsv, &start, &size) >= 0) {
				avoid_ram(start, size);
			}
		}

		add_ram(S32G_DRAM0_BASE,S32G_DRAM_SIZE);
		add_ram(S32G_DRAM1_BASE,S32G_DRAM_SIZE	);

		// alloc reserved memory
		while(nrsv-- > 0) {
			if(fdt_get_mem_rsv(fdt, nrsv, &start, &size) >= 0) {
				alloc_ram(start, size, 1);
			}
		}
	}
}
#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/s32g_init_raminfo.c $ $Rev: 900617 $")
#endif

