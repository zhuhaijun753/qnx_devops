/*
 * $QNXLicenseC:
 * Copyright 2018, QNX Software Systems.
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



#ifndef _PROTO_H_INCLUDED
#define _PROTO_H_INCLUDED

#define PRINT_FUNCTIONNAME

struct gpio_dev;
struct gpio_ocb;

#define IOFUNC_ATTR_T   struct gpio_dev
#define IOFUNC_OCB_T    struct gpio_ocb
#define THREAD_POOL_PARAM_T dispatch_context_t

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <hw/inout.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <devctl.h>
#include <atomic.h>
#include <sys/neutrino.h>
#include <sys/procmgr.h>
#include <sys/mman.h>
#include <time.h>


//HW specific includes
#include "siul2.h"


#define GPIO_RESMGR_NPARTS_MIN   2
#define GPIO_RESMGR_MSGSIZE_MIN  16
#define GPIO_CLIENTS_MAX         32

typedef struct gpio_dev {
    iofunc_attr_t       hdr;
    dispatch_t          *dpp;
    dispatch_context_t  *ctp;
    int                 id;
    iofunc_notify_t     notify[3];
    void                *hdl;
} gpio_dev_t;

typedef struct gpio_ocb {
    iofunc_ocb_t        hdr;
} gpio_ocb_t;

#define GPIO_SET_INPUT                     0
#define GPIO_SET_OUTPUT                    1
#define GPIO_READ                          2
#define GPIO_WRITE                         3

#define DCMD_SET_INPUT                      __DIOT(_DCMD_MISC, GPIO_SET_INPUT, gpio_devctl_input_t)
#define DCMD_SET_OUTPUT                     __DIOT(_DCMD_MISC, GPIO_SET_OUTPUT, gpio_devctl_output_t)
#define DCMD_READ                           __DIOTF(_DCMD_MISC, GPIO_READ, gpio_devctl_read_t)
#define DCMD_WRITE                          __DIOTF(_DCMD_MISC, GPIO_WRITE, gpio_devctl_write_t)

uintptr_t siul2_0_base;
uintptr_t siul2_1_base;

typedef struct siul2_reg {
    uintptr_t       base;
    uint32_t        mscr;
    uint32_t        gpdo;
    uint32_t        gpdi;
    uint32_t        gpio_num;
} siul2_reg_t;

gpio_ocb_t *gpio_ocb_calloc(resmgr_context_t *ctp, gpio_dev_t *dev);
void gpio_ocb_free(gpio_ocb_t *ocb);
int gpio_devctl(resmgr_context_t *ctp, io_devctl_t *msg, gpio_ocb_t *ocb);
int gpio_slogf(const char *fmt, ...);

int hw_cmd_write(void *hdl, void *buf);
int hw_cmd_read(void *hdl, void *buf);
int hw_set_output(void *hdl, uint8_t data);
int hw_set_input(void *hdl, uint8_t data);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/support/gpio-siul2/resmgr/proto.h $ $Rev: 859876 $")
#endif

