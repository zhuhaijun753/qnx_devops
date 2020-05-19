/*
 * $QNXLicenseC:
 * Copyright 2018 QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */


/*
 * NXP S32G system reboot
 */

#include "callout.ah"
#include <aarch64/s32g.h>

/*
 * -----------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *	x0 - physical address of syspage
 *	x1 - virtual  address of syspage
 *	x2 - offset from start of syspage to start of the callout routine
 *	x3 - offset from start of syspage to read/write data used by callout
 * -----------------------------------------------------------------------
 */

patch_reboot:
	sub	sp, sp, #16
	stp	x19, x30, [sp]

	add     x19, x0, x2     // x19 = address of callout routine

	/*
	 * Map UART using patch_data parameter
	 */
	mov	x0, #0x1000
	adr     x1, Lpaddr
	bl      callout_io_map_indirect

	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH   x19, w4, w5

	ldp	x19, x30, [sp]
	add	sp, sp, #16
	ret

	.align	3

Lpaddr:	.quad	 S32G_MC_ME_BASE

CALLOUT_START(reboot_s32g, 0, patch_reboot)
	/*
	 * Get the Mode Entry Module base address (patched)
	 */
	mov	x7, #0xabcd
	movk	x7, #0xabcd, lsl #16
	movk	x7, #0xabcd, lsl #32
	movk	x7, #0xabcd, lsl #48

_do_reboot:
	mov	w0, #0x8002	//Set Functional reset request bit
	movk	w0, #0x0000, lsl #16
	str	w0, [x7, #S32G_MC_ME_MODE]

	mov	w0, #0x1	//Set Mod Update bit
	movk	w0, #0x0000, lsl #16
	str	w0, [x7, #S32G_MC_ME_UPD]

	mov	w0, #0x5af0	//Write the valid control key (0x5AF0)
	movk	w0, #0x0000, lsl #16
	str	w0, [x7, #S32G_MC_ME_CTL]

	mov	w0, #0xa50f	//Write the vaid invert control key (0xA50F)
	movk	w0, #0x0000, lsl #16
	str	w0, [x7, #S32G_MC_ME_CTL]

0:
	b		0b
CALLOUT_END(reboot_s32g)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/callout_reboot_s32g.S $ $Rev: 858304 $";
.previous
#endif
#endif
