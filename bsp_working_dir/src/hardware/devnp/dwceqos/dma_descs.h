/*
 * $QNXLicenseC:
 * Copyright 2019, QNX Software Systems.
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

#ifndef    __DMA_DESCS_H__
#define    __DMA_DESCS_H__


/* Normal transmit descriptor */

/* TDES2 (read format) */
#define TDES2_HL_B1L_MASK		0x3FFF
#define TDES2_VTIR_MASK			0x3
#define TDES2_VTIR_SHIFT		14
#define TDES2_B2L_MASK			0x3FFF
#define TDES2_B2L_SHIFT			16
#define TDES2_TTSE_TMWD			(1 << 30)
#define TDES2_IOC			(1 << 31)

/* TDES3 (read format) */
#define TDES3_FL_TPL_MASK		0x7FFF
#define TDES3_CIC_TPL_MASK		0x3
#define TDES3_CIC_TPL_SHIFT		16
	#define DES_IPCSUM	(1 << 16)
	#define DES_IPPCSUM	(2 << 16)
	#define DES_IPPPCSUM	(3 << 16)
#define TDES3_TCP_PKT_PAYLOAD_MASK	0x3FFFF
#define TDES3_TSE			(1 << 18)
#define TDES3_THL_SHIFT			19
#define TDES3_SLOTNUM_MASK		0xF
#define TDES3_SLOTNUM_SHIFT		19
#define TDES3_SAIC_MASK			0x7
#define TDES3_SAIC_SHIFT		23
#define TDES3_CPC_MASK			0x3
#define TDES3_CPC_SHIFT			26

/* TDES3 (write back format) */
#define TDES3_IHE			(1 << 0)
#define TDES3_DB			(1 << 1)
#define TDES3_UF			(1 << 2)
#define TDES3_ED			(1 << 3)
#define TDES3_CC_MASK			0xF
#define TDES3_CCOUNT_SHIFT		4
#define TDES3_EC			(1 << 8)
#define TDES3_LC			(1 << 9)
#define TDES3_NC			(1 << 10)
#define TDES3_LOC			(1 << 11)
#define TDES3_PCE			(1 << 12)
#define TDES3_FF			(1 << 13)
#define TDES3_JT			(1 << 14)
#define TDES3_ES			(1 << 15)
#define TDES3_TTSS			(1 << 17)

/* TDES3 Common */
#define	TDES3_RS1V			(1 << 26)
#define	TDES3_RS1V_SHIFT		26
#define TDES3_LD			(1 << 28)
#define TDES3_FD			(1 << 29)
#define TDES3_CTXT			(1 << 30)

/* TDS3 use for both format (read and write back) */
#define TDES3_OWN			(1 << 31)


/* Normal receive descriptor */

/* RDES0 (write back format) */
#define RDES0_OVT_MASK			0xFFFF

/* RDES1 (write back format) */
#define RDES1_PT_MASK			0x7
	#define DES_PT_UDP	0x1
	#define DES_PT_TCP	0x2
	#define DES_PT_ICMP	0x3
	#define DES_PT_MASK	0x3
#define RDES1_IPHE			(1 << 3)
#define RDES1_IPV4			(1 << 4)
#define RDES1_IPV6			(1 << 5)
#define RDES1_IPCB			(1 << 6)
#define RDES1_IPCE			(1 << 7)
#define RDES1_PMT_MASK			0xF
#define RDES1_PMT_SHIFT			8
#define RDES1_PFT			(1 << 12)
#define RDES1_PV			(1 << 13)
#define RDES1_TSA			(1 << 14)
#define RDES1_TD			(1 << 15)
#define RDES1_OPC_MASK			0xFFFF
#define RDES1_OPC_SHIFT			16

/* RDES2 (write back format) */
#define RDES2_HL_MASK			0x3FF
#define RDES2_OTS			(1 << 15)
#define RDES2_SAF			(1 << 16)
#define RDES2_DAF			(1 << 17)
#define RDES2_HF			(1 << 18)
#define RDES2_MADRM_MASK		0xFF
#define RDES2_MADRM_SHIFT		19
#define RDES2_HASH_VALUE_MASK		0xFF
#define RDES2_HASH_VALUE_SHIFT		19
#define RDES2_L3FM			(1 << 27)
#define RDES2_L4FM			(1 << 28)
#define RDES2_L3L4FM_MASK		0x3
#define RDES2_L3L4FM_SHIFT		26

/* RDES3 (write back format) */
#define RDES3_PL_MASK			0x7FFF
#define RDES3_ES			(1 << 15)
#define RDES3_LTE_MASK			0x7
#define RDES3_LT_SHIFT			16
#define RDES3_DE			(1 << 19)
#define RDES3_RE			(1 << 20)
#define RDES3_OE			(1 << 21)
#define RDES3_RWT			(1 << 22)
#define RDES3_GP			(1 << 23)
#define RDES3_CE			(1 << 24)
#define RDES3_RS0V			(1 << 25)
#define RDES3_RS1V			(1 << 26)
#define RDES3_RS2V			(1 << 27)
#define RDES3_LD			(1 << 28)
#define RDES3_FD			(1 << 29)
#define RDES3_CTXT			(1 << 30)

/* RDES3 (read format) */
#define RDES3_BUF1V			(1 << 24)
#define RDES3_BUF2V			(1 << 25)
#define RDES3_IOC			(1 << 30)

/* TDS3 use for both format (read and write back) */
#define RDES3_OWN			(1 << 31)


/* Transmit and receive descriptors */
typedef struct {
    uint32_t    des0;
    uint32_t    des1;
    uint32_t    des2;
    uint32_t    des3;
    struct mbuf *m;
    char        pad[16 - sizeof(struct mbuf *)];
} dwceqos_desc_t;



#endif    /* __DWCEQOS_H__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/dwceqos/dma_descs.h $ $Rev: 877989 $")
#endif
