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

#ifndef _HW_DCMD_GPIO_SIUL2_H_INCLUDED
#define _HW_DCMD_GPIO_SIUL2_H_INCLUDED

typedef struct {
        uint8_t        pin_num;
} gpio_devctl_input_t;

typedef struct {
        uint8_t        pin_num;
} gpio_devctl_output_t;

typedef struct {
        uint8_t        pin_num;
        uint8_t        data;
} gpio_devctl_read_t;

typedef struct {
        uint8_t        pin_num;
        uint8_t        data;
} gpio_devctl_write_t;


// A union of all devctl messages
typedef union {
        gpio_devctl_input_t                 set_input;
        gpio_devctl_output_t                set_output;
        gpio_devctl_read_t                  cmd_read;
        gpio_devctl_write_t                 cmd_write;
} gpio_devctl_t;


#define GPIO_SET_INPUT                     0
#define GPIO_SET_OUTPUT                    1
#define GPIO_READ                          2
#define GPIO_WRITE                         3
#define DCMD_GPIO_SET_INPUT                __DIOT(_DCMD_MISC, GPIO_SET_INPUT, gpio_devctl_input_t)
#define DCMD_GPIO_SET_OUTPUT               __DIOT(_DCMD_MISC, GPIO_SET_OUTPUT, gpio_devctl_output_t)
#define DCMD_GPIO_READ                     __DIOTF(_DCMD_MISC, GPIO_READ, gpio_devctl_read_t)
#define DCMD_GPIO_WRITE                    __DIOTF(_DCMD_MISC, GPIO_WRITE, gpio_devctl_write_t)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/support/gpio-siul2/resmgr/public/hw/dcmd_gpio_siul2.h $ $Rev: 859876 $")
#endif
