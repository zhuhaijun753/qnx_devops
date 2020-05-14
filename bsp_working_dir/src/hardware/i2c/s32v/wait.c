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

static void s32v_i2c_reset(s32v_dev_t *dev)
{
    out8(dev->regbase + I2C_IBCR, IBCR_MDIS);
    out8(dev->regbase + I2C_IBSR, IBSR_IBAL | IBSR_IBIF);

    out8(dev->regbase + I2C_IBCR, 0);
    out8(dev->regbase + I2C_IBFD, dev->i2c_freq_val );
    dev->restart = 0;
}

int s32v_wait_bus_not_busy(s32v_dev_t *dev, unsigned int stop)
{
    unsigned        tries = 1000000;
    if(dev->restart) {
        return 0;
    }else {
        while (in8(dev->regbase + I2C_IBSR) & IBSR_IBB){
            if (tries-- == 0){
                s32v_i2c_slogf(dev, _SLOG_ERROR, "wait bus idle failed (%x %x)",in8(dev->regbase + I2C_IBCR), in8(dev->regbase + I2C_IBSR));
                /* reset the controller to see if it's able to recover*/
                s32v_i2c_reset(dev);
                /*try again to see if it's OK now after reset */
                if(in8(dev->regbase + I2C_IBSR) & IBSR_IBB){
                    return -1;
                }else{
                    break;
                }
            }
        }
        return 0;
    }
    return -1;
}

uint8_t
s32v_wait_status(s32v_dev_t *dev)
{
    uint8_t    status;
    uint64_t   ntime = 500000000ULL;
    int        interr = EOK;

    while ( interr != ETIMEDOUT ){
        TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_INTR, NULL, &ntime, NULL);
        interr = InterruptWait_r(0, NULL);
        status = in8(dev->regbase + I2C_IBSR);
        if (status & IBSR_IBIF){
            out8(dev->regbase + I2C_IBSR, in8(dev->regbase + I2C_IBSR) | (IBSR_IBIF));
            InterruptUnmask(dev->intr, dev->iid);
            return status;
        }
    }
    s32v_i2c_slogf(dev, _SLOG_ERROR,"s32v_wait_status timedout (%x %x)",in8(dev->regbase + I2C_IBCR), in8(dev->regbase + I2C_IBSR));
    /* timeout case, we need reset */
    s32v_i2c_reset(dev);

    return 0;
}

i2c_status_t
s32v_recvbyte(s32v_dev_t *dev, uint8_t *byte, int nack, int stop){
    uint8_t    status;

    status = s32v_wait_status(dev);

    if (!(status & IBSR_TCF)){
        if(status)
            s32v_i2c_reset(dev);
        return I2C_STATUS_ERROR;
    }

    if (nack){
        out8(dev->regbase + I2C_IBCR, IBCR_IBIE | IBCR_MSSL | IBCR_NOACK);
    }else if(stop){
        out8(dev->regbase + I2C_IBCR, IBCR_NOACK);
    }

    *byte = in8(dev->regbase + I2C_IBDR);

    return 0;
}

i2c_status_t
s32v_sendbyte(s32v_dev_t *dev, uint8_t byte){
    uint8_t status, ctrl ;

    /* write data in Data Register */

    out8(dev->regbase + I2C_IBCR, in8(dev->regbase + I2C_IBCR) | IBCR_TXRX);
    out8(dev->regbase + I2C_IBDR, byte);

    status = s32v_wait_status(dev);
    if (!(status & IBSR_TCF)){
        if(status)
            s32v_i2c_reset(dev);
        return I2C_STATUS_ERROR;
    }

    if (!(in8(dev->regbase + I2C_IBCR) & IBCR_MSSL)){
		if (status & IBSR_IBAL) {
			out8(dev->regbase + I2C_IBSR, in16(dev->regbase + I2C_IBSR) | IBSR_IBAL);
			s32v_i2c_reset(dev);
			return I2C_STATUS_ARBL;
		}
		if (status & IBSR_IAAS){
			s32v_i2c_reset(dev);
			return I2C_STATUS_ERROR;
		}
	}

    if (status & IBSR_RXAK){

        /* Send Stop to stop I2C transaction */
        ctrl = in8(dev->regbase + I2C_IBCR);
        ctrl &= ~(IBCR_MSSL | IBCR_TXRX);
        out8(dev->regbase + I2C_IBCR, ctrl);

        return I2C_STATUS_NACK;
    }

    return 0;
}

static inline i2c_status_t s32v_wait_busy(s32v_dev_t *dev)
{
    /* wait for 500us */
    int timeout = 5000;
    while(!(in8(dev->regbase + I2C_IBSR) & IBSR_IBB) && timeout--){
        nanospin_ns(100);
    }
    if(timeout<=0){
        s32v_i2c_slogf(dev, _SLOG_ERROR, "s32v_wait_busy timedout (%x %x)",in8(dev->regbase + I2C_IBCR), in8(dev->regbase + I2C_IBSR));
        s32v_i2c_reset(dev);
        return (I2C_STATUS_ERROR);
    }

    return (0);
}

i2c_status_t
s32v_sendaddr7(s32v_dev_t *dev, unsigned addr, int read, int restart){
    out8(dev->regbase + I2C_IBCR, IBCR_IBIE | IBCR_MSSL |
                                                (restart? IBCR_RSTA: 0) | IBCR_TXRX);
    if(s32v_wait_busy(dev)){
        return I2C_STATUS_ERROR;
    }

    return s32v_sendbyte(dev, (addr << 1) | read);
}

i2c_status_t
s32v_sendaddr10(s32v_dev_t *dev, unsigned
 addr, int read, int restart){
    i2c_status_t    err;

    out8(dev->regbase + I2C_IBCR,  IBCR_IBIE | IBCR_MSSL |
                                                (restart? IBCR_RSTA: 0) | IBCR_TXRX);
    if(s32v_wait_busy(dev)){
        return I2C_STATUS_ERROR;
    }
    err = s32v_sendbyte(dev, I2C_XADDR1(addr));
    if (err!=0)
        return err;
    err = s32v_sendbyte(dev, I2C_XADDR2(addr));
    if (err!=0)
        return err;

    if (read){
        out8(dev->regbase + I2C_IBCR, IBCR_IBIE | IBCR_MSSL |
                                                    IBCR_RSTA | IBCR_TXRX);
        err = s32v_sendbyte(dev, I2C_XADDR1(addr) | read);
        if (err!=0)
            return err;
    }
    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/s32v/wait.c $ $Rev: 810496 $")
#endif

