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

/* convert reference clock divider to bit value which can be programmed to UFCR register */
#define REF_CLK_DIV_REG(x)      (((x) < 7 ? 6 - (x) : 6) << 7)

#define MIN_REF_FRQ_DIV (1)
#define MAX_REF_FRQ_DIV	(7)

#define UINT_DIFFERENCE(a,b)    ((a > b) ? (a-b) : (b-a))

/* Round last digit during division */
#define DIVIDE_AND_ROUND(A,B)   (((A) + ((B)-1))/(B))

int
tto(TTYDEV *ttydev, int action, int arg1)
{
    TTYBUF              *bup = &ttydev->obuf;
    DEV_LINFLEXD        *dev = (DEV_LINFLEXD *)ttydev;
    uintptr_t           base = dev->base;
    unsigned char       c;

    switch (action)
    {
        case TTO_STTY:
            ser_stty(dev);
            return 0;

        case TTO_CTRL:
            /* LinFlexd doesn't support Break control and Hardware Flow Control */ 
            return 0;

        case TTO_LINESTATUS:
            return (in32(base + LINFLEXD_LINESR));

        case TTO_DATA:
        case TTO_EVENT:
            break;

        default:
            return 0;
    }

    if (dev->buffer_mode == UART_MODE_BUFFER){
        if (bup->cnt > 0) {
            /*
             * Get the next character to print from the output buffer
             * after Data Transmission Complete
             */
            if (dev->is_tx == 0) {
                dev->is_tx = 1;
                dev_lock(&dev->tty);
                c=tto_getchar(&dev->tty);
                dev_unlock(&dev->tty);
                out8(base + LINFLEXD_BDRL, c);
                atomic_clr(&dev->tty.xflags, OSW_PAGED_OVERRIDE);
             }
        }
    }
    else    /* FIFO mode */
    {
        /* Keep transmitting bytes until FIFO is full */
        while (bup->cnt > 0 && !((in32(base + LINFLEXD_UARTSR) & LINFLEXD_UARTSR_DTFTFF)))
        {
            /*
             * If the OSW_PAGED_OVERRIDE flag is set then allow
             * transmit of character even if output is suspended via
             * the OSW_PAGED flag. This flag implies that the next
             * character in the obuf is a software flow control
             * charater (STOP/START).
             * Note: tx_inject sets it up so that the contol
             *        character is at the start (tail) of the buffer.
             */
            if (dev->tty.flags & (OHW_PAGED|OSW_PAGED) && !(dev->tty.xflags & OSW_PAGED_OVERRIDE))
                break;

            /*
             * Get the next character to print from the output buffer
             */
            dev_lock(&dev->tty);
            c=tto_getchar(&dev->tty);
            dev_unlock(&dev->tty);

            dev->tty.un.s.tx_tmr = 1;	/* Make timer as small as possible - 50ms */
            out8(base + LINFLEXD_BDRL, c);

            /* Clear the OSW_PAGED_OVERRIDE flag as we only want
             * one character to be transmitted in this case.
             */
            if (dev->tty.xflags & OSW_PAGED_OVERRIDE){
                atomic_clr(&dev->tty.xflags, OSW_PAGED_OVERRIDE);
                break;
            }
        }
    }

    return (tto_checkclients(&dev->tty));
}

void ser_stty(DEV_LINFLEXD *dev)
{
    uintptr_t base = dev->base;
    unsigned uartcr;

    /* Disable Receiver and Transmitter */
    uartcr = in32(dev->base + LINFLEXD_UARTCR);
    uartcr &= ~(LINFLEXD_UARTCR_RXEN | LINFLEXD_UARTCR_TXEN);
    out32(dev->base + LINFLEXD_UARTCR, uartcr);

    if ( dev->tty.baud != 0 && dev->clk != 0 ) {
        /* Set baud rate */
        out32(base + LINFLEXD_LINIBRR, dev->clk/(dev->tty.baud * 16));
    }

    /* Determine data bits */
    switch (dev->tty.c_cflag & CSIZE)
    {
        case CS7:
            uartcr &= ~(LINFLEXD_UARTCR_WL0 | LINFLEXD_UARTCR_WL1);
            break;

        case CS8:
        default:
            uartcr &= ~LINFLEXD_UARTCR_WL1;
            uartcr |= LINFLEXD_UARTCR_WL0;
            break;
    }

    /* Determine stop bits. */
    uartcr &= ~LINFLEXD_UARTCR_SBUR_MASK;
    /* CSTOPB indicates two stop bits, else one stop bit */
    if (dev->tty.c_cflag & CSTOPB)
        uartcr |= LINFLEXD_UARTCR_SBUR;

    /* Determine Parity bits */
    uartcr &= ~(LINFLEXD_UARTCR_PCE | LINFLEXD_UARTCR_PC0 | LINFLEXD_UARTCR_PC1);
    if (dev->tty.c_cflag & PARENB) {
        uartcr |= LINFLEXD_UARTCR_PCE;
        if (dev->tty.c_cflag & PARODD)
            uartcr |= LINFLEXD_UARTCR_PC0;
    }

    /* select UART mode and re-enable Receiver and Transmitter */
    uartcr |= (LINFLEXD_UARTCR_RXEN | LINFLEXD_UARTCR_TXEN | LINFLEXD_UARTCR_UART);
    out32(base + LINFLEXD_UARTCR, uartcr);

}


int
drain_check (TTYDEV * ttydev, uintptr_t * count)
{
    TTYBUF              *bup = &ttydev->obuf;
    DEV_LINFLEXD        *dev = (DEV_LINFLEXD *)ttydev;

    /* if the device has DRAINED, return 1 */
    if ((bup->cnt == 0) && !dev->is_tx)
        return 1;

    /* if the device has not DRAINED, set a timer based on 50ms
     * counts wait for the time it take for one character to the
     * transmitted out the shift register. We do this dynamically
     * since the baud rate can change.
     */
    if(count != NULL)
        *count = (ttydev->baud == 0) ? 0: ((IO_CHAR_DEFAULT_BITSIZE * 20) / ttydev->baud) + 1;
    return 0;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devc/serlinflexd/tto.c $ $Rev: 886103 $")
#endif

