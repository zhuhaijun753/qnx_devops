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


#include "proto.h"

int
query_hwi_device(s32v_dev_t *dev, unsigned unit)
{
    unsigned     hwi_off = hwi_find_bus(HWI_ITEM_BUS_I2C, unit);
    if(hwi_off != HWI_NULL_OFF) {
        hwi_tag *tag = hwi_tag_find(hwi_off, HWI_TAG_NAME_location, 0);
        if(tag){
            dev->physbase = tag->location.base;
            dev->intr = hwitag_find_ivec(hwi_off, NULL);
       }
        return 1;
    }

    /* No default device, the base address and irq have been specified */
    /* using defaults */
    s32v_i2c_slogf(dev, _SLOG_ERROR,"Hwinfo I2C section is empty");
	return 0;
 }

void *
s32v_init(int argc, char *argv[])
{
    s32v_dev_t      *dev = NULL;

    if (-1 == ThreadCtl(_NTO_TCTL_IO_PRIV, 0)) {
        s32v_i2c_slogf(dev, _SLOG_ERROR,"%s : ThreadCtl -%s", __FUNCTION__, strerror(errno));
        return NULL;
    }

    dev = malloc(sizeof(s32v_dev_t));
    if (!dev){
        s32v_i2c_slogf(dev, _SLOG_ERROR,"Device malloc error.");
        return NULL;
    }

    /* Default getting the I2C1 Base addresss and irq from the Hwinfo Section if available */
    query_hwi_device(dev, 0);

    if (-1 == s32v_options(dev, argc, argv))
        goto fail;

    if(dev->physbase == 0 || dev->intr ==0){
        s32v_i2c_slogf(dev, _SLOG_ERROR,"%s : Invalid  I2C controller physics based address or IRQ value.", __FUNCTION__);
        s32v_i2c_slogf(dev, _SLOG_ERROR,"%s : Please check the command line or Hwinfo default setting.",__FUNCTION__);
        goto fail;
     }

    dev->regbase = mmap_device_io(I2C_REGLEN, dev->physbase);
    if (dev->regbase == (uintptr_t)MAP_FAILED) {
        s32v_i2c_slogf(dev, _SLOG_ERROR,"%s : mmap_device_io", __FUNCTION__);
        goto fail;
    }

    /* Setup I2C module */

    out8(dev->regbase + I2C_IBCR, IBCR_MDIS);  //disable I2C bus

    out8(dev->regbase + I2C_IBSR, IBSR_IBAL | IBSR_IBIF);  //Clean I2C status register
    out8(dev->regbase + I2C_IBCR, 0);  // enable I2C bus

    /* Initialize interrupt handler */
    SIGEV_INTR_INIT(&dev->intrevent);
    dev->iid = InterruptAttachEvent(dev->intr, &dev->intrevent,
            _NTO_INTR_FLAGS_TRK_MSK);
    if (dev->iid == -1) {
        s32v_i2c_slogf(dev, _SLOG_ERROR,"%s: InterruptAttachEvent failure", __FUNCTION__ );
        goto fail1;
    }

    /* Set clock prescaler using default baud*/
    s32v_set_bus_speed(dev, 100000, NULL);

    /* enable interrupts */
    out8(dev->regbase + I2C_IBCR, in8(dev->regbase + I2C_IBCR) | IBCR_IBIE );

   return dev;

fail1:
    munmap_device_io(dev->regbase, I2C_REGLEN);

fail:
    free(dev);
    return NULL;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/s32v/init.c $ $Rev: 886103 $")
#endif
