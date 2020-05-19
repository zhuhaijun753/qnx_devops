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


#ifndef __PROTO_H_INCLUDED
#define __PROTO_H_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <hw/i2c.h>
#include <sys/hwinfo.h>
#include <drvr/hwinfo.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <aarch64/s32v.h>


typedef struct _s32v_dev {

    uintptr_t           regbase;
    unsigned            physbase;

    int                 intr;
    int                 iid;
    struct sigevent     intrevent;

    unsigned            slave_addr;
    i2c_addrfmt_t       slave_addr_fmt;
    unsigned            restart;
    unsigned            speed;
    unsigned            i2c_freq_val;
    unsigned            i2c_clk_rate;
    uint32_t            verbose;

} s32v_dev_t;

#define I2C_XADDR1(addr)    (0xf0 | (((addr) >> 7) & 0x6))
#define I2C_XADDR2(addr)    ((addr) & 0xff)
#define I2C_ADDR_RD            1
#define I2C_ADDR_WR            0

#define OPT_VERBOSE           0x00000002
#define I2C_INPUT_CLOCK      133000000

#define I2C_REGLEN      0x6

#define I2C_IBAD        0x0
#define I2C_IBFD        0x1
#define I2C_IBCR        0x2
    #define IBCR_MDIS            (1 << 7)
    #define IBCR_IBIE            (1 << 6)
    #define IBCR_MSSL            (1 << 5)
    #define IBCR_TXRX            (1 << 4)
    #define IBCR_NOACK           (1 << 3)
    #define IBCR_RSTA            (1 << 2)
    #define IBCR_DMAEN           (1 << 1)

#define I2C_IBSR        0x3
    #define IBSR_TCF             (1 << 7)
    #define IBSR_IAAS            (1 << 6)
    #define IBSR_IBB             (1 << 5)
    #define IBSR_IBAL            (1 << 4)
    #define IBSR_SRW             (1 << 2)
    #define IBSR_IBIF            (1 << 1)
    #define IBSR_RXAK            (1 << 0)

#define I2C_IBDR        0x4
#define I2C_IBIC        0x5
    #define IBIC_BIIE            (1 << 7)
    #define IBIC_BYTERXIE        (1 << 6)

#define I2C_IBDBG       0x6

#define I2C_MAX_SPEED    400000


//#define BUS_CLK        300000000        // 300MHz
//#define I2C_CLK        150000000       //BUS_CLK /2

#define I2C_REVMAJOR(rev)      (((rev) >> 8) & 0xff)
#define I2C_REVMINOR(rev)      ((rev) & 0xff)


#define s32v_i2c_slogf(dev, level, fmt, ...)                                \
    do {                                                                    \
    if ((level <= _SLOG_ERROR) || (dev->verbose & level)) {      \
        slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), level, fmt, ##__VA_ARGS__);    \
    }                                                                       \
    } while (0)


void *s32v_init(int argc, char *argv[]);
void s32v_fini(void *hdl);
int s32v_options(s32v_dev_t *dev, int argc, char *argv[]);

int s32v_wait_bus_not_busy(s32v_dev_t *dev,unsigned int stop);
int s32v_set_slave_addr(void *hdl, unsigned int addr, i2c_addrfmt_t fmt);
int s32v_set_bus_speed(void *hdl, unsigned int speed, unsigned int *ospeed);
int s32v_version_info(i2c_libversion_t *version);
int s32v_driver_info(void *hdl, i2c_driver_info_t *info);
int s32v_devctl(void *hdl, int cmd, void *msg, int msglen,
        int *nbytes, int *info);
i2c_status_t s32v_recv(void *hdl, void *buf,
        unsigned int len, unsigned int stop);
i2c_status_t s32v_send(void *hdl, void *buf,
        unsigned int len, unsigned int stop);
uint8_t s32v_wait_status(s32v_dev_t *dev);
i2c_status_t s32v_sendaddr7(s32v_dev_t *dev,
                              unsigned addr, int read, int restart);
i2c_status_t s32v_sendaddr10(s32v_dev_t *dev,
                                unsigned addr, int read, int restart);
i2c_status_t s32v_sendbyte(s32v_dev_t *dev, uint8_t byte);
i2c_status_t s32v_recvbyte(s32v_dev_t *dev, uint8_t *byte,
                              int nack, int stop);
extern int query_hwi_device(s32v_dev_t *dev, unsigned unit);


#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/s32v/proto.h $ $Rev: 810496 $")
#endif

