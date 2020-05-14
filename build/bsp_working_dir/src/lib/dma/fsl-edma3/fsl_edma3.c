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


#include "fsl_edma3.h"


/////////////////
// global vars //
/////////////////
static int fsl_edma3_num = 0;
static int slots_num = 0;

static fsl_edma3_t * fsl_edma3[EDMA3_MAX_NUM];
static fsl_edma3_soc_t soc_type = SOC_UNKNOWN;


// mutex
static pthread_mutex_t processinit_mutex = PTHREAD_MUTEX_INITIALIZER;
static int n_users_in_process = 0;

static char *dma_opts[] = {
#define EDMA3_INTERFACE 0
    "edma3",          // edma3 interface #
    NULL
}

////////////////////////////////////////////////////////////////////////////////
//                            PRIVATE FUNCTIONS                               //
////////////////////////////////////////////////////////////////////////////////
;

static int
dma_parse_options(dma_channel_t *chan, char *options)
{
    char   *value;
    int     opt;

    while (options && *options != '\0') {
        if ((opt = getsubopt(&options, dma_opts, &value)) == -1) {
            return EINVAL;
        }

        switch (opt) {
            case EDMA3_INTERFACE:
                chan->edma3_id = strtoul(value, 0, 0);
                break;
            default:
                return EINVAL;
        }
    }
    return EOK;
}


static int
halt_channel (dma_channel_t* chan)
{
    uintptr_t            edma3_base;
    int                  ch;

    /* Validate current channel and its state */
    if(chan == NULL){
        errno = EINVAL;
        return -1;
    }

    edma3_base = fsl_edma3[chan->edma3_id]->fsl_edma3_base;
    ch = chan->id - chan->edma3_id * CHANS_PER_EDMA3;

    /* Nothing to be done if channel is already stop! */
    if (chan->running == 0)
        return 0;

    pthread_mutex_lock( fsl_edma3_sync_libinit_mutex_get() );

    /* Disable */
    out32(edma3_base + EDMA3_CHn_CSR(ch), 0);

    chan->running = 0;
    chan->tcdCtrl.currBDInUse = 0;

    pthread_mutex_unlock(fsl_edma3_sync_libinit_mutex_get() ); //done share init

    return 0;
}


#define DMA_XFER_TYPE_FLAG   ( DMA_ADDR_FLAG_IO | DMA_ADDR_FLAG_MEMORY | DMA_ADDR_FLAG_DEVICE)

static dma_channel_caps
get_xfer_type ( dma_xfer_flags src_flags, dma_xfer_flags dst_flags)
{
    static uint32_t xferType[] = { DMA_CAP_IO_TO_IO
                                 , DMA_CAP_IO_TO_MEMORY
                                 , DMA_CAP_IO_TO_DEVICE
                                 , DMA_CAP_MEMORY_TO_IO
                                 , DMA_CAP_MEMORY_TO_MEMORY
                                 , DMA_CAP_MEMORY_TO_DEVICE
                                 , DMA_CAP_DEVICE_TO_IO
                                 , DMA_CAP_DEVICE_TO_MEMORY
                                 , DMA_CAP_DEVICE_TO_DEVICE
                                 };
    uint32_t    type;

    /* Validate flags are not empty */
    if (!(src_flags & DMA_XFER_TYPE_FLAG) || !(src_flags & DMA_XFER_TYPE_FLAG)) {
        return 0;
    }

    src_flags = (src_flags & DMA_XFER_TYPE_FLAG ) >> 1;
    dst_flags = (dst_flags & DMA_XFER_TYPE_FLAG ) >> 1;

    type = dst_flags + (src_flags * 3);
    if (type >= (sizeof(xferType)/sizeof(xferType[0]))) {
        return 0;
    }

    return xferType[type];
}


/* Reset HW TCD table*/
static void
reset_tcd()
{
    int i, j;
    uintptr_t            edma3_base;

    for(j = 0; j < 2; j++)
    {
        for(i = 0; i < 32; i++)
        {
            edma3_base = fsl_edma3[j]->fsl_edma3_base;

            outde32(edma3_base + EDMA3_TCD_SADDR(i), 0);
            outde16(edma3_base + EDMA3_TCD_SOFF(i), 0);
            outde16(edma3_base + EDMA3_TCD_ATTR(i), 0);
            outde32(edma3_base + EDMA3_TCD_NBYTES(i), 0);
            outde32(edma3_base + EDMA3_TCD_SLAST(i), 0);
            outde32(edma3_base + EDMA3_TCD_DADDR(i), 0);
            outde16(edma3_base + EDMA3_TCD_DOFF(i), 0);
            outde16(edma3_base + EDMA3_TCD_CITER(i), 0);
            outde32(edma3_base + EDMA3_TCD_DLAST_SGA(i), 0);
            outde16(edma3_base + EDMA3_TCD_CSR(i), 0);
            outde16(edma3_base + EDMA3_TCD_BITER(i), 0);
        }
    }
}

static int
set_tcd_regs(dma_channel_t* chan)
{
    TcdChanCtrl_t*      pTcdCtl = &chan->tcdCtrl;
    tcdListItem_t*      pCurrent = pTcdCtl->pCurrent;
    fsl_edma3_tcd_t*     pTcd = pCurrent->pTcd;
    uintptr_t           edma3_base;
    int                  ch;

    /* Validate current channel and its state */
    if(chan == NULL){
        errno = EINVAL;
        return -1;
    }

    edma3_base = fsl_edma3[chan->edma3_id]->fsl_edma3_base;
    ch = chan->id - chan->edma3_id * CHANS_PER_EDMA3;

    if(pTcd != NULL){

        outde32(edma3_base + EDMA3_TCD_SADDR(ch), pTcd->saddr);
        outde16(edma3_base + EDMA3_TCD_SOFF(ch), pTcd->soff);
        outde16(edma3_base + EDMA3_TCD_ATTR(ch), pTcd->attr);
        outde32(edma3_base + EDMA3_TCD_NBYTES(ch), pTcd->nbytes);
        outde32(edma3_base + EDMA3_TCD_SLAST(ch), pTcd->slast);
        outde32(edma3_base + EDMA3_TCD_DADDR(ch), pTcd->daddr);
        outde16(edma3_base + EDMA3_TCD_DOFF(ch), pTcd->doff);
        outde16(edma3_base + EDMA3_TCD_CITER(ch), pTcd->citer);
        outde32(edma3_base + EDMA3_TCD_DLAST_SGA(ch), pTcd->dlast_sga);
        outde16(edma3_base + EDMA3_TCD_CSR(ch), pTcd->csr);
        outde16(edma3_base + EDMA3_TCD_BITER(ch), pTcd->biter);
    }
    return 0;
}


