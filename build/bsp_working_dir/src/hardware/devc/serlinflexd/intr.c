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

/*
 * Process data in a line status register
 */

unsigned
process_lsr(DEV_LINFLEXD *dev, unsigned char lsr) {
    unsigned key = 0;

    /* Return immediately if no errors. */
    if((lsr & (LINFLEXD_LINESR_IDPEF|LINFLEXD_LINESR_FEF|LINFLEXD_LINESR_BOF)) == 0){
        return(0);
    }

    /* Save the error as out-of-band data which can be retrieved via devctl().*/
    dev->tty.oband_data |= (lsr >> 1) & 0x0f;
    atomic_set(&dev->tty.flags, OBAND_DATA);

    if(lsr & LINFLEXD_LINESR_BOF)
        key |= TTI_OVERRUN;
    else if(lsr & LINFLEXD_LINESR_FEF)
        key |= TTI_FRAME;
    else if(lsr & LINFLEXD_LINESR_IDPEF)
        key |= TTI_PARITY;

   return (key);
}


/**************** BUFFER MODE ************/
static inline int tx_interrupt_buffer(DEV_LINFLEXD *dev)
{
    uintptr_t   base = dev->base;

    out32(base + LINFLEXD_UARTSR, LINFLEXD_UARTSR_DTFTFF);

    dev->is_tx = 0;
    /* Send event to io-char, tto() will be processed at thread time */
    atomic_set(&dev->tty.flags, EVENT_TTO);

    return (1);
}

static inline int rx_interrupt_buffer(DEV_LINFLEXD *dev)
{
    int         status = 0;
    int         byte_count = 0;
    unsigned    rxdata;
    uintptr_t   base = dev->base;
    uint32_t    lesr;

    /* limit loop iterations by 32 to prevent ISR from running too long */
    while ((in32(base + LINFLEXD_UARTSR) & LINFLEXD_UARTSR_DRFRFE) && (byte_count < 32))
    {
        /* Clear data reception complete bit */
        out32(base + LINFLEXD_UARTSR, LINFLEXD_UARTSR_DRFRFE);

        /* Read next character from FIFO */
        rxdata = in8(base + LINFLEXD_BDRM);

        /* Read LIN Error status register */
        lesr = in32(base + LINFLEXD_LINESR) & LINFLEXD_LINESR_MSK;
        if ( lesr ) {
            rxdata |= process_lsr(dev, lesr);
            out32(base + LINFLEXD_LINESR, lesr);
        }

        /* Process char */
        status |= tti(&dev->tty, rxdata);

        /* Clear RMB */
        out32(base + LINFLEXD_UARTSR, LINFLEXD_UARTSR_RMB);

        byte_count++;
    }

    return status;
}

static inline int do_interrupt_buffer(DEV_LINFLEXD *dev, int id)
{
    int sts = 0;

    if (in32(dev->base + LINFLEXD_UARTSR) & LINFLEXD_UARTSR_DRFRFE)
        sts = rx_interrupt_buffer(dev);

    /* TX interrupt */
    if (in32(dev->base + LINFLEXD_UARTSR) & LINFLEXD_UARTSR_DTFTFF)
        sts |= tx_interrupt_buffer(dev);

    return sts;
}

/*
 * Serial interrupt handler when LINFLEXD buffer configured in buffer mode
 */
static const struct sigevent * ser_intr_buffer(void *area, int id)
{
    DEV_LINFLEXD    *dev = area;

    if (do_interrupt_buffer(dev,id) && (dev->tty.flags & EVENT_QUEUED) == 0) {
        dev_lock(&ttyctrl);
        ttyctrl.event_queue[ttyctrl.num_events++] = &dev->tty;
        atomic_set(&dev->tty.flags, EVENT_QUEUED);
        dev_unlock(&ttyctrl);
        return &ttyctrl.event;
    }

    return 0;
}
/*****************************************/

/**************** FIFO MODE ************/
static inline int rx_interrupt_fifo(DEV_LINFLEXD *dev)
{
    int         status = 0;
    int         byte_count = 0;
    unsigned    rxdata;
    uintptr_t   base = dev->base;

    /* Reset timer */
    out32(base + LINFLEXD_UARTSR, in32(base + LINFLEXD_UARTSR) | LINFLEXD_UARTSR_TO);

    /* limit loop iterations by FIFO size to prevent ISR from running too long */
    while ((in32(base + LINFLEXD_UARTSR) & LINFLEXD_UARTSR_RFNE) && byte_count < FIFO_SIZE)
    {
        /*
         * Read next character from FIFO
         */
        rxdata = in8(base + LINFLEXD_BDRM);
        status |= tti(&dev->tty, rxdata);
        byte_count++;
    }

    return status;
}

static inline int do_interrupt_fifo(DEV_LINFLEXD *dev, int id)
{
    int sts=0;

    /* RX timer interrupt */
    if (in32(dev->base + LINFLEXD_UARTSR) & LINFLEXD_UARTSR_TO)
        sts = rx_interrupt_fifo(dev);

     return sts;
}

/*
 * Serial interrupt handler when LINFLEXD buffer configured in FIFO mode
 */
static const struct sigevent * ser_intr_fifo(void *area, int id)
{
    DEV_LINFLEXD    *dev = area;

    if (do_interrupt_fifo(dev,id) && (dev->tty.flags & EVENT_QUEUED) == 0) {
        dev_lock(&ttyctrl);
        ttyctrl.event_queue[ttyctrl.num_events++] = &dev->tty;
        atomic_set(&dev->tty.flags, EVENT_QUEUED);
        dev_unlock(&ttyctrl);
        return &ttyctrl.event;
    }

    return 0;
}
/*****************************************/


/**************** Common Functions (for both Buffer and FIFO mode) ************/
void ser_attach_intr(DEV_LINFLEXD *dev)
{
    uintptr_t    base = dev->base;
    uint32_t     ier;

    /* Disable TX/RX interrupts */
    ier = in32(base + LINFLEXD_LINIER);
    ier &= ~(LINFLEXD_LINIER_DTIE | LINFLEXD_LINIER_DRIE);
    out32(base + LINFLEXD_LINIER, ier);

    slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO, "%s: Attaching to interrupt %d", __FUNCTION__, dev->intr);
    if (dev->buffer_mode == UART_MODE_BUFFER)
        dev->iid = InterruptAttach(dev->intr, ser_intr_buffer, dev, 0, _NTO_INTR_FLAGS_TRK_MSK);
    else /* FIFO mode */
        dev->iid = InterruptAttach(dev->intr, ser_intr_fifo, dev, 0, _NTO_INTR_FLAGS_TRK_MSK);

    /* Re-enable interrupts */
    ier = in32(base + LINFLEXD_LINIER);
    ier |= (LINFLEXD_LINIER_DTIE | LINFLEXD_LINIER_DRIE);
    out32(base + LINFLEXD_LINIER, ier);
}

void ser_detach_intr(DEV_LINFLEXD *dev)
{
    uintptr_t    base = dev->base;
    uint32_t     ier;

    /* Disable TX/RX interrupts */
    ier = in32(base + LINFLEXD_LINIER);
    ier &= ~(LINFLEXD_LINIER_DTIE | LINFLEXD_LINIER_DRIE);
    out32(base + LINFLEXD_LINIER, ier);

    InterruptDetach(dev->iid);
    dev->intr = -1;
}
/*****************************************/


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devc/serlinflexd/intr.c $ $Rev: 886103 $")
#endif
