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
#include <stdbool.h>
#include <stddef.h>


enum opt_index {BASE, IRQ, CLOCK, CSD, END};

static char *nxp_opts[] = {
    [BASE]      =   "base",         /* Base address for this CSPI controller */
    [IRQ]       =   "irq",          /* IRQ for this CSPI intereface */
    [CLOCK]     =   "clock",        /* SPI clock */
    [CSD]       =   "csdelay",      /* Chip select delay between chip select active edge and first SPI clock edge */
    [END]       =   NULL
};

spi_funcs_t spi_drv_entry = {
    sizeof(spi_funcs_t),
    nxp_init,       /* init() */
    nxp_dinit,      /* fini() */
    nxp_drvinfo,    /* drvinfo() */
    nxp_devinfo,    /* devinfo() */
    nxp_setcfg,     /* setcfg() */
    nxp_xfer,       /* xfer() */
    NULL            /* dma_xfer() */
};

/*
 * Note:
 * The devices listed are just examples, users should change
 * this according to their own hardware spec.
 */
static spi_devinfo_t devlist[NXP_SPI_CHANNEL_MAX] = {
    {
        0x00,               // Device ID, for SS0
        "SPI-DEV0",         // Description
        {
            // data length 8bit, clock active high CPOL 0, CPHA 0, data transfer MSB first
            8 | SPI_MODE_CKPOL_HIGH | SPI_MODE_CKPHASE_HALF | SPI_MODE_BODER_MSB,
            1000000         // Clock rate 1M
        },
    },
    {
        0x01,               // Device ID, for SS1
        "SPI-DEV1",         // Description
        {
            // data length 8bit, clock active high CPOL 0, CPHA 0, data transfer MSB first
            8 | SPI_MODE_CKPOL_HIGH | SPI_MODE_CKPHASE_HALF | SPI_MODE_BODER_MSB,
            5000000         // Clock rate 5M
        },
    },
    {
        0x02,               // Device ID, for SS2
        "SPI-DEV2",         // Description
        {
            // data length 8bit, clock active high CPOL 0, CPHA 0, data transfer MSB first
            8 | SPI_MODE_CKPOL_HIGH | SPI_MODE_CKPHASE_HALF | SPI_MODE_BODER_MSB,
            5000000         // Clock rate 5M
        },
    },
    {
        0x03,               // Device ID, for SS3
        "SPI-DEV3",         // Description
        {
            // data length 8bit, clock active high CPOL 0, CPHA 0, data transfer MSB first
            8 | SPI_MODE_CKPOL_HIGH | SPI_MODE_CKPHASE_HALF | SPI_MODE_BODER_MSB,
            5000000         // Clock rate 5M
        },
    }
};

static uint32_t devctrl[NXP_SPI_CHANNEL_MAX] = {0,0,0,0};
static uint32_t devcfg[NXP_SPI_CHANNEL_MAX]  = {0,0,0,0};

static int nxp_options(nxp_spi_t *dev, char *optstring)
{
    int     opt, rc = 0;
    char    *options, *freeptr, *c, *value;

    if (optstring == NULL) {
        return 0;
    }

    freeptr = options = strdup(optstring);

    while (options && *options != '\0') {
        c = options;
        if ((opt = getsubopt(&options, nxp_opts, &value)) == -1) {
            goto error;
        }

        switch (opt) {
            case BASE:
                dev->pbase = strtoul(value, 0, 0);
                continue;
            case IRQ:
                dev->irq   = strtoul(value, 0, 0);
                continue;
            case CLOCK:
                dev->clock = strtoul(value, 0, 0);
                continue;
            case CSD:
                dev->csdelay = strtoul(value, 0, 0);
                continue;
        }
error:
        fprintf(stderr, "nxpspi: unknown option %s", c);
        rc = -1;
    }

    free(freeptr);

    return rc;
}