static int tx_iid[EDMA3_MAX_NUM][DMAMUX_TX_IRQ_MAX_NUM];
static int err_iid[EDMA3_MAX_NUM];
const struct sigevent * event_array[EDMA3_MAX_NUM][EDMA3_CHANNEL_NUM];


const struct sigevent *err_irq_handler(void *area, int id) {
    volatile uint32_t err_int, ch_es;
    uint32_t i;
    uintptr_t            edma3_base;
    int                  ch;

     for(i=0; i < EDMA3_MAX_NUM; i++) {
        edma3_base = fsl_edma3[i]->fsl_edma3_base;
        err_int = inde32(edma3_base + EDMA3_MP_ES);
        if (EDMA3_MP_ES_VLD(err_int)){
            /* find first active interrupt that belongs to this process */
            for(ch = 0; ch < CHANS_PER_EDMA3; ch++) {
                ch_es = inde32(edma3_base + EDMA3_CHn_ES(ch));
                if (ch_es & EDMA3_CHn_ES_ERR) {
                    /* clear irq status bit */
                    out32(edma3_base + EDMA3_CHn_ES(ch), EDMA3_CHn_ES_ERR);
                }
            }
        }
    }
    return NULL;
}

const struct sigevent *irq_handler(void *area, int id) {
    volatile uint32_t ch_int;
    uint32_t i;
    uintptr_t            edma3_base;
    int                  ch;

     for(i=0; i < EDMA3_MAX_NUM; i++) {
        edma3_base = fsl_edma3[i]->fsl_edma3_base;
        for(ch = 0; ch < CHANS_PER_EDMA3; ch++) {
           ch_int = inde32(edma3_base + EDMA3_CHn_INT(ch));
           if (ch_int & EDMA3_CHn_INT_INT) {
                /* clear irq status bit */
                out32(edma3_base + EDMA3_CHn_INT(ch), EDMA3_CHn_INT_INT);
                return event_array[i][ch];
           }
       }
    }
    return NULL;
}

int fsl_edma3_irq_init() {
    int i, j;

    if (-1 == ThreadCtl(_NTO_TCTL_IO_PRIV, 0)) {
        fsl_edma3_slogf("%s : ThreadCtl -%s", __FUNCTION__, strerror(errno));
        return -1;
    }

    for(i = 0; i < fsl_edma3_num; i++) {
        for(j = 0; j < fsl_edma3[i]->tx_irq_num; j++){

            tx_iid[i][j] = InterruptAttach(fsl_edma3[i]->tx_irq[j], irq_handler,NULL,0,_NTO_INTR_FLAGS_TRK_MSK);
            if (tx_iid[i][j] == -1) {
                return -1;
            }
        }

        err_iid[i] = InterruptAttach(fsl_edma3[i]->err_irq, err_irq_handler,NULL,0,_NTO_INTR_FLAGS_TRK_MSK);
        if (err_iid[i] == -1) {
            return -1;
        }

        for(j = 0;j < fsl_edma3_num;j++) {
            event_array[i][j] = NULL;
        }
    }
    return 0;
}


