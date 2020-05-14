/*
 * $QNXLicenseC:
 * Copyright 2019-2020, QNX Software Systems.
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

/* This file can contain board specific preprocessor macros and function declarations */

#ifndef __BOARD_H
#define __BOARD_H

#define NO_FDT

#define S32G_MEMRSV_BASE            0x8000FFF8
#define S32G_MEMRSV_SIZE            0x00010000

extern void hwi_s32gevb();
extern void init_gmac();
extern void init_can();

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/evb/board.h $ $Rev: 905644 $")
#endif
