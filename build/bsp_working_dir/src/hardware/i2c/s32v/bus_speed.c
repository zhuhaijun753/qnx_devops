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

#define ARRAY_LEN	60

static const uint16_t i2c_clk_div[ARRAY_LEN][2] = {
        { 20,    0x00 },    { 22,    0x01 },    { 24,    0x02 },    { 26,    0x03 },
        { 28,    0x04 },    { 30,    0x05 },    { 32,    0x09 },    { 34,    0x06 },
        { 36,    0x0A },    { 40,    0x07 },    { 44,    0x0C },    { 48,    0x0D },
        { 52,    0x43 },    { 56,    0x0E },    { 60,    0x45 },    { 64,    0x12 },
        { 68,    0x0F },    { 72,    0x13 },    { 80,    0x14 },    { 88,    0x15 },
        { 96,    0x19 },    { 104,   0x16 },    { 112,   0x1A },    { 128,   0x17 },
        { 136,   0x4F },    { 144,   0x1C },    { 160,   0x1D },    { 176,   0x55 },
        { 192,   0x1E },    { 208,   0x56 },    { 224,   0x22 },    { 228,   0x24 },
        { 240,   0x1F },    { 256,   0x23 },    { 288,   0x5C },    { 320,   0x25 },
        { 384,   0x26 },    { 448,   0x2A },    { 480,   0x27 },    { 512,   0x2B },
        { 576,   0x2C },    { 640,   0x2D },    { 768,   0x31 },    { 896,   0x32 },
        { 960,   0x2F },    { 1024,  0x33 },    { 1152,  0x34 },    { 1280,  0x35 },
        { 1536,  0x36 },    { 1792,  0x3A },    { 1920,  0x37 },    { 2048,  0x3B },
        { 2304,  0x3C },    { 2560,  0x3D },    { 3072,  0x3E },    { 3584,  0x7A },
        { 3840,  0x3F },    { 4096,  0x7B },    { 5120,  0x7D },    { 6144,  0x7E },
};


/* calculate and set proper clock rate */
static uint8_t find_best_ic( s32v_dev_t  *dev, unsigned int speed)
{
    unsigned int div;
    uint8_t clk_div;

    /* Divider value calculation */
    div = (dev->i2c_clk_rate + speed - 1) / speed;
    if (div < i2c_clk_div[0][0]) {
        clk_div = 0;
    } else if (div > i2c_clk_div[ARRAY_LEN - 1][0]) {
        clk_div = ARRAY_LEN - 1;
    } else {
        for (clk_div = 0; i2c_clk_div[clk_div][0] < div; clk_div++)
            ;
    }

    /* Store divider value */
    return clk_div;
  }

int
s32v_set_bus_speed(void *hdl, unsigned int speed, unsigned int *ospeed)
{
    s32v_dev_t    *dev = hdl;
    unsigned int    i2c_freq_val=0;

    if (speed >I2C_MAX_SPEED ) {
        errno = EINVAL;
        return -1;
    }
    if(speed != dev->speed){
        i2c_freq_val = find_best_ic( dev, speed );
        out8( dev->regbase + I2C_IBFD,  i2c_clk_div[i2c_freq_val][1]);
        dev->speed = speed; /*save the speed, next time we don't have to recalculate */
        dev->i2c_freq_val =  i2c_clk_div[i2c_freq_val][1];
    }

    if (ospeed)
        *ospeed = dev->i2c_clk_rate / i2c_clk_div[i2c_freq_val][0];

    return 0;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/s32v/bus_speed.c $ $Rev: 810496 $")
#endif