void *nxp_init(void *hdl, char *options)
{
    nxp_spi_t   *dev;
    uintptr_t   base;
    int         i;

    dev = calloc(1, sizeof(nxp_spi_t));

    if (dev == NULL) {
        return NULL;
    }

    /* Set defaults */
    dev->pbase = NXP_SPI_BASE;
    dev->irq   = NXP_SPI_IRQ;
    dev->clock = NXP_SPI_CLK;    // 133 MHz SPI clock
    dev->csdelay = 0;

    if (nxp_options(dev, options)) {
        goto fail0;
    }


    if (ThreadCtl(_NTO_TCTL_IO_PRIV, 0)  == -1) {
        fprintf(stderr, "ThreadCtl failed\n");
        goto fail0;
    }
    /*
     * Map in SPI registers
     */
    if ((base = mmap_device_io(NXP_SPI_SIZE, dev->pbase)) == (uintptr_t)MAP_FAILED) {
        fprintf(stderr, "Mapping of SPI registers failed\n");
        goto fail0;
    }

    dev->vbase = base;
    /* Enable SPI interface and set Master mode, clear tx and rx fifos, and set it to halt state */
    out32(base + NXP_SPI_MCR,
            NXP_SPI_MCR_MSTR | NXP_SPI_MCR_CLR_TXF | NXP_SPI_MCR_CLR_RXF | NXP_SPI_MCR_HALT);

    /* TODO set wait states and chip select delay */

    /*
     * Calculate all device configuration here
     * we are using only CTAR0
     */
    for (i = 0; i < NXP_SPI_CHANNEL_MAX; i++) {
        nxp_setcfg(dev, i, &devlist[i].cfg);
    }

    /* Disable interrupts */
    out32(base + NXP_SPI_RSER, 0);
    /*
     * Attach SPI interrupt
     */
    if (nxp_attach_intr(dev)) {
        fprintf(stderr, "Attaching interrupt failed\n");
        goto fail1;
    }

    dev->spi.hdl = hdl;

    return dev;

fail1:
    munmap_device_io(dev->vbase, NXP_SPI_SIZE);
fail0:
    free(dev);
    return NULL;
}

void nxp_dinit(void *hdl)
{
    nxp_spi_t   *dev = hdl;

    /* Disable interrupts */
    out32(dev->vbase + NXP_SPI_RSER, 0);
    /*
     * unmap the register, detach the interrupt
     */
    InterruptDetach(dev->iid);
    ConnectDetach(dev->coid);
    ChannelDestroy(dev->chid);

    /*
     * Halt SPI frames, and set module disable mode
     */
    out32(dev->vbase + NXP_SPI_MCR,
            NXP_SPI_MCR_MDIS | NXP_SPI_MCR_HALT);

    munmap_device_io(dev->vbase, NXP_SPI_SIZE);
    free(hdl);
}

int nxp_drvinfo(void *hdl, spi_drvinfo_t *info)
{
    info->version = (SPI_VERSION_MAJOR << SPI_VERMAJOR_SHIFT) |
                    (SPI_VERSION_MINOR << SPI_VERMINOR_SHIFT) |
                    (SPI_REVISION << SPI_VERREV_SHIFT);
    strcpy(info->name, "NXP SPI");
    info->feature = 0;
    return (EOK);
}

