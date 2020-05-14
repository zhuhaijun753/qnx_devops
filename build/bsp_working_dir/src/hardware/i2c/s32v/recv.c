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

i2c_status_t
s32v_recv(void *hdl, void *buf, unsigned int len, unsigned int stop){
    s32v_dev_t  *dev = hdl;
    i2c_status_t    status;

    if (len <= 0)
        return I2C_STATUS_DONE;

    if(s32v_wait_bus_not_busy(dev, stop))
        return I2C_STATUS_ERROR;

    /* send slave address */

    if (dev->slave_addr_fmt == I2C_ADDRFMT_7BIT)
        status = s32v_sendaddr7(dev, dev->slave_addr, I2C_ADDR_RD, dev->restart);
    else
        status = s32v_sendaddr10(dev, dev->slave_addr, I2C_ADDR_RD, dev->restart);

    if (status)
        return status;

    if (len > 1)
        out8(dev->regbase + I2C_IBCR,  IBCR_IBIE | IBCR_MSSL);
    else
        out8(dev->regbase + I2C_IBCR,  IBCR_IBIE | IBCR_MSSL | IBCR_NOACK);

    /* Dummy read */
    in8(dev->regbase + I2C_IBDR);

    while (len > 0){
        status = s32v_recvbyte(dev, buf, (len == 2), (len == 1) && stop);
        if (status)
            return status;
        ++buf; --len;
    }

    if (!stop)
        s32v_wait_status(dev);

    dev->restart = !stop;

    return I2C_STATUS_DONE;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/s32v/recv.c $ $Rev: 810496 $")
#endif
