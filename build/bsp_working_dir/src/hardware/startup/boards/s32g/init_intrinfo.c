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

#include <startup.h>

#define GICD_BASE		0x50800000
#define GICR_BASE		0x50880000
#define GICC_BASE		0x50900000
#define GICV_BASE		0x50920000
#define GICH_BASE		0x50910000


void
init_intrinfo(void)
{
	/*
	 * Initialise GIC
	 */
	gic_v3_init(GICD_BASE, GICR_BASE, GICC_BASE, 0, 0);
	gicv_asinfo(GICV_BASE, GICH_BASE, NULL_PADDR, 0, 0);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/init_intrinfo.c $ $Rev: 858304 $")
#endif