int nxp_setcfg(void *hdl, uint16_t device, spi_cfg_t *cfg)
{
    nxp_spi_t   *dev = hdl;

    if (device >= NXP_SPI_CHANNEL_MAX) {
        return (EINVAL);
    }

    memcpy(&devlist[device].cfg, cfg, sizeof(spi_cfg_t));

    devcfg[device] = nxp_cfg(dev, &devlist[device].cfg);

    if (devcfg[device]  == 0)
    {
        return (EINVAL);
    }

    /* set SCLK polarity, CPOL */
    if (devlist[device].cfg.mode & SPI_MODE_CKPOL_HIGH) {
        devcfg[device] &= ~NXP_SPI_CTAR0_CPOL;    // set CPOL=0
    }
    else {
        devcfg[device] |= NXP_SPI_CTAR0_CPOL;     // set CPOL=1
    }

    /* set SCLK phase, CPHA */
    if (devlist[device].cfg.mode & SPI_MODE_CKPHASE_HALF) {
        devcfg[device] |= NXP_SPI_CTAR0_CPHA;    // set CPHA=1
    }
    else {
        devcfg[device] &= ~NXP_SPI_CTAR0_CPHA;     // set CPHA=0
    }

    /* set bit transfer order */
    if (devlist[device].cfg.mode & SPI_MODE_BODER_MSB) {
        devcfg[device] &= ~NXP_SPI_CTAR0_LSBFE;    // set data transfer MSB first
    }
    else {
        devcfg[device] |= NXP_SPI_CTAR0_LSBFE;    // set data transfer LSB first
    }

    /* set slave select (SS) polarity, 0 = active low, 1 = active high
     * note that SCLK, CS polarity bit settings are in different registers */
    if (devlist[device].cfg.mode & SPI_MODE_CSPOL_HIGH) {
        devctrl[device] &= ~(1 << (device + NXP_SPI_MCR_PCSIS_POS));    // SS active high
    }
    else {
        devctrl[device] |= (1 << (device + NXP_SPI_MCR_PCSIS_POS));     // SS active low
    }

    return (EOK);
}

int nxp_devinfo(void *hdl, uint32_t device, spi_devinfo_t *info)
{
    int dev = device & SPI_DEV_ID_MASK;

    if (device & SPI_DEV_DEFAULT) {
        /* Info of this device */
        if (dev >= 0 && dev < NXP_SPI_CHANNEL_MAX) {
            memcpy(info, &devlist[dev], sizeof(spi_devinfo_t));
        }
        else {
            return (EINVAL);
        }
    }
    else {
        /* Info of next device */
        if (dev == SPI_DEV_ID_NONE) {
            dev = -1;
        }

        dev++;    // get next device number
        if (dev >= 0 && dev < NXP_SPI_CHANNEL_MAX) {
            memcpy(info, &devlist[dev], sizeof(spi_devinfo_t));
        }
        else {
            return (EINVAL);
        }
    }

    return (EOK);
}

static void buf_swap(nxp_spi_t *dev, uint8_t *buf, int len)
{
    int i;
    uint32_t * tmp_buf = (uint32_t *)buf;

    switch(dev->emulate_8b){
        case 4:
            for(i=0; i<len/4; i++){
                ENDIAN_SWAP32(tmp_buf);
                tmp_buf++;
            }
            break;
        default:
            break;
    }
}

