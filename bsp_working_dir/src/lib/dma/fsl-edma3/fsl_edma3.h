/*
 * $QNXLicenseC:
 * Copyright 2018 QNX Software Systems.
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


#ifndef FSL_EDMA3_H
#define FSL_EDMA3_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/siginfo.h>
#include <sys/mman.h>
#include <string.h>
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <errno.h>
#include <atomic.h>
#include <fcntl.h>
#include <sys/rsrcdbmgr.h>
#include <sys/rsrcdbmsg.h>
#include <hw/dma.h>
#include <sys/rsrcdbmgr.h>
#include <sys/hwinfo.h>
#include <drvr/hwinfo.h>
#include <sys/cache.h>
#include <aarch64/s32g.h>

#if defined(VARIANT_dev_be)
#define ENDIAN_ADDR8(__addr)    ((__addr) ^ 0x3)
#define ENDIAN_ADDR16(__addr)   ((__addr) ^ 0x2)
#else
#define ENDIAN_ADDR8(__addr)    (__addr)
#define ENDIAN_ADDR16(__addr)   (__addr)
#endif

/* EDMA3 Registers */
#define EDMA3_MP_CSR			0x00
#define EDMA3_MP_CSR_ERCA		(1 << 2)

#define EDMA3_MP_ES			0x04
#define EDMA3_MP_ES_VLD(x)		((x) & 0x80000000)

#define EDMA3_CHn_CSR(ch)		(0x4000 + (ch) * 0x1000)
#define EDMA3_CHn_CSR_ERQ		(1 << 0)
#define EDMA3_CHn_CSR_EEI		(1 << 2)
#define EDMA3_CHn_CSR_DONE		(1 << 30)

#define EDMA3_CHn_ES(ch)		(0x4004 + (ch) * 0x1000)
#define EDMA3_CHn_ES_ERR		(1 << 31)

#define EDMA3_CHn_INT(ch)		(0x4008 + (ch) * 0x1000)
#define EDMA3_CHn_INT_INT		(1 << 0)

#define EDMA3_TCD(ch)			(0x4020 + 0x1000 * (ch))


/* EDMA3 TCD fields. */
#define EDMA3_TCD_SADDR(ch)              (EDMA3_TCD(ch) + 0x00)
#define EDMA3_TCD_SOFF(ch)               (EDMA3_TCD(ch) + ENDIAN_ADDR16(0x04))
#define EDMA3_TCD_ATTR(ch)               (EDMA3_TCD(ch) + ENDIAN_ADDR16(0x06))
#define EDMA3_TCD_NBYTES(ch)             (EDMA3_TCD(ch) + 0x08)
#define EDMA3_TCD_SLAST(ch)              (EDMA3_TCD(ch) + 0x0C)
#define EDMA3_TCD_DADDR(ch)              (EDMA3_TCD(ch) + 0x10)
#define EDMA3_TCD_DOFF(ch)               (EDMA3_TCD(ch) + ENDIAN_ADDR16(0x14))
#define EDMA3_TCD_CITER_ELINK(ch)        (EDMA3_TCD(ch) + ENDIAN_ADDR16(0x16))
#define EDMA3_TCD_CITER(ch)              EDMA3_TCD_CITER_ELINK(ch)
#define EDMA3_TCD_DLAST_SGA(ch)          (EDMA3_TCD(ch) + 0x18)
#define EDMA3_TCD_CSR(ch)                (EDMA3_TCD(ch) + ENDIAN_ADDR16(0x1C))
#define EDMA3_TCD_BITER_ELINK(ch)        (EDMA3_TCD(ch) + ENDIAN_ADDR16(0x1E))
#define EDMA3_TCD_BITER(ch)              EDMA3_TCD_BITER_ELINK(ch)

#define EDMA3_TCD_ATTR_DSIZE(dsize)      ((dsize & 0x0007))
#define EDMA3_TCD_ATTR_DMOD(dmod)        ((dmod & 0x001F) << 3)
#define EDMA3_TCD_ATTR_SSIZE(ssize)      ((ssize & 0x0007) << 8)
#define EDMA3_TCD_ATTR_SMOD(smod)        ((smod & 0x001F) << 11)
#define EDMA3_TCD_ATTR_SSIZE_8BIT        (0x0000)
#define EDMA3_TCD_ATTR_SSIZE_16BIT       (0x0100)
#define EDMA3_TCD_ATTR_SSIZE_32BIT       (0x0200)
#define EDMA3_TCD_ATTR_SSIZE_64BIT       (0x0300)
#define EDMA3_TCD_ATTR_SSIZE_32BYTE      (0x0500)
#define EDMA3_TCD_ATTR_DSIZE_8BIT        (0x0000)
#define EDMA3_TCD_ATTR_DSIZE_16BIT       (0x0001)
#define EDMA3_TCD_ATTR_DSIZE_32BIT       (0x0002)
#define EDMA3_TCD_ATTR_DSIZE_64BIT       (0x0003)
#define EDMA3_TCD_ATTR_DSIZE_32BYTE      (0x0005)

