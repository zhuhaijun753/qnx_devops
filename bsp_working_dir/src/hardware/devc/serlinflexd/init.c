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

#include "externs.h"
#include <sys/mman.h>
#include <string.h>

static void free_device( DEV_LINFLEXD *dev, int step ) {
    switch ( step ) {
        /* The failing through is intentional */
        default:
        case 5:
            ser_detach_intr(dev);
        case 4:
            munmap_device_io(dev->base, sysconf(_SC_PAGESIZE));
        case 3:
            free(dev->tty.cbuf.buff);
        case 2:
            free(dev->tty.obuf.buff);
        case 1:
            free(dev->tty.ibuf.buff);
        case 0:
            free(dev);
        break;
    }
}

void clear_device(DEV_LINFLEXD *dev) {
    unsigned    ier;
    uintptr_t	base = dev->base;

    /* Disable TX/RX interrupts */
    ier = in32(base + LINFLEXD_LINIER);
    ier &= ~(LINFLEXD_LINIER_DTIE | LINFLEXD_LINIER_DRIE);
    out32(base + LINFLEXD_LINIER, ier);
}

DEV_LINFLEXD *
create_device(TTYINIT_LINFLEXD *dip, unsigned unit)
{
    DEV_LINFLEXD *dev;
    unsigned uartcr, lincr1;

    /*
     * Get a device entry and the input/output buffers for it.
     */
    dev = calloc(1, sizeof(*dev));
    if(dev == NULL){
        slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "%s: LINFLEXD UART: Unable to allocate device entry", __FUNCTION__);
        return (dev);
    }

    /* Get buffers */
    dev->tty.ibuf.size = dip->tty.isize;
    dev->tty.ibuf.head = dev->tty.ibuf.tail = dev->tty.ibuf.buff = (unsigned char *) malloc(dev->tty.ibuf.size);
    if (dev->tty.ibuf.buff == NULL) {
        slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "io-char: Allocation of input buffer failed (%d)", errno);
        free_device(dev, 0);
        return (NULL);
    }

    dev->tty.obuf.size = dip->tty.osize;
    dev->tty.obuf.head = dev->tty.obuf.tail = dev->tty.obuf.buff = (unsigned char *) malloc(dev->tty.obuf.size);
    if (dev->tty.obuf.buff == NULL) {
        slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "io-char: Allocation of output buffer failed (%d)", errno);
        free_device(dev, 1);
        return (NULL);
    }

    dev->tty.cbuf.size = dip->tty.csize;
    dev->tty.cbuf.head = dev->tty.cbuf.tail = dev->tty.cbuf.buff = (unsigned char *) malloc(dev->tty.cbuf.size);
    if (dev->tty.cbuf.buff == NULL) {
        slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "io-char: Allocation of canonical buffer failed (%d)", errno);
        free_device(dev, 2);
        return (NULL);
    }

    if (dev->tty.ibuf.size < 128)
        dev->tty.highwater = dev->tty.ibuf.size/4;
    else
        dev->tty.highwater = 100;

    if (dev->tty.highwater <= 0) {
        free_device(dev, 3);
        return (NULL);
    }

    strlcpy(dev->tty.name, dip->tty.name, TTY_NAME_MAX);

    dev->tty.baud    = dip->tty.baud;

    // Buffer mode
    dev->buffer_mode = dip->buffer_mode;

    dev->tty.flags = EDIT_INSERT;
    dev->tty.c_cflag = dip->tty.c_cflag;
    dev->tty.c_iflag = dip->tty.c_iflag;
    dev->tty.c_lflag = dip->tty.c_lflag;
    dev->tty.c_oflag = dip->tty.c_oflag;
    dev->tty.verbose = dip->tty.verbose;
    dev->tty.fifo    = dip->tty.fifo;

    dev->fifo        = dip->tty.fifo;
    dev->intr        = dip->intr;
    dev->clk         = dip->tty.clk;
    dev->div         = dip->tty.div;

    /*
     * Map device registers
     */
    dev->base = mmap_device_io(sysconf(_SC_PAGESIZE), dip->tty.port);
    if (dev->base == (uintptr_t)MAP_FAILED) {
        perror("Linflexd UART: MAP_FAILED\n");
        free_device(dev, 3);
	return (NULL);
    }

    /*
     * Initialize termios cc codes to an ANSI terminal.
     */
    ttc(TTC_INIT_CC, &dev->tty, 0);

    /*
     * Initialize the device's name.
     * Assume that the basename is set in device name.  This will attach
     * to the path assigned by the unit number/minor number combination
     */
    unit = SET_NAME_NUMBER(unit) | NUMBER_DEV_FROM_USER;
    ttc(TTC_INIT_TTYNAME, &dev->tty, unit);

    /* Disable Receiver and Transmitter */
    uartcr = in32(dev->base + LINFLEXD_UARTCR);
    uartcr &= ~(LINFLEXD_UARTCR_RXEN | LINFLEXD_UARTCR_TXEN);
    out32(dev->base + LINFLEXD_UARTCR, uartcr);

    /* Enter initialization mode */
    lincr1 = in32(dev->base + LINFLEXD_LINCR1);
    lincr1 |= LINFLEXD_LINCR1_INIT;
    out32(dev->base + LINFLEXD_LINCR1, lincr1);

    /* configure buffer */
    if (dev->buffer_mode == UART_MODE_BUFFER)
        uartcr &= ~(LINFLEXD_UARTCR_RFBM | LINFLEXD_UARTCR_TFBM);
    else
        uartcr |= (LINFLEXD_UARTCR_RFBM | LINFLEXD_UARTCR_TFBM);

    /* select UART mode and re-enable Receiver and Transmitter */
    uartcr |= (LINFLEXD_UARTCR_RXEN | LINFLEXD_UARTCR_TXEN | LINFLEXD_UARTCR_UART);

    /* Set baud rate using integer baud rate divider */
    out32(dev->base + LINFLEXD_LINIBRR, dev->clk/(dev->tty.baud * 16));

    /* Disable fractional divider - this could be used in the future to use a more accurate baud rate*/
    out32(dev->base + LINFLEXD_LINFBRR, 0);

    out32(dev->base + LINFLEXD_UARTCR, uartcr);

    /* Transition from INIT mode to NORMAL mode */
    lincr1 &= ~LINFLEXD_LINCR1_INIT;
    out32(dev->base + LINFLEXD_LINCR1, lincr1);

    /* Configure buffer mode or FIFO mode settings */
    if (dev->buffer_mode == UART_MODE_BUFFER) {
        /* TX - interrupt fires when data transmission is complete */
        /* RX - interrupt fires when numner of bytes programmed in RFBM are received */
        out32(dev->base + LINFLEXD_LINIER, LINFLEXD_LINIER_DTIE | LINFLEXD_LINIER_DRIE);
    }
    else
    {
        /*
         * Note - S32V Reference Manual doesn't clearly indicate a mechanism for interrupt on RX activity
         * when in FIFO mode. The S32V Reference Manual briefly describes a Timeout mechanism which can also
         * be used to get an interrupt after RX activity.
         * Use the Timeout mechanism for now, although we appear to get a lot of false interrupts
         * and Freescale has confirmed that there is an issue with the timeout mechanism and an errata
         * will be created.
         *
         * S32V also doesn't have an interrupt for TX completion in FIFO mode so we rely on io-char's 50ms timer for now.
         */
        out32(dev->base + LINFLEXD_LINIER, LINFLEXD_LINIER_TOIE);
    }


    if (dip->tty.port != 0 && dev->intr != -1) {
        /* init */
        ser_stty(dev);

        /* Attach to interrupt */
        ser_attach_intr(dev);
    }
    else {
        slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR,
			"%s: LINFLEXD UART: Unable to determine LINFLEXD base address or IRQ, exiting", __FUNCTION__);
        free_device(dev, 3);
    }

    /* Attach the resource manager */
    ttc(TTC_INIT_ATTACH, &dev->tty, 0);

    return (dev);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devc/serlinflexd/init.c $ $Rev: 886103 $")
#endif
