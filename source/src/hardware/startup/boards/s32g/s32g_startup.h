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

#ifndef __S32G_STARTUP_H_INCLUDED
#define __S32G_STARTUP_H_INCLUDED

#include <stdint.h>
#include <sys/types.h>

#define S32G_A53_NUM		4
#define CPU_RELEASE_ADDR	0x80000000
#define CPU_RELEASE_DATA_SIZE	8

/* Startup command line arguments */
#define S32G_WDOG_ENABLE	(1 << 0)

void s32g_wdg_enable(void);
void s32g_wdg_reload(int32_t time_out_val);

void s32g_init_raminfo(void);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/s32g_startup.h $ $Rev: 900722 $")
#endif

