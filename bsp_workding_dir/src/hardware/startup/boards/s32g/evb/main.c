/*
 * $QNXLicenseC:
 * Copyright 2019, QNX Software Systems.
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
 * NXP S32G EVB
 */

#include "startup.h"
#include <time.h>
#include "board.h"
#include "s32g_startup.h"

extern void s32g_init_raminfo();
extern uint32_t get_s32g_chip_rev();

extern struct callout_rtn reboot_s32g;

const struct callout_slot callouts[] = {
	{ CALLOUT_SLOT( reboot, _s32g) },
};


/* Debug port is on USB0 */
const struct debug_device debug_devices[] = {
	{	"linflexd",
		{"0x401c8000^0.115200.80000000.16",
		},
		init_linflexd,
		put_linflexd,
		{	&display_char_linflexd,
			&poll_key_linflexd,
			&break_detect_linflexd,
		}
	},
};

/*
 * main()
 *	Startup program executing out of RAM
 *
 * 1. It gathers information about the system and places it in a structure
 *    called the system page. The kernel references this structure to
 *    determine everything it needs to know about the system. This structure
 *    is also available to user programs (read only if protection is on)
 *    via _syspage->.
 *
 * 2. It (optionally) turns on the MMU and starts the next program
 *    in the image file system.
 */
int
main(int argc, char **argv, char **envv)
{
	int opt, options = 0;

	/*
	 * Check for and initialize flattened device tree
	 * AARCH64 versions of u-boot stash the FDT paddr in x0 prior to jumping (which gets copied to boot_regs[0])
	 * and AARCH32 versions use r2.
	 */

#if (defined (__aarch64__))
	uint64_t fdt_base_paddr = boot_regs[0];
#else
	uint32_t fdt_base_paddr = boot_regs[2];
#endif

#ifdef NO_FDT
	fdt_base_paddr = NULL;
#endif

	if(fdt_base_paddr) {
		fdt_init(fdt_base_paddr);
	}

	/*
	 * Initialise debugging output
	 */
	select_debug(debug_devices, sizeof(debug_devices));

	add_callout_array(callouts, sizeof(callouts));

	// common options that should be avoided are:
	// "AD:F:f:I:i:K:M:N:o:P:R:S:Tvr:j:Z"
	while ((opt = getopt(argc, argv, COMMON_OPTIONS_STRING"W")) != -1) {
		switch (opt) {
			case 'W':
				options |= S32G_WDOG_ENABLE;
				break;
			default:
				handle_common_option(opt);
				break;
		}
	}

	/* Set the default cpu frequency of 1000MHz unless override by -f option */
	if (cpu_freq == 0) {
		// 1 GHz
		cpu_freq = 1000 * 1000 * 1000;
	}

	if (options & S32G_WDOG_ENABLE) {
		/* Enable WDT */
		s32g_wdg_reload(-1);
		s32g_wdg_enable();
	}

	/* Collect information on all free RAM in the system */
	s32g_init_raminfo();

	/* Remove RAM used by modules in the image */
	alloc_ram(shdr->ram_paddr, shdr->ram_size, 1);

	/* Initialize SMP */
	init_smp();

	/* Initialize MMU */
	if (shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL){
		init_mmu();
		board_mmu_enable();
	}

	/* Initialize the Interrupts related Information */
	init_intrinfo();


	/* Initialize the Timer related information */
	init_qtime();

	/* Init L2 Cache Controller */
	init_cacheattr();


	/* Initialize the CPU related information */
	init_cpuinfo();

	/* Initialize the Hwinfo section of the Syspage */
	init_hwinfo();

	/* Initialize GMAC */
	init_gmac();

	/* Initialize CAN */
	init_can();

	/* Add iMX8 TLB errata workaround */
	/* To fix 2 CPU clusters Cache Coherence issue */
	struct cpuinfo_entry    *cpu;

	cpu = &lsp.cpuinfo.p[0];
	cpu->flags |= AARCH64_CPU_FLAG_IMX8QM_TLB_ERRATA;
	/* iMX8 errata workaround END */

	add_typed_string(_CS_MACHINE, "NXP S32G275 PROCEVB-S");

	/*
	 * Update the syspage with entries from fdt.
	 * Note that fdt_asinfo must be caled before init_system_private
	 */
	if(fdt_base_paddr) {
		fdt_asinfo();
	}

	/*
	 * Load bootstrap executables in the image file system and Initialise
	 * various syspage pointers. This must be the _last_ initialisation done
	 * before transferring control to the next program.
	 */
	init_system_private();

	/*
	 * This is handy for debugging a new version of the startup program.
	 * Commenting this line out will save a great deal of code.
	 */
	print_syspage();

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/evb/main.c $ $Rev: 905644 $")
#endif