#define EDMA3_TCD_SOFF_SOFF(x)           (x)
#define EDMA3_TCD_NBYTES_NBYTES(x)       ((x) & 0x3FFF)
#define EDMA3_TCD_SLAST_SLAST(x)         (x)
#define EDMA3_TCD_DADDR_DADDR(x)         (x)
#define EDMA3_TCD_CITER_CITER(x)         ((x) & 0x7FFF)
#define EDMA3_TCD_DOFF_DOFF(x)           (x)
#define EDMA3_TCD_DLAST_SGA_DLAST_SGA(x) (x)
#define EDMA3_TCD_BITER_BITER(x)         ((x) & 0x7FFF)

#define EDMA3_TCD_CSR_START              (1 << 0)
#define EDMA3_TCD_CSR_INT_MAJOR          (1 << 1)
#define EDMA3_TCD_CSR_INT_HALF           (1 << 2)
#define EDMA3_TCD_CSR_D_REQ              (1 << 3)
#define EDMA3_TCD_CSR_E_SG               (1 << 4)
#define EDMA3_TCD_CSR_E_LINK             (1 << 5)
#define EDMA3_TCD_CSR_E_EOP              (1 << 6)
#define EDMA3_TCD_CSR_E_SDA              (1 << 7)
#define EDMA3_TCD_CSR_MAJOR_LINK(ch)     ((ch & 0x1f) << 8)
#define EDMA3_TCD_CSR_BWC(bwc)           ((bwc & 0x3) << 14)
    #define BWC_NO_STALL                 0
    #define BWC_STALL_EACH_4_CYCLES      2
    #define BWC_STALL_EACH_8_CYCLES      3

#define EDMA3_TCD_BITER_0to8(biter)      (biter & 0x1ff)
#define EDMA3_TCD_BITER_0to14(biter)     (biter & 0x7fff)
#define EDMA3_TCD_BITER_LINK(ch)         ((ch & 0x1f) << 9)
#define EDMA3_TCD_BITER_E_LINK           (1 << 15)

#define EDMA3_TCD_SIZE_8Bit              0
#define EDMA3_TCD_SIZE_16Bit             1
#define EDMA3_TCD_SIZE_32Bit             2
#define EDMA3_TCD_SIZE_64Bit             3
#define EDMA3_TCD_SIZE_32Byte            5

/* EDMAMUX Registers */
#define EDMAMUX_CHCFG(x)                 (x)
#define EDMAMUX_CHCFG_DIS                0x0
#define EDMAMUX_CHCFG_ENBL               0x80
#define EDMAMUX_CHCFG_SOURCE(n)          ((n) & 0x3F)


#define EDMA3_SIZE                       0x44000
#define DMAMUX_SIZE                      0x4000

#define SDRAM_BASE                       0x80000000

#ifndef FSL_EDMA3_DESCRIPTION_STR
    #define FSL_EDMA3_DESCRIPTION_STR    "FSL EDMA3 Controller"
#endif
#define EDMA3_MEMORY_NAME                "FSL EDMA3 MEMORY"


#define EDMA3_MAX_NUM	                 2
#define DMAMUX_MAX_NUM                   2
#define DMAMUX_TX_IRQ_MAX_NUM            2
#define EDMA3_CHANNEL_NUM                (EDMA3_MAX_NUM * 32)
#define EDMA3_MIN_PRIORITY               0
#define EDMA3_MAX_PRIORITY               (EDMA3_CHANNEL_NUM-1)

#define MAX_TCDS                         EDMA3_CHANNEL_NUM

#define CHANS_PER_EDMA3                  32
#define CHANS_PER_MUX                    16

#define MAX_FRAG_LEN                     65536
#define MAX_DESCRIPTORS                  1024
#define DMA_NUM_REQ_LINE                 EDMA3_CHANNEL_NUM
#define DMA_LOWER_MEM_LIMIT              0x00000000UL
#define DMA_UPPER_MEM_LIMIT              0x10000000UL
#define DMA_MAX_NUM_TCD                  MAX_TCDS

/* Define supported xfer unit size */
#define DMA_XFER_UNIT_SIZE_IN_BYTE(nbBytes)     (nbBytes)
#define DMA_XFER_UNIT_SIZE              ( DMA_XFER_UNIT_SIZE_IN_BYTE(1) \
                                        | DMA_XFER_UNIT_SIZE_IN_BYTE(2) \
                                        | DMA_XFER_UNIT_SIZE_IN_BYTE(4) \
                                        | DMA_XFER_UNIT_SIZE_IN_BYTE(16)\
                                        | DMA_XFER_UNIT_SIZE_IN_BYTE(32))

/* Data Structures */

typedef enum {
    SOC_S32G,
    SOC_UNKNOWN
} fsl_edma3_soc_t;

