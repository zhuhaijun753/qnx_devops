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
s32v_options(s32v_dev_t *dev, int argc, char *argv[])
{
    int                 c;
    int                 prev_optind;
    int                 done = 0;
    unsigned long       interface = 1;
    dev->restart = 0;
    dev->i2c_clk_rate = I2C_INPUT_CLOCK;

    while (!done) {
        prev_optind = optind;
        c = getopt(argc, argv, "I:s:vc:p:i:");
        switch (c) {
        case 'I':
           interface = strtoul(optarg, &optarg, 0);
            query_hwi_device(dev, interface);
           break;

        case 'v':
            dev->verbose++;
            break;

        case 'c':
            dev->i2c_clk_rate = strtoul(optarg, &optarg, 0);
            break;

        case 'p':
            dev->physbase = strtoul(optarg, &optarg, 0);
            fprintf(stderr, "i2c driver: physbase = 0x%x\n", dev->physbase);
           break;

        case 'i':
            dev->intr = strtol(optarg, &optarg, 0);
            break;

        case '?':
            if (optopt == '-') {
                ++optind;
                break;
            }
            return -1;

        case -1:
            if (prev_optind < optind)
                return -1;

            if (argv[optind] == NULL) {
                done = 1;
                break;
            }
            if (*argv[optind] != '-') {
                ++optind;
                break;
            }
            return -1;

        case ':':
        default:
            return -1;
        }
    }

    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/s32v/options.c $ $Rev: 810496 $")
#endif