void fsl_edma3_irq_fini() {
    int i, j;

    for(i = 0; i < fsl_edma3_num; i++) {

        for(j = 0; j < fsl_edma3[i]->tx_irq_num; j++){
            if (tx_iid[i][j] != -1) {
                InterruptDetach(tx_iid[i][j]);
            }
        }

        if (err_iid[i] == -1) {
            InterruptDetach(err_iid[i]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//                                   DMA MUX functions                        //
////////////////////////////////////////////////////////////////////////////////

static int mux_free_chan_get(dma_channel_t* chan)
{
    rsrc_request_t *req = &chan->rsrc_req;
    /*get free channel from resource database manager */
    memset(req, 0, sizeof (rsrc_request_t));

    req->length = 1;
    req->flags = RSRCDBMGR_FLAG_RANGE | RSRCDBMGR_DMA_CHANNEL;

    req->start = (chan->edma3_id * CHANS_PER_EDMA3) + chan->mux * CHANS_PER_MUX;
    req->end   = req->start + CHANS_PER_MUX - 1;

    if (rsrcdbmgr_attach(req, 1) == -1){
        return -1;
    }

    return req->start;
}

static void mux_chan_set(unsigned int dma_id, unsigned int dma_chan, unsigned int mux, unsigned int slot, int enable)
{
    unsigned int mux_chan_reg;
    unsigned int mux_dma_chan;

    mux_dma_chan = (dma_chan % CHANS_PER_MUX) & 0xFF;

    /* Offset of CHCFG registers is arranged in big-endian order */
    if (soc_type == SOC_S32G) {
        mux_dma_chan ^= 0x3;
    }
    mux_chan_reg = fsl_edma3[dma_id]->fsl_dmamux_base[mux] + (mux_dma_chan);

    if (enable) {
        out8(mux_chan_reg, EDMAMUX_CHCFG_ENBL | EDMAMUX_CHCFG_SOURCE(slot));
    } else {
        out8(mux_chan_reg, EDMAMUX_CHCFG_DIS);
    }
}

static int init_board() {
    char machine_name[128];
    int rc;
    int i;

    machine_name[0] = '\0';
    rc = confstr(_CS_MACHINE, machine_name, sizeof(machine_name));
    if (0 == rc) {
        fsl_edma3_slogf("Failed to get machine name.");
    } else if (rc >= sizeof(machine_name)) {
        machine_name[sizeof(machine_name)-1] = '\0';
        fsl_edma3_slogf("using truncated machine name: %s", machine_name);
    }

    rc = 0;
    if (strstr(machine_name, "S32G")) {
        soc_type            = SOC_S32G;
	fsl_edma3_num        = 2;
    } else {
        fsl_edma3_slogf("Unknown board: %s", machine_name);
        rc = -1;
    }

    for (i = 0; i < fsl_edma3_num; i++){
        fsl_edma3[i] = (fsl_edma3_t *) calloc(1, sizeof(fsl_edma3_t));
    }

    switch(soc_type){
        case SOC_S32G:
            fsl_edma3[0]->fsl_edma3_phys      = S32G_EDMA0_BASE;
            fsl_edma3[0]->tx_irq_num          = 2;
            fsl_edma3[0]->tx_irq[0]           = S32G_EDMA0TX0_IRQ;
            fsl_edma3[0]->tx_irq[1]           = S32G_EDMA0TX1_IRQ;
            fsl_edma3[0]->err_irq             = S32G_EDMA0ERR_IRQ;
            fsl_edma3[0]->dmamux_slots_num    = S32G_DMAMUX_NUM_SLOTS;
            fsl_edma3[0]->dmamux_num          = 2;
            fsl_edma3[0]->dmamux_phys_base[0] = S32G_DMAMUX0_BASE;
            fsl_edma3[0]->dmamux_phys_base[1] = S32G_DMAMUX1_BASE;

            fsl_edma3[1]->fsl_edma3_phys       = S32G_EDMA1_BASE;
            fsl_edma3[1]->tx_irq_num          = 2;
            fsl_edma3[1]->tx_irq[0]           = S32G_EDMA1TX0_IRQ;
            fsl_edma3[1]->tx_irq[1]           = S32G_EDMA1TX1_IRQ;
            fsl_edma3[1]->err_irq             = S32G_EDMA1ERR_IRQ;
            fsl_edma3[1]->dmamux_slots_num    = S32G_DMAMUX_NUM_SLOTS;
            fsl_edma3[1]->dmamux_num          = 2;
            fsl_edma3[1]->dmamux_phys_base[0] = S32G_DMAMUX2_BASE;
            fsl_edma3[1]->dmamux_phys_base[1] = S32G_DMAMUX3_BASE;

            slots_num = fsl_edma3[0]->dmamux_num * fsl_edma3[0]->dmamux_slots_num +
                        fsl_edma3[1]->dmamux_num * fsl_edma3[1]->dmamux_slots_num;
            break;

        default:
            break;
    }

    return rc;
}

////////////////////////////////////////////////////////////////////////////////
//                                   API                                      //
////////////////////////////////////////////////////////////////////////////////

int
dma_init(const char* options)
{
    int         i, j;

    pthread_mutex_lock(&processinit_mutex);
    n_users_in_process++;

    // Only need to initialize once per process
    if (n_users_in_process == 1) {
        // init slogging
        if (fsl_slog2_init() != EOK)
            goto fail1;

        // init multi-process saftey
        if ( fsl_edma3_sync_init() != 0) {
            fsl_edma3_slogf("%s: could not init shared memory sync", __func__);
            goto fail1;
        }

        if ( init_board() != 0) {
            fsl_edma3_slogf("%s: init_board failed", __func__);
            goto fail2;
        }

        /* Map physical register for DMA */
        for (i = 0; i < fsl_edma3_num; i++){
            fsl_edma3[i]->fsl_edma3_base = mmap_device_io(EDMA3_SIZE,fsl_edma3[i]->fsl_edma3_phys);
            if (fsl_edma3[i]->fsl_edma3_base == (uintptr_t)MAP_FAILED) {
                fsl_edma3_slogf("%s: could not mmap EDMA device IO for physical memory", __func__);
                goto fail2;
            }

            for (j = 0; j < fsl_edma3[i]->dmamux_num; j++) {
                fsl_edma3[i]->fsl_dmamux_base[j] = mmap_device_io(DMAMUX_SIZE, fsl_edma3[i]->dmamux_phys_base[j]);
                if (fsl_edma3[i]->fsl_dmamux_base[j] == (uintptr_t)MAP_FAILED) {
                    fsl_edma3_slogf("%s: could not mmap DMAMUX device IO for physical memory", __func__);
                    goto fail3;
                }
            }
        }

        /*init tx interrupt and err interrupt*/
        if ( fsl_edma3_irq_init() != EOK ){
            fsl_edma3_slogf("dma_init: could init IRQ");
            goto fail4;
        }
        /* allow only 1 process to init the dma shared components at a time*/
        pthread_mutex_lock( fsl_edma3_sync_libinit_mutex_get() );
        fsl_edma3_sync_process_cnt_incr();

        if ( fsl_edma3_sync_is_first_process() ) {
            /* Reset HW TCD table */
            reset_tcd();
        }
        pthread_mutex_unlock( fsl_edma3_sync_libinit_mutex_get() ); // done share init

    }

    pthread_mutex_unlock(&processinit_mutex);
    return EOK;


// Cleanup on error
fail4:
    fsl_edma3_sync_process_cnt_decr();
    pthread_mutex_unlock( fsl_edma3_sync_libinit_mutex_get() );
    fsl_edma3_irq_fini();
fail3:
    for (i = 0; i < fsl_edma3_num; i++){
        for (j = 0; j < fsl_edma3[i]->dmamux_num; j++) {
            munmap_device_io(fsl_edma3[i]->fsl_dmamux_base[j],DMAMUX_SIZE);
        }
    }
fail2:
    for (i = 0; i < fsl_edma3_num; i++){
        munmap_device_io(fsl_edma3[i]->fsl_edma3_base,EDMA3_SIZE);
    }
    fsl_edma3_sync_fini();
fail1:
    n_users_in_process--;
    pthread_mutex_unlock(&processinit_mutex);
    return -1;
}


/*
 * Should be called when library is no longer needed, e.g. when the
 * driver is about to be unloaded.
 */

void dma_fini() {

    int i, j;

    pthread_mutex_lock(&processinit_mutex);
    n_users_in_process--;

    // if we are the last lib user in this process, then cleanup
    if (n_users_in_process == 0) {

        pthread_mutex_lock( fsl_edma3_sync_libinit_mutex_get() );
        fsl_edma3_sync_process_cnt_decr();
        pthread_mutex_unlock( fsl_edma3_sync_libinit_mutex_get() );

        fsl_edma3_irq_fini();
        for (i = 0; i < fsl_edma3_num; i++){
            munmap_device_io(fsl_edma3[i]->fsl_edma3_base,EDMA3_SIZE);

            for (j = 0; j < fsl_edma3[i]->dmamux_num; j++) {
                munmap_device_io(fsl_edma3[i]->fsl_dmamux_base[j],DMAMUX_SIZE);
            }
        }
        fsl_edma3_sync_fini();
    }
    pthread_mutex_unlock(&processinit_mutex);
}


int
dma_driver_info(dma_driver_info_t *info)
{
    if (info == NULL) {
        return -1;
    }

    info->dma_version_major = DMALIB_VERSION_MAJOR;
    info->dma_version_minor = DMALIB_VERSION_MINOR;
    info->dma_rev           = DMALIB_REVISION;
    info->lib_rev           = 0;
    info->description       = FSL_EDMA3_DESCRIPTION_STR;
    info->num_channels      = EDMA3_CHANNEL_NUM;
    info->max_priority      = EDMA3_MAX_PRIORITY;

    return EOK;
}


int
dma_channel_info(unsigned channel, dma_channel_info_t *info)
{
    if (info == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Fill generic information about the a DMA Channel */

    info->max_xfer_size         = MAX_FRAG_LEN;
    info->xfer_unit_sizes       = DMA_XFER_UNIT_SIZE;
    info->mem_upper_limit       = DMA_UPPER_MEM_LIMIT;
    info->mem_lower_limit       = DMA_LOWER_MEM_LIMIT;
    info->mem_nocross_boundary  = -1;

    /* Define channel capabilities
     * Need to provide per CH information and also see if we can use
     * the devive to device xfer
     */
    info->max_src_fragments = DMA_MAX_NUM_TCD;
    info->max_dst_fragments = DMA_MAX_NUM_TCD;
    info->max_src_segments = DMA_MAX_NUM_TCD;
    info->max_dst_segments = DMA_MAX_NUM_TCD;

    info->caps =    DMA_CAP_EVENT_ON_COMPLETE |
                    DMA_CAP_EVENT_PER_SEGMENT |
                    DMA_CAP_DEVICE_TO_MEMORY |
                    DMA_CAP_MEMORY_TO_DEVICE |
                    DMA_CAP_MEMORY_TO_MEMORY;

    return EOK;
}


static void
free_TCD(dma_channel_t* chan)
{
    TcdChanCtrl_t* pTcdCtl;

    if (chan == NULL)
        return;

    pTcdCtl = &chan->tcdCtrl;

    if (pTcdCtl->tcdList != NULL) {
        munmap( (void*)pTcdCtl->tcdList
              , pTcdCtl->numTCD  * sizeof(tcdListItem_t));
    }

    if (pTcdCtl->tcdArray != NULL) {
        munmap( (void*)pTcdCtl->tcdArray
              , (pTcdCtl->numTCD + 1)  * sizeof(fsl_edma3_tcd_t));
    }
}


static int
alloc_TCD(dma_channel_t* chan)
{
    int                     idx;
    TcdChanCtrl_t*          pTcdCtl = &chan->tcdCtrl;
    tcdListItem_t*          pTcdInfo;
    fsl_edma3_tcd_t*        pTcd;

    /* Initialize pointers used to keep track of allocated memory */
    pTcdCtl->tcdList  = NULL;
    pTcdCtl->tcdArray = NULL;

    /* Initialize channel configuration variables */
    pTcdCtl->currBDInUse    = 0;
    pTcdCtl->numTCD         = DMA_MAX_NUM_TCD;

    /* Allocate the number of TCD list item requested */
    if ((pTcdCtl->tcdList = (tcdListItem_t*)mmap( 0, pTcdCtl->numTCD  * sizeof(tcdListItem_t)
                    , PROT_READ|PROT_WRITE, MAP_SHARED, NOFD, 0)) == MAP_FAILED) {
        fsl_edma3_slogf("%s: Fail to allocate memory for TCD list", __func__);
        return -1; /* Fail to allocate memory for TCD */
    }

    /* Allocate the number of TCD requested */
    if ((pTcdCtl->tcdArray = mmap( 0, (pTcdCtl->numTCD + 1) * sizeof(fsl_edma3_tcd_t)
                   , PROT_READ|PROT_WRITE|PROT_NOCACHE, MAP_SHARED, NOFD, 0)) == MAP_FAILED) {
        fsl_edma3_slogf("%s: Fail to allocate memory for TCD", __func__);
        return -1; /* Fail to allocate memory for TCD */
    }

    /* Reset data */
    memset(pTcdCtl->tcdArray, 0, (pTcdCtl->numTCD + 1) * sizeof(fsl_edma3_tcd_t));
    memset(pTcdCtl->tcdList,  0, (pTcdCtl->numTCD) * sizeof(tcdListItem_t));

    /* Get physical address of TCD table */
    if(mem_offset64(pTcdCtl->tcdArray, NOFD, 1, &pTcdCtl->tcdList[0].phyAddr, 0) == -1) {
        return -1;
    }

    /* Align TCD to be 32 bytes aligned */
    pTcd = pTcdCtl->tcdList[0].pTcd = (fsl_edma3_tcd_t*)(((uintptr_t)pTcdCtl->tcdArray + sizeof(fsl_edma3_tcd_t)) & ~(sizeof(fsl_edma3_tcd_t) - 1 ));

    pTcdCtl->tcdList[0].phyAddr += (uintptr_t)pTcdCtl->tcdList[0].pTcd - (uintptr_t)pTcdCtl->tcdArray;

    /* Initialize task information */
    pTcdCtl->pCurrent = pTcdCtl->pNextFree = pTcdCtl->tcdList;

    /* Initialize TCD linked-list */
    pTcdInfo = pTcdCtl->tcdList;
    for (idx=0; idx< pTcdCtl->numTCD; idx++) {
        pTcdInfo->pNext = pTcdInfo + 1;
        pTcdInfo->pNext->pPrev = pTcdInfo;
        pTcdInfo->pTcd    = pTcd++;
        pTcdInfo->phyAddr = pTcdCtl->tcdList[0].phyAddr + (idx * sizeof(fsl_edma3_tcd_t));
        pTcdInfo++;
    }

    /* Link first and last element of the list */
    pTcdInfo--;
    pTcdCtl->tcdList->pPrev = pTcdInfo;
    pTcdInfo->pNext         = pTcdCtl->tcdList;

    return 0;
}

void*
dma_channel_attach( const char* options, const struct sigevent* event, unsigned* muxes_slot, int priority, dma_attach_flags flags)
{
    dma_channel_t*            chan = NULL;
    char*                     optstr = NULL;

    int                       reqResult = -1;
    int                       ch;
    int                       dmamux_slots_num;

    /* Is the slot within the total slots on all the muxes? */
    if (*muxes_slot >= slots_num) {
        fsl_edma3_slogf("%s: Invalid slot number %d. Total available slots: %d",
                __func__, *muxes_slot, slots_num);
        errno = ECHRNG;
        return NULL;
    }

    if (options) {
        optstr = strdup(options);
        if (optstr == NULL)
            return NULL;
    }

    /* Allocated memory for channel */
    chan = (dma_channel_t*)calloc(1, sizeof (dma_channel_t));
    if (chan == NULL)
        goto fail;

    /* Allocate TCD memory */
    if (alloc_TCD(chan) != -0)
        goto fail;


    if ((errno = dma_parse_options(chan, optstr)) != EOK) {
        fprintf(stderr, "%s: parse options failed\n", __FUNCTION__);
        goto fail;
    }

    /* Initialize channel control variables */
    dmamux_slots_num = fsl_edma3[chan->edma3_id]->dmamux_slots_num;

    chan->mux      = *muxes_slot / dmamux_slots_num;
    chan->mux_slot = *muxes_slot % dmamux_slots_num;
    chan->id      = mux_free_chan_get(chan);
    chan->flags   = flags;
    chan->busy    = 0;
    chan->running = 0;

    ch = chan->id - chan->edma3_id * CHANS_PER_EDMA3;

    fsl_edma3_slogf("%s: muxes_slot %d edma_id %d mux %d mux_slot %d chan %d",
            __func__, *muxes_slot, chan->edma3_id, chan->mux, chan->mux_slot, chan->id);

    if (chan->id < 0) {
        fsl_edma3_slogf("%s: Unable to assign channel or no free channel, slot: %d",
                __func__, *muxes_slot);
        goto fail;
    }

    if (flags & (DMA_ATTACH_EVENT_ON_COMPLETE | DMA_ATTACH_EVENT_PER_SEGMENT)) {
        event_array[chan->edma3_id][ch] = event;
    }

    return chan;

fail:

    /* Release TCD data used by channel */
    if (chan != NULL) {
        /* Detach from ressource manager */
        if ( reqResult != -1) {
            rsrcdbmgr_detach(&chan->rsrc_req, 1);
        }
        free_TCD(chan);
        free(chan);
    }

    if (optstr != NULL)
        free(optstr);

    return NULL;
}


void
dma_channel_release(void* handle)
{
    dma_channel_t*      chan = handle;
    int                 ch;

    ch = chan->id - chan->edma3_id * CHANS_PER_EDMA3;

    halt_channel(chan);
    event_array[chan->edma3_id][ch] = NULL;
    chan->mode_flags      = 0;

    /* Release channel */
    rsrcdbmgr_detach(&(chan->rsrc_req), 1);
    mux_chan_set(chan->edma3_id, ch, chan->mux, chan->mux_slot, 0);

    free_TCD(chan);
    free(handle);
}


int
dma_alloc_buffer(void* handle, dma_addr_t* addr, unsigned size, unsigned flags)
{
    rsrc_request_t  req;

    memset(&req, 0, sizeof (req));
    req.length = size;
    req.align  = __PAGESIZE;
    req.flags  = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_TOPDOWN | RSRCDBMGR_FLAG_ALIGN;
    req.name   = EDMA3_MEMORY_NAME;
    if (rsrcdbmgr_attach(&req, 1) == -1)
    return -1;

    addr->vaddr = mmap_device_memory( 0, size, PROT_READ|PROT_WRITE|PROT_NOCACHE
                                    , 0, req.start);

    if (addr->vaddr == MAP_FAILED) {
        rsrcdbmgr_detach(&req, 1);
    return -1;
    }

    addr->paddr = req.start;
    addr->len = size;

    return 0;
}


void
dma_free_buffer( void* handle, dma_addr_t* addr)
{
    rsrc_request_t  req;

    munmap_device_memory(addr->vaddr, addr->len);

    memset(&req, 0, sizeof (req));
    req.start  = addr->paddr;
    req.end    = addr->paddr + addr->len - 1;
    req.length = addr->len;
    req.flags  = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_TOPDOWN;
    req.name   = EDMA3_MEMORY_NAME;
    rsrcdbmgr_detach(&req, 1);
}

uint16_t    dmaSizeTable[] = { -1 /* 0 Byte Not supported */
                             , EDMA3_TCD_SIZE_8Bit
                             , EDMA3_TCD_SIZE_16Bit
                             , -1 /* 3 Bytes not supported */
                             , EDMA3_TCD_SIZE_32Bit
                             , -1 /* 5 Bytes not supported */
                             , -1 /* 6 Bytes not supported */
                             , -1 /* 7 Bytes not supported */
                             , EDMA3_TCD_SIZE_64Bit
                             , -1 /* 9 Bytes not supported */
                             , -1 /* 10 Bytes not supported */
                             , -1 /* 11 Bytes not supported */
                             , -1 /* 12 Bytes not supported */
                             , -1 /* 13 Bytes not supported */
                             , -1 /* 14 Bytes not supported */
                             , -1 /* 15 Bytes not supported */
                             , -1 /* 16 Bytes not supported */
                             , -1 /* 17 Bytes not supported */
                             , -1 /* 18 Bytes not supported */
                             , -1 /* 19 Bytes not supported */
                             , -1 /* 20 Bytes not supported */
                             , -1 /* 21 Bytes not supported */
                             , -1 /* 22 Bytes not supported */
                             , -1 /* 23 Bytes not supported */
                             , -1 /* 24 Bytes not supported */
                             , -1 /* 25 Bytes not supported */
                             , -1 /* 26 Bytes not supported */
                             , -1 /* 27 Bytes not supported */
                             , -1 /* 28 Bytes not supported */
                             , -1 /* 29 Bytes not supported */
                             , -1 /* 30 Bytes not supported */
                             , -1 /* 31 Bytes not supported */
                             , EDMA3_TCD_SIZE_32Byte
                             };
#define NUM_ITEMS(array)     (sizeof(array)/sizeof(array[0]))

int
dma_setup_xfer( void* handle, const dma_transfer_t* tinfo)
{
    dma_channel_t*          chan = handle;
    dma_channel_caps        xfer_type;
    uint16_t                nbytes;
    uint32_t                seg_size;
    int                     src_idx = 0;
    int                     dst_idx = 0;
    int                     segments;
    int                     first_segment = 1;
    int                     is_segment    = 0;
    fsl_edma3_tcd_t*        pHwTcd;
    fsl_edma3_tcd_t*        ptcd = NULL;
    tcdListItem_t*          ptcd_first;
    fsl_edma3_tcd_t*        ptcd_prev;
    TcdChanCtrl_t*          pTcdCtl = &chan->tcdCtrl;
    uintptr_t               edma3_base;
    int                     ch;

    fsl_edma3_slogf("%s: ch_id = %d", __func__, chan->id);


    if (chan == NULL || tinfo == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Do we have a valid channel? */
    if (chan->id >= EDMA3_CHANNEL_NUM) {
        errno = ECHRNG;
        return -1;
    }

    edma3_base = fsl_edma3[chan->edma3_id]->fsl_edma3_base;
    ch = chan->id - chan->edma3_id * CHANS_PER_EDMA3;

    /* Can't configure xfer if channel is currently running */
    if (chan->running) {
        errno = EBUSY;
        return -1;
    }

    xfer_type = get_xfer_type(tinfo->src_flags, tinfo->dst_flags);

    /* Validate xfer unit size is supported */
    if (  ( tinfo->xfer_unit_size >= NUM_ITEMS(dmaSizeTable))
       || ( (nbytes = dmaSizeTable[tinfo->xfer_unit_size] ) == (uint16_t)-1)) {
        errno = EINVAL;
        return -1;
    }

    /* Validate if transfer size if compatible with bytes transfer */
    if (tinfo->xfer_bytes % tinfo->xfer_unit_size) {
        errno = EINVAL;
        return -1;
    }

    segments = (tinfo->src_fragments > tinfo->dst_fragments)?tinfo->src_fragments:tinfo->dst_fragments;

    if(segments > DMA_MAX_NUM_TCD) {
        errno = EINVAL;
        return -1;
    }

    if (segments > (pTcdCtl->numTCD - pTcdCtl->currBDInUse)) {
        /*
         * The buffer ring is full.
         */
         errno = ENOMEM;
         return  -1;
    }

    if ((tinfo->xfer_bytes % segments)) {
        errno = EINVAL;
        return -1;
    }

    /* Determine segment addr */
    seg_size = tinfo->xfer_bytes / segments;

    chan->mode_flags = tinfo->mode_flags;

    if (segments > 1)
        is_segment = 1;

    /* Get pointer to active task component */
    pHwTcd     = &pTcdCtl->tcdArray[chan->id];
    ptcd_first = pTcdCtl->pNextFree;

    /* Set each segment required */
    while (segments--)
    {
        /* Increase Buffer Descriptor in-use variable. */
        ptcd_prev = pTcdCtl->pNextFree->pPrev->pTcd;
        ptcd      = pTcdCtl->pNextFree->pTcd;
        pTcdCtl->pNextFree = pTcdCtl->pNextFree->pNext;
        pTcdCtl->currBDInUse++;

        /* Set source and destination address */
        if (tinfo->src_flags & DMA_ADDR_FLAG_SEGMENTED)
            ptcd->saddr = tinfo->src_addrs[0].paddr + (seg_size * src_idx);
        else
            ptcd->saddr = tinfo->src_addrs[src_idx].paddr;

        if (src_idx < (tinfo->src_fragments-1))
            src_idx++;

        if (tinfo->dst_flags & DMA_ADDR_FLAG_SEGMENTED)
            ptcd->daddr = tinfo->dst_addrs[0].paddr + (seg_size * dst_idx);
        else
            ptcd->daddr = tinfo->dst_addrs[dst_idx].paddr;

        if (dst_idx < (tinfo->dst_fragments-1))
            dst_idx++;

        /* Set Soure and address transfer size and modulo operation */
        ptcd->attr = EDMA3_TCD_ATTR_SMOD(0)
                    | EDMA3_TCD_ATTR_SSIZE(nbytes)
                    | EDMA3_TCD_ATTR_DMOD(0)
                    | EDMA3_TCD_ATTR_DSIZE(nbytes);

        /* Set the way source and destination will increment */
        switch (tinfo->src_flags & (DMA_ADDR_FLAG_DECREMENT | DMA_ADDR_FLAG_NO_INCREMENT)) {

            case DMA_ADDR_FLAG_NO_INCREMENT:
                ptcd->soff  = 0;
                ptcd->slast = 0;
                break;
            case DMA_ADDR_FLAG_DECREMENT:
                ptcd->soff  = -(uint16_t)tinfo->xfer_unit_size;
                ptcd->slast = seg_size;
                break;
            default : /* Increment */
                ptcd->soff  = (uint16_t)tinfo->xfer_unit_size;
                ptcd->slast = -seg_size;
                break;
        }

        switch (tinfo->dst_flags & (DMA_ADDR_FLAG_DECREMENT | DMA_ADDR_FLAG_NO_INCREMENT)) {

            case DMA_ADDR_FLAG_NO_INCREMENT:
                ptcd->doff      = 0;
                ptcd->dlast_sga = 0;
                break;
            case DMA_ADDR_FLAG_DECREMENT:
                ptcd->doff = -(uint16_t)tinfo->xfer_unit_size;
                ptcd->dlast_sga = seg_size;
                break;
            default : /* Increment */
                ptcd->doff      = (uint16_t)tinfo->xfer_unit_size;
                ptcd->dlast_sga = -seg_size;
                break;
        }

        if (xfer_type == DMA_CAP_MEMORY_TO_MEMORY) {
            ptcd->nbytes = seg_size;
            ptcd->citer  = ptcd->biter = 1;
         } else {
            /* Compute number of iteration according to buffer size */
            ptcd->nbytes = tinfo->xfer_unit_size;
            ptcd->citer  = ptcd->biter = seg_size / ptcd->nbytes;
        }

        /* Enable interrupt if requested for each segment or fragment */
        if (chan->flags & DMA_ATTACH_EVENT_PER_SEGMENT)
            ptcd->csr |= EDMA3_TCD_CSR_INT_MAJOR;

        /* If segmented of fragmented, link with previous CH */
        if (!(first_segment && is_segment)) {
            ptcd_prev->dlast_sga  = pTcdCtl->pNextFree->pPrev->phyAddr;
            ptcd_prev->csr     |= EDMA3_TCD_CSR_E_SG;
        }
        else {
           first_segment = 0;
        }
    }

    /* Enable interrupt if requested for last segment */
    if (chan->flags & DMA_ATTACH_EVENT_ON_COMPLETE)
        ptcd->csr |= EDMA3_TCD_CSR_INT_MAJOR;

    /* Link with first channel if continous ring is setup */
    if (tinfo->mode_flags & DMA_MODE_FLAG_REPEAT) {
        ptcd->dlast_sga  = ptcd_first->phyAddr;
        ptcd->csr     |= EDMA3_TCD_CSR_E_SG;
    } else if (tinfo->src_flags & DMA_ADDR_FLAG_DEVICE ||
               tinfo->dst_flags & DMA_ADDR_FLAG_DEVICE) {
         /* Disable DMA request when major loop is done */
         ptcd->csr |= EDMA3_TCD_CSR_D_REQ;
    }

    /*
     * Attach to proper TCD depending if DMA is running or not!.
     */
    if (chan->running) {        /* DMA should be up and running! */

        /* Set last source and destination address after transfer */
        pHwTcd->dlast_sga  = (uint32_t)pTcdCtl->pNextFree->pPrev->phyAddr;

        /* Indicate we do scatter gather but clear done bit first */
	if(in32(edma3_base + EDMA3_CHn_CSR(ch)) & 0x40000000)
	    out32(edma3_base + EDMA3_CHn_CSR(ch), EDMA3_CHn_CSR_DONE);

        pHwTcd->csr |= 0x12;

        if ((pHwTcd->csr & 0x10) == 0) {
            /* Stop current task */
            halt_channel(chan);

            /* Copy current TCD image into DMA controller */
            memcpy( pHwTcd, pTcdCtl->pCurrent->pTcd, sizeof(fsl_edma3_tcd_t));

            /* Enable DMA request */
            out32(edma3_base + EDMA3_CHn_CSR(ch), EDMA3_CHn_CSR_ERQ | EDMA3_CHn_CSR_EEI);
        }
    } else {
        chan->curXferSize = tinfo->xfer_bytes;
        pTcdCtl->pCurrent = ptcd_first;

        /* Indicate the type of tranfer we are doing so we know which bit to set to trigger xfer */
         chan->ctrl = xfer_type;
    }

    return 0;
}


int
dma_xfer_start(void* handle)
{
    dma_channel_t*       chan = handle;
    uintptr_t            edma3_base;
    int                  ch;

    /* Validate current channel and its state */
    if(chan == NULL){
        errno = EINVAL;
        return -1;
    }

    edma3_base = fsl_edma3[chan->edma3_id]->fsl_edma3_base;
    ch = chan->id - chan->edma3_id * CHANS_PER_EDMA3;

    /* Start the transfer */
    pthread_mutex_lock( fsl_edma3_sync_libinit_mutex_get() );

    if (chan->running != 0) {
        errno = EAGAIN;
        return -1;
    }

    /* Clear DMA request */
    out32(edma3_base + EDMA3_CHn_CSR(ch), 0);

    /* Copy current TCD image into DMA controller */
    set_tcd_regs(chan);

    /* Select the source in the dmamux */
    mux_chan_set(chan->edma3_id, ch, chan->mux, chan->mux_slot, 1);

    /* Trigger the xfer */
    out32(edma3_base + EDMA3_CHn_CSR(ch), EDMA3_CHn_CSR_ERQ | EDMA3_CHn_CSR_EEI);

    outde16(edma3_base + EDMA3_TCD_CSR(ch), 0x1 | inde16(edma3_base + EDMA3_TCD_CSR(ch)));

    chan->running = 1;

    pthread_mutex_unlock( fsl_edma3_sync_libinit_mutex_get() );
    return 0;
}

int
dma_xfer_abort( void* handle)
{
    dma_channel_t       *chan = handle;

    return halt_channel(chan);
}


int
dma_xfer_complete( void* handle)
{
    dma_channel_t       *chan = handle;
    uintptr_t            edma3_base;
    int                  ch;

    /* Validate current channel and its state */
    if(chan == NULL){
        errno = EINVAL;
        return -1;
    }

    edma3_base = fsl_edma3[chan->edma3_id]->fsl_edma3_base;
    ch = chan->id - chan->edma3_id * CHANS_PER_EDMA3;

    if (!(chan->mode_flags & DMA_MODE_FLAG_REPEAT)) {
        /* Not in repeat mode, disable the channel */
        halt_channel(chan);
    } else {
        /* Just clear the interrupt sources */
        out32(edma3_base + EDMA3_CHn_INT(ch), EDMA3_CHn_INT_INT);
    }

    return 0;
}

/**
 * Gets actual number of transfered bytes.
 *
 * There are 2 scenarios supported.
 * First scenario is when a channel completed a major loop and DONE bit is set.
 * For single-segment buffer secnario, we can determine the completetion by checking the DONE bit from TCDn_CSR resiger.
 * For multi-segment buffer, we cannot read destination address from TCD register directly because TCD is automatically
 * reloaded once major loop is done. So, in this case, we can't determine if the last segmented is just started or already
 * finished from the reading of TCD regisers. we are still under the process to discuss with NXP to figure out a better solution.
 * So, for now, we only return 0 for this case.
 *
 * Second scenario is when a channel is idle or completed a minor loop so DONE bit is not set. In that case we read
 * source or destination address directly from TCD register and calculate transferred bytes.
 *
 * Total amount of transfered bytes is not calculated correctly when DMA_MODE_FLAG_REPEAT flag is set for a channel.
 * However, the returned bytes can still be used by client application to determine the current position of write pointer.
 *
 * @param handle Channel handle.
 *
 * @return Number bytes remain to be transfered.
 */
#define DMA_XFER_TYPE_DEVICE   (DMA_CAP_IO_TO_DEVICE | DMA_CAP_MEMORY_TO_DEVICE |\
                                DMA_CAP_DEVICE_TO_IO | DMA_CAP_DEVICE_TO_MEMORY |\
                                DMA_CAP_DEVICE_TO_DEVICE)
unsigned
dma_bytes_left(void* handle)
{
    dma_channel_t*    chan = (dma_channel_t*)handle;
    TcdChanCtrl_t*      pTcdCtl = &chan->tcdCtrl;
    tcdListItem_t*      pCurrent = pTcdCtl->pCurrent;
    fsl_edma3_tcd_t*     pTcd = pCurrent->pTcd;
    uint32_t pos, start, i, len;
    unsigned bytes = 0;
    uintptr_t            edma3_base;
    int                  ch;

    /* Validate current channel and its state */
    if(chan == NULL){
        errno = EINVAL;
        return -1;
    }

    edma3_base = fsl_edma3[chan->edma3_id]->fsl_edma3_base;
    ch = chan->id - chan->edma3_id * CHANS_PER_EDMA3;


    if (!chan->running) {
          return 0;
    }

    if (!(chan->mode_flags & DMA_MODE_FLAG_REPEAT) &&
        !(chan->ctrl & DMA_XFER_TYPE_DEVICE) && pTcdCtl->currBDInUse > 1) {
        /* For multi-segment buffer, there is no good way to tell if the current segment is being sent
         * or not when DONE bit is set by the DMA engine. This will be reveiwed again after we find
         * out a solution. For now, only return 0.
        */
        return 0;
    } else if ((pTcdCtl->currBDInUse == 1)) {
        /* For single-segment buffer and major loop is done */
        bytes = pTcd->biter * pTcd->nbytes;
    } else {
        /* For multi-segment buffer in repeated mode or channel is executing, we
         * read TCD register directly
         */
        for (i = 0; i < pTcdCtl->currBDInUse; i++) {
            len = pTcd->biter * pTcd->nbytes;
            if (pTcd->doff != 0) {
                pos = inde32(edma3_base + EDMA3_TCD_DADDR(ch));
                start = pTcd->daddr;
            } else if (pTcd->soff != 0) {
                pos = inde32(edma3_base + EDMA3_TCD_SADDR(ch));
                start = pTcd->saddr;
            } else {
                continue;
            }

            if ((start <= pos) && ((start + len) > pos)) {
                bytes += (pos - start);
                /* If DREQ bit is enabled from last tcd, ERQ is clear and current position is at start address,
                 * we can safely assume last segment is completed.
                 */
                if ( (pTcd->csr & EDMA3_TCD_CSR_D_REQ) && (start == pos) ) {
                    bytes += len;
                }
                break;
            }
            bytes += len;
            pCurrent = pCurrent->pNext;
            pTcd = pCurrent->pTcd;
        }
    }
    return chan->curXferSize - bytes;
}

////////////////////////////////////////////////////////////////////////////////

int
get_dmafuncs(dma_functions_t *functable, int tabsize)
{
    DMA_ADD_FUNC(functable, init,            dma_init,            tabsize);
    DMA_ADD_FUNC(functable, fini,            dma_fini,            tabsize);
    DMA_ADD_FUNC(functable, driver_info,     dma_driver_info,     tabsize);
    DMA_ADD_FUNC(functable, channel_info,    dma_channel_info,    tabsize);
    DMA_ADD_FUNC(functable, channel_attach,  dma_channel_attach,  tabsize);
    DMA_ADD_FUNC(functable, channel_release, dma_channel_release, tabsize);
    DMA_ADD_FUNC(functable, setup_xfer,      dma_setup_xfer,      tabsize);
    DMA_ADD_FUNC(functable, xfer_start,      dma_xfer_start,      tabsize);
    DMA_ADD_FUNC(functable, xfer_abort,      dma_xfer_abort,      tabsize);
    DMA_ADD_FUNC(functable, bytes_left,      dma_bytes_left,      tabsize);
    DMA_ADD_FUNC(functable, xfer_complete,   dma_xfer_complete,   tabsize);
    DMA_ADD_FUNC(functable, alloc_buffer,    dma_alloc_buffer,    tabsize);
    DMA_ADD_FUNC(functable, free_buffer,     dma_free_buffer,     tabsize);
    return 0;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/lib/dma/fsl-edma3/fsl_edma3.c $ $Rev: 862698 $")
#endif