typedef struct {
    uint32_t          fsl_edma3_phys;
    uintptr_t         fsl_edma3_base;
    int               tx_irq_num;
    int               tx_irq[DMAMUX_TX_IRQ_MAX_NUM];
    int               err_irq;
    int               dmamux_slots_num;
    int               dmamux_num;
    uint32_t          dmamux_phys_base[DMAMUX_MAX_NUM];
    uintptr_t         fsl_dmamux_base[DMAMUX_MAX_NUM];
} fsl_edma3_t;


/* TCB (Transfer Control descriptor) Structure */
typedef struct {
    uint32_t    saddr;          /* Source Address */
    uint16_t    soff;           /* Signed source address offset */
    uint16_t    attr;           /* Transfer attribute (smod, ssize, dmod, dsize) */
    uint32_t    nbytes;         /* Inner "Minor" byte count (nbytes) */
    uint32_t    slast;          /* Last source address adjustement */
    uint32_t    daddr;          /* Destination address */
    uint16_t    doff;           /* Signed Destination Address offset */
    uint16_t    citer;          /* Current "Major" iteration Count */
    uint32_t    dlast_sga;      /* Last Destination address adjustement/scatter gather address */
    uint16_t    csr;            /* Channel Control/Status */
    uint16_t    biter;          /* Beginning "Major" iteration count */
} fsl_edma3_tcd_t;


typedef struct swTCDPtrInfo {
    struct swTCDPtrInfo*       pPrev;
    struct swTCDPtrInfo*       pNext;
    fsl_edma3_tcd_t*            pTcd;
    off64_t                    phyAddr;
} tcdListItem_t;

typedef struct {
    uint32_t            numTCD;     /* Size of TCD Tabl */
    tcdListItem_t*      tcdList;    /* Pointer to a TCD tables */
    fsl_edma3_tcd_t*     tcdArray;
                       /* Task's current BD pointer. This pointer is
                        * used to set a task's BD pointer upon startup.
                        * It is only valid for BD tasks and only after
                        * TaskSetup() or TaskBDReset() are called. You
                        * cannot use this to track a task's BD pointer.
                        */
    tcdListItem_t*      pCurrent;
    tcdListItem_t*      pNextFree;

                       /* Current number of buffer descriptors assigned but
                        * not released yet.
                        */
    uint32_t            currBDInUse;
} TcdChanCtrl_t;

typedef struct {
    unsigned            edma3_id;   /* EDMA3 ID */
    unsigned            mux;        /* EDMA mux */
    unsigned            mux_slot;   /* Slot on the EDMA mux */
    int                 id;         /* Channel ID */
    int                 busy;       /* Channel is already in used */
    int                 running;    /* Channel is running */
    unsigned            curXferSize;
    int                 irq;

    TcdChanCtrl_t       tcdCtrl;    /* TCD Channel Control */
    rsrc_request_t      rsrc_req;   /* reserve resource request */
    struct sigevent     event;
    dma_attach_flags    flags;
    dma_mode_flags      mode_flags;
    int                 ctrl;

}dma_channel_t;


/* fsl_edma3 shared memory */
typedef struct {
    uint32_t process_cnt;
    pthread_mutex_t libinit_mutex;
} fsl_edma3_shmem_t;

#define RESERVED_INTERNAL_API __attribute__(( visibility ("hidden") ))

/* prototypes */
extern RESERVED_INTERNAL_API void fsl_edma3_seed_shared_mem(void);
extern RESERVED_INTERNAL_API int fsl_edma3_sync_init(void);
extern RESERVED_INTERNAL_API void fsl_edma3_sync_fini(void);
extern RESERVED_INTERNAL_API pthread_mutex_t * fsl_edma3_sync_libinit_mutex_get();
extern RESERVED_INTERNAL_API int fsl_edma3_sync_is_first_process();
extern RESERVED_INTERNAL_API int fsl_edma3_sync_is_last_process();
extern RESERVED_INTERNAL_API void fsl_edma3_sync_process_cnt_incr();
extern RESERVED_INTERNAL_API void fsl_edma3_sync_process_cnt_decr();
extern RESERVED_INTERNAL_API int fsl_edma3_tx_irq_init(uint32_t irq);
extern RESERVED_INTERNAL_API int fsl_edma3_tx_irq_attch(uint32_t irq);
extern RESERVED_INTERNAL_API void fsl_edma3_tx_irq_fini();
extern RESERVED_INTERNAL_API void fsl_edma3_tx_irq_event_add(uint32_t channel, const struct sigevent *event);
extern RESERVED_INTERNAL_API void fsl_edma3_tx_irq_event_remove(uint32_t channel);
extern RESERVED_INTERNAL_API int fsl_edma3_err_irq_init(uint32_t irq);
extern RESERVED_INTERNAL_API int fsl_edma3_err_irq_attch(uint32_t irq);
extern RESERVED_INTERNAL_API void fsl_edma3_err_irq_fini();
extern RESERVED_INTERNAL_API int fsl_slog2_init(void);
extern RESERVED_INTERNAL_API int fsl_edma3_slogf(const char *fmt, ...);

#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/lib/dma/fsl-edma3/fsl_edma3.h $ $Rev: 862698 $")
#endif
