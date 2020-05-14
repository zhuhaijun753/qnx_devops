/*
 * $QNXLicenseC:
 * Copyright 2016, QNX Software Systems.
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

/* Module Description:	board specific header file */

#ifndef _BS_H_INCLUDED
#define _BS_H_INCLUDED

#include <internal.h>

#include <sys/utsname.h>

/* add new chipset externs here */
#define SDIO_HC_IMX6

#define SDIO_SOC_SUPPORT
#define ADMA_SUPPORTED		1

#define CD_PIN			16
#define WP_PIN			19

typedef struct _s32v_ext {
    int          emmc;             /* If non-0, implies "nocd" option as well */
    int          nocd;             /* If non-0, indicates CD is not supported */
    int          cd_gpio;          /* CD GPIO# */
    int          bw;               /* Data bus width */
    int          vdd1_8;           /* 1.8v support */
    int          cd_pin;           /* uSDHCx_PRES_STATE register: CINST bit */
    int          wp_pin;           /* uSDHCx_PRES_STATE register: WPSPL bit */
} s32v_ext_t;

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/aarch64/s32v.le/bs.h $ $Rev: 886103 $")
#endif
