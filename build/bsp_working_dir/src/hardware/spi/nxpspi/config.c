/*
 * $QNXLicenseC:
 * Copyright 2017, QNX Software Systems.
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


#include "nxpspi.h"
#include <stdbool.h>

uint32_t nxp_cfg(void *hdl, spi_cfg_t *cfg)
{
    nxp_spi_t   *nxp = hdl;
    uint32_t    charlen, post_div, pre_div, drate, post_drate, pbr, ctar = 0;
    uint32_t    best_drate = 0, best_post_div = 0, best_pre_div = 0;

    if (cfg == NULL) {
        return 0;
    }

    charlen = cfg->mode & SPI_MODE_CHAR_LEN_MASK;

    /* The controller hw supports 4-16 bits, but this driver only has support
     * for 4-8 bit data frame lengths.  For frame lengths > 8-bit (and a multiple of 8-bits)
     * we can emulate as an 8-bit transfer. Current only support 4~8bit and 32bit data frame size.
     * TODO: Need to add other frame size
     */
    if(charlen < 4) {
        return 0;
    } else if (charlen > 8) {
        if (charlen == 32 && (cfg->mode & SPI_MODE_IDLE_INSERT) == 0 ) {
            nxp->emulate_8b = charlen / 8;
            charlen = 8;
        } else {
            return 0;
        }
    } else {
        nxp->emulate_8b = 0;
    }

    /* Assign the datarate if calculated rate <= desired rate
     * Find the highest possible baud rate that is less than or equal to the desired rate
     */
    for (post_div = 0; post_div < 16; post_div++) {
        if (post_div < 2) {
            post_drate = nxp->clock >> (post_div + 1);
        }
        else if (post_div == 2) {
            post_drate = nxp->clock / 6;
        }
        else {
            post_drate = nxp->clock >> (post_div);
        }

        for (pre_div = 0; pre_div < 4; pre_div++) {
            pbr = (2 * pre_div) + 1;
            if (pre_div == 0) {
                pbr++;
            }
            drate = post_drate / pbr;

            if (drate <= cfg->clock_rate && drate > best_drate) {
              // this is a better match, take this one for now
              best_drate = drate;
              best_post_div = post_div;
              best_pre_div = pre_div;
            }
        }
    }

    cfg->clock_rate = best_drate;

    if (best_drate == 0)
    {
        return 0;
    }
    ctar = (best_post_div << NXP_SPI_CTAR0_BR_POS) | (best_pre_div << NXP_SPI_CTAR0_PBR_POS) | NXP_SPI_CTAR0_FMSZ(charlen);

    return ctar;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/spi/nxpspi/config.c $ $Rev: 847655 $")
#endif
