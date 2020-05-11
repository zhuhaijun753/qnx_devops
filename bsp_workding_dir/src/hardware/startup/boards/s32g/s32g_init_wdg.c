/*
 * $QNXLicenseC:
 * Copyright 2018  QNX Software Systems.
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
#include "board.h"
#include <aarch64/s32g.h>
#include <stdlib.h>
#include <hw/inout.h>

#define SWT_CLOCK_SPEED         48000000

#define WDT_TO_MAX_VALUE        0xFFFFFFFF
#define WDT_DEFAULT_VALUE       0x0FFFFFFD
#define WDT_TO_MIN_COUNT        0x100

#define SWT_CR_CONTROL_BITS     0x7FF

/* Enable watch-dog timer */

void s32g_wdg_enable(void)
{
    if (in32(S32G_SWT4_BASE + SWT_CR) & SWT_CR_HLK) {
        /* Hard-locked, can't do anything */
        return;
    }

    /* Enable the watchdog*/
    out32(S32G_SWT4_BASE + SWT_CR, in32(S32G_SWT4_BASE + SWT_CR) | SWT_CR_WEN);
}

/* Setup watch-dog timer */

void s32g_wdg_reload(int32_t time_out_val)
{
    uint32_t val;

    /* set timeout value */
    if (time_out_val < 0) {
        val = WDT_DEFAULT_VALUE;
    } else {
        uint64_t tmp = (uint64_t) time_out_val;
        tmp *= (SWT_CLOCK_SPEED / 1000);
        tmp = max(tmp, WDT_TO_MIN_COUNT);
        tmp = min(tmp, WDT_TO_MAX_VALUE);
        val = (uint32_t) tmp;
    }
    out32(S32G_SWT4_BASE + SWT_TO, val);

    val = in32(S32G_SWT4_BASE + SWT_CR);

    if (val & SWT_CR_HLK) {
        kprintf("%s: Error - SWT4 has been hard-locked, cannot configure.", __FUNCTION__);
        return;
    } else if (val & SWT_CR_SLK) {
        /* Soft lock is on, unlock sequence is to write two magic numbers into SR in sequence */
        out32(S32G_SWT4_BASE + SWT_SR, 0xC520);
        out32(S32G_SWT4_BASE + SWT_SR, 0xD928);
    }

    /* Mask off these control bits so no unintended settings make it through. This will also clear WEN */
    val &= ~SWT_CR_CONTROL_BITS;

    /* Allows watchdog timer to be stopped when device enters debug mode */
    val |= SWT_CR_FRZ;

    /* Use Fixed Service Sequence to ping the watchdog */
    val |= SWT_CR_FIXED_SS;

    out32(S32G_SWT4_BASE + SWT_CR, val);

}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/s32g_init_wdg.c $ $Rev: 890119 $")
#endif

