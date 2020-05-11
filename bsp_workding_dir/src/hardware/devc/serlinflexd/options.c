/*
 * $QNXLicenseC:
 * Copyright 2016 QNX Software Systems.
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







/*
#ifdef __USAGE
%C - Serial driver for the SOC's with LinflexD IP such as the NXP S32V

%C [options] [port[,irq]] &
Options:
 -b number    Define initial baud rate (default 115200)
 -c clk       Set the input clock rate (default 96000000)
 -C number    Size of canonical input buffer (default 256)
 -e           Set options to "edit" mode
 -E           Set options to "raw" mode (default)
 -I number    Size of raw input buffer (default 2048)
 -f           Enable hardware flow control (default)
 -F           Disable hardware flow control
 -m           Buffer mode - 0=buffer mode, 1=FIFO mode, default: 0
 -O number    Size of output buffer (default 2048)
 -s           Enable software flow control
 -S           Disable software flow control (default)
 -t number    Set receive FIFO trigger level ( 0 - 32; default 24)
 -T number    Set number of characters to send to transmit FIFO
                                             ( 2 - 32; default 32)
 -u unit      Set serial unit number (default 1)

Examples:

# Disable HW, SW flow control, enable edited mode for
# LINFLEXD0 on NXP S32V SOC
devc-serlinflexd -e -F -S -c66666667 0x40053000,91

#endif
*/
#include "externs.h"
#include <sys/mman.h>

/*
 * Specify parameters for default devices from hwi_info tags.
 */
int
query_hwi_device(TTYINIT_LINFLEXD *dip, unsigned unit)
{
    unsigned hwi_off = hwi_find_device("uart", unit);
    if(hwi_off != HWI_NULL_OFF) {
        hwi_tag *tag_location = hwi_tag_find(hwi_off, HWI_TAG_NAME_location, 0);
        if(tag_location) {
            dip->tty.port = tag_location->location.base;
        }
        hwi_tag *tag_irq = hwi_tag_find(hwi_off, HWI_TAG_NAME_irq, 0);
        if(tag_irq) {
            dip->intr = tag_irq->irq.vector;
        }
        return 1;
    }
    /* No default device, the base address and irq have been specified */
    return 0;
 }


unsigned
options(int argc, char *argv[])
{
    int         opt;
    int         numports = 0;
    unsigned    unit;
    unsigned    rx_fifo = 24;   /* default */
    unsigned    tx_fifo = 2;    /* default */
    TTYINIT_LINFLEXD    devinit = {
        {   .port       = 0,
            .port_shift = 0,
            .intr       = 0,
            .baud       = 115200,
            .isize      = 2048,
            .osize      = 2048,
            .csize      = 256,
            .c_cflag    = 0,
            .c_iflag    = 0,
            .c_lflag    = 0,
            .c_oflag    = 0,
            .fifo       = 0,
            .clk        = 96000000,
            .div        = 16,
            .name       = "/dev/ser"
        },
        .intr           = 0,
        .buffer_mode    = UART_MODE_BUFFER,
    };

    int found_hwi_device = -1;

    /*
     * Initialize the devinit to raw mode
     */
    ttc(TTC_INIT_RAW, &devinit, 0);

    unit = 0;

    /* Getting the UART Base addresss and irq from the Hwinfo Section if available */
    unsigned hwi_off = hwi_find_device("uart", 0);
    if(hwi_off != HWI_NULL_OFF){
        hwi_tag *tag_inputclk = hwi_tag_find(hwi_off, HWI_TAG_NAME_inputclk, 0);
        if(tag_inputclk){
            devinit.tty.clk = tag_inputclk->inputclk.clk;
        }
    }

    while (optind < argc) {
        /*
         * Process dash options.
         * Options already used by io-char (do not use these!): b,e,E,f,F,s,S,C,I,O,o,v
         */
        while ((opt = getopt(argc, argv, IO_CHAR_SERIAL_OPTIONS "t:T:c:u:d:m:")) != -1) {
            switch (ttc(TTC_SET_OPTION, &devinit, opt)) {
                case 'm':
                    devinit.buffer_mode = strtoul(optarg, NULL, 0);
                    if (devinit.buffer_mode >= UART_MODE_LAST){
                        fprintf(stderr, "Invalid Buffer mode, setting to buffer mode\n");
                        devinit.buffer_mode = UART_MODE_BUFFER;
                    }
                    if (devinit.buffer_mode == UART_MODE_FIFO){
                        fprintf(stderr, "Currently FIFO mode doesn't support, setting to buffer mode\n");
                        devinit.buffer_mode = UART_MODE_BUFFER;
                    }

                    break;
                case 't':
                    rx_fifo = strtoul(optarg, NULL, 0);
                    if (rx_fifo > 32) {
                        fprintf(stderr, "FIFO trigger must be <= 32.\n");
                        fprintf(stderr, "Will disable FIFO.\n");
                        rx_fifo = 0;
                    }
                    break;

                case 'T':
                    tx_fifo = strtoul(optarg, NULL, 0);
                    if ((tx_fifo > 32) || (tx_fifo < 2)) {
                        fprintf(stderr, "Tx fifo size must be >= 2 and <= 32.\n");
                        fprintf(stderr, "Using tx fifo size of 32\n");
                        tx_fifo = 2;
                    }

                    break;

                case 'c':
                    devinit.tty.clk = strtoul(optarg, &optarg, 0);
                    break;

                case 'u':
                    unit = strtoul(optarg, NULL, 0);
                    break;
            }
        }

        devinit.tty.fifo = rx_fifo | (tx_fifo << 10);

        /*
         * Process ports and interrupts.
         */
        while (optind < argc && *(optarg = argv[optind]) != '-') {
            devinit.tty.port = strtoul(optarg, &optarg, 16);
            if (*optarg == ',') {
                devinit.intr = strtoul(optarg + 1, &optarg, 0);
                if (*optarg == ',')
                    devinit.intr = strtoul(optarg + 1, &optarg, 0);
            }

            if (devinit.tty.port != 0 && devinit.intr != -1) {
                create_device(&devinit, unit++);
                ++numports;
            }
            ++optind;
        }
    }

    if (numports == 0) {
        unit = 0;
        devinit.tty.fifo = rx_fifo | (tx_fifo << 10);
        while (1) {
            found_hwi_device = query_hwi_device(&devinit,unit);
            if (!found_hwi_device)
                 break;
            create_device(&devinit, unit++);
            ++numports;
        }
    }

    return numports;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devc/serlinflexd/options.c $ $Rev: 810496 $")
#endif
