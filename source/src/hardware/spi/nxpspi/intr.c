/*
 * $QNXLicenseC:
 * Copyright 2017, QNX Software Systems.
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


#include "nxpspi.h"

/*
 * We use the same buffer for transmit and receive
 * For exchange, that's exactly what we wanted
 * For Read, it doesn't matter what we write to SPI, so we are OK.
 * For transmit, the receive data is put at the buffer we just transmitted, we are still OK.
 */

static const struct sigevent *spi_intr(void *area, int id)
{
    nxp_spi_t   *dev = area;
    uintptr_t   base = dev->vbase;
    uint32_t    data, tmp;
    int         count;
    uint32_t    val;

    /* check which interrupt we got */
    tmp = in32(base + NXP_SPI_SR);
    /* clear all set interrupts in Status reg */
    out32(base + NXP_SPI_SR, tmp);

    /* check if Rx FIFO overflow or Transmit FIFO invalid write flag bits are set in the Status reg */
    if ((tmp & NXP_SPI_SR_RFOF) || (tmp & NXP_SPI_SR_TFIWF)) {
        /* disable interrupt and return spievent */
        out32(base + NXP_SPI_RSER, 0);
        return (&dev->spievent);
    }

    if (tmp & NXP_SPI_SR_TCF) {
        /* check how many words received in RXFIFO */
        count = ((in32(base + NXP_SPI_SR) & NXP_SPI_SR_RXCTR) >> NXP_SPI_SR_RXCTR_POS);
        dev->irq_count++;
    }
    else {
        /* wrong interrupt, disable interrupt and return spievent */
        out32(base + NXP_SPI_RSER, 0);
        return (&dev->spievent);
    }

    while (count != 0) {
        // read recv fifo
        data = in32(base + NXP_SPI_POPR);
        dev->pbuf[dev->rlen++] = (uint8_t)data;
        count = ((in32(base + NXP_SPI_SR) & NXP_SPI_SR_RXCTR) >> NXP_SPI_SR_RXCTR_POS);

        // decrement the txfifo
        dev->txfifo--;

        // add data to txfifo if any
        if ((dev->tlen < dev->xlen) && (dev->txfifo < NXP_SPI_FIFO_SIZE)) {
            val = (NXP_SPI_PUSHR_CONT | (1 << NXP_SPI_PUSHR_PCS_POS) | dev->pbuf[dev->tlen]);

            /* Enable End Of Queue bit if this is the last frame to be tx'd */
            if (dev->tlen == (dev->xlen - 1)) {
                val |= NXP_SPI_PUSHR_EOQ;
                val &= ~NXP_SPI_PUSHR_CONT;
            }

            out32(base + NXP_SPI_PUSHR, val);

            dev->tlen++;
            dev->txfifo++;
        }
    }

    if (dev->rlen >= dev->xlen) {
        /* Disable interrupt and return spievent */
        out32(base + NXP_SPI_RSER, 0);
        return (&dev->spievent);
    }

    return 0;
}

int nxp_attach_intr(nxp_spi_t *nxp)
{
    if ((nxp->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1) {
        return -1;
    }

    if ((nxp->coid = ConnectAttach(0, 0, nxp->chid, _NTO_SIDE_CHANNEL, 0)) == -1) {
        goto fail0;
    }

    nxp->spievent.sigev_notify   = SIGEV_PULSE;
    nxp->spievent.sigev_coid     = nxp->coid;
    nxp->spievent.sigev_code     = NXP_SPI_EVENT;
    nxp->spievent.sigev_priority = NXP_SPI_PRIORITY;

    /*
     * Attach SPI interrupt
     */
    nxp->iid = InterruptAttach(nxp->irq, spi_intr, nxp, 0, _NTO_INTR_FLAGS_TRK_MSK);

    if (nxp->iid != -1) {
        return 0;
    }

    ConnectDetach(nxp->coid);
fail0:
    ChannelDestroy(nxp->chid);

    return -1;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/spi/nxpspi/intr.c $ $Rev: 823316 $")
#endif