void *nxp_xfer(void *hdl, uint32_t device, uint8_t *buf, int *len)
{
    nxp_spi_t   *dev = hdl;
    uintptr_t   base = dev->vbase;
    uint32_t    id;
    uint32_t    cfg, ctrl, val;
    uint8_t     count =0;
    uint32_t    sr = 0;

    id = device & SPI_DEV_ID_MASK;

    if (id >= NXP_SPI_CHANNEL_MAX) {
        *len = -1;
        fprintf(stderr, "nxp_xfer: Unexpected device (id=%d)\n", id);
        return buf;
    }

    dev->xlen = *len;   // total exchange length
    dev->rlen = 0;      // total receive length
    dev->tlen = 0;      // total transmit length
    dev->txfifo = 0;    // current tx fifo level
    dev->irq_count = 0; // number of irq events


    // check if need swap buf
    if(dev->emulate_8b != 0){
        if (devlist[id].cfg.mode & SPI_MODE_BODER_MSB) {
            buf_swap(dev, buf, dev->xlen);
        }
    }

    dev->pbuf = buf;
    dev->dlen = ((devlist[id].cfg.mode & SPI_MODE_CHAR_LEN_MASK) + 7) >> 3;

    if ((dev->xlen == 0) || ((dev->xlen % dev->dlen) != 0)) {
        *len = -1;
        fprintf(stderr, "nxp_xfer: Unexpected exchange data length %d (word length is %d)\n", dev->xlen, dev->dlen);
        return buf;
    }

    /* Estimate transfer time in us...
     * The calculated dtime is only used for the timeout, so it doesn't have to be that accurate.
     * The dtime value should round up to the next integer value; and at higher clock rates,
     * a calcuated dtime of 0 would mess-up the timeout calculation.
     * So always add up 1us here.
     */
    dev->dtime = dev->dlen * 8 * 1000 * 1000 / devlist[id].cfg.clock_rate;
    dev->dtime++;

    /* Disable Interrupts */
    out32(base + NXP_SPI_RSER, 0x0);

    cfg = devcfg [id];
    ctrl = devctrl[id];

    // flush RXFIFO and set cs polarity
    out32( base + NXP_SPI_MCR,
        (in32(base + NXP_SPI_MCR) & ~NXP_SPI_MCR_PCSIS_BITS) | NXP_SPI_MCR_CLR_RXF | ctrl);

    /* Clear set flags in Status reg */
    out32(base + NXP_SPI_SR, in32(base + NXP_SPI_SR)) ;

    // write to CTAR0 register
    // The values for NXP_SPI_CTAR0_PBR, NXP_SPI_CTAR0_CPHA, NXP_SPI_CTAR0_CPOL are passed in cfg
    out32(base + NXP_SPI_CTAR0, (NXP_SPI_CTAR0_ASC_32 | cfg | NXP_SPI_CTAR0_PCSSCK_3 | NXP_SPI_CTAR0_CSSCK_32 | NXP_SPI_CTAR0_PASC_3));

    /* Enable Tx Complete Request Enable, Transmit FIFO Invalid Write Request Enable and Receive FIFO Overflow Request Enable interrupts */
    out32(base + NXP_SPI_RSER, NXP_SPI_RSER_TCF_RE | NXP_SPI_RSER_TFIWF_RE | NXP_SPI_RSER_RFOF_RE );

    if ((dev->rlen < dev->xlen) && (dev->tlen < dev->xlen)) {

        // write tx buffer
        while ((dev->tlen < dev->xlen) && (dev->txfifo < NXP_SPI_FIFO_SIZE)) {
            // write one buffer of 16-bits CMDFIFO and 8-bits data
            val = (NXP_SPI_PUSHR_CONT | (1 << NXP_SPI_PUSHR_PCS_POS) | buf[dev->tlen]);

            if (dev->tlen == (dev->xlen - 1)) {
                /* Enable End Of Queue bit if this is the last frame to be tx'd */
                val |= NXP_SPI_PUSHR_EOQ;
                val &= ~NXP_SPI_PUSHR_CONT;
            }

            out32(base + NXP_SPI_PUSHR, val);

            dev->tlen++;
            dev->txfifo++;
        }

        /* Start exchange */
        out32(base + NXP_SPI_MCR,
            in32(base + NXP_SPI_MCR) & ~NXP_SPI_MCR_HALT);

        /*
         * Wait for exchange to finish
         */
        if (nxp_wait(dev, dev->xlen)) {
            fprintf(stderr, "spi-nxpspi: XFER Timeout!!!\n");
            sr = in32(base + NXP_SPI_SR);

            /* Check how many words received in RXFIFO */
            count = ((in32(base + NXP_SPI_SR) & NXP_SPI_SR_RXCTR) >> NXP_SPI_SR_RXCTR_POS);
            fprintf(stderr, "SR reg 0x%x, words in RXFIFO %d, dev->rlen %d\n", sr, count, dev->rlen);
        }

    }

    *len = dev->rlen;

    /* Halt SPI and disable interrupts */
    out32(base + NXP_SPI_MCR,
        in32(base + NXP_SPI_MCR) | NXP_SPI_MCR_HALT);
    out32(base + NXP_SPI_RSER, 0);

    // clear set bits in Status Reg
    out32(base + NXP_SPI_SR, in32(base + NXP_SPI_SR));

    // check if need swap buf
    if(dev->emulate_8b != 0){
        if (devlist[id].cfg.mode & SPI_MODE_BODER_MSB) {
            buf_swap(dev, buf, dev->rlen);
        }
    }

    return buf;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/spi/nxpspi/nxpspi.c $ $Rev: 847655 $")
#endif
