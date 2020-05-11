/*
 * $QNXLicenseC:
 * Copyright 2020, QNX Software Systems.
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


#include "startup.h"
#include "board.h"
#include <aarch64/s32g.h>

/* CAN configuration */
#define PAD_CTL_CAN_TX_MSCRx	(SIUL2_MSCR_SRE_100MHZ | SIUL2_MSCR_OBE)
#define PAD_CTL_CAN_RX_MSCR	(SIUL2_MSCR_IBE)
#define PAD_CTL_CAN_RX_IMCR	(SIUL2_MSCR_SSS_2)
#define PAD_CTL_SRC_SIG_SEL1	(SIUL2_MSCR_SSS_1)
#define PAD_CTL_SRC_SIG_SEL2	(SIUL2_MSCR_SSS_2)


/* EVB Board, Output on PROC Board, J79 */
#define S32G_MSCR_PC12		44
#define S32G_MSCR_PC11		43
#define S32G_IMCR_PC11		513

#define S32G_PC12_CAN0_TXD	(PAD_CTL_CAN_TX_MSCRx | PAD_CTL_SRC_SIG_SEL1)
#define S32G_PC11_CAN0_RXD_OUT	PAD_CTL_CAN_RX_MSCR
#define S32G_PC11_CAN0_RXD_IN	PAD_CTL_CAN_RX_IMCR

/* EVB Board, Output on Platform Board, J19 */
#define S32G_MSCR_PB11		27
#define S32G_MSCR_PB12		28
#define S32G_IMCR_PB12		632

#define S32G_PB11_CAN2_TXD	(PAD_CTL_CAN_TX_MSCRx | PAD_CTL_SRC_SIG_SEL2)
#define S32G_PB12_CAN2_RXD_OUT	PAD_CTL_CAN_RX_MSCR
#define S32G_PB12_CAN2_RXD_IN	PAD_CTL_CAN_RX_IMCR

/* EVB Board, Output on Platform Board, J20 */
#define S32G_MSCR_PB9		25
#define S32G_MSCR_PB10		26
#define S32G_IMCR_PB10		633

#define S32G_PB9_CAN3_TXD	(PAD_CTL_CAN_TX_MSCRx | PAD_CTL_SRC_SIG_SEL2)
#define S32G_PB10_CAN3_RXD_OUT	PAD_CTL_CAN_RX_MSCR
#define S32G_PB10_CAN3_RXD_IN	PAD_CTL_CAN_RX_IMCR

void init_pinmux_can()
{
	uint32_t val;

	/* Muxing for CAN0 */
	/* set CAN0: TXD */
	out32(S32G_SIUL2_0_BASE + SIUL2_0_MSCRn(S32G_MSCR_PC12), S32G_PC12_CAN0_TXD);
	/* set CAN: RXD_OUT*/
	out32(S32G_SIUL2_0_BASE + SIUL2_0_MSCRn(S32G_MSCR_PC11), S32G_PC11_CAN0_RXD_OUT);
	out32(S32G_SIUL2_0_BASE + SIUL2_0_IMCRn(S32G_IMCR_PC11), S32G_PC11_CAN0_RXD_IN);

	/* Muxing for CAN2 */
	/* set CAN1:TXD */
	out32(S32G_SIUL2_0_BASE + SIUL2_0_MSCRn(S32G_MSCR_PB11), S32G_PB11_CAN2_TXD);
	/* set CAN1:RXD */
	out32(S32G_SIUL2_0_BASE + SIUL2_0_MSCRn(S32G_MSCR_PB12), S32G_PB12_CAN2_RXD_OUT);
	out32(S32G_SIUL2_1_BASE + SIUL2_1_IMCRn(S32G_IMCR_PB12), S32G_PB12_CAN2_RXD_IN);

	/* Muxing for CAN3 */
	/* set CAN1:TXD */
	out32(S32G_SIUL2_0_BASE + SIUL2_0_MSCRn(S32G_MSCR_PB9), S32G_PB9_CAN3_TXD);
	/* set CAN1:RXD */
	out32(S32G_SIUL2_0_BASE + SIUL2_0_MSCRn(S32G_MSCR_PB10), S32G_PB10_CAN3_RXD_OUT);
	out32(S32G_SIUL2_1_BASE + SIUL2_1_IMCRn(S32G_IMCR_PB10), S32G_PB10_CAN3_RXD_IN);

}

void init_can()
{
	init_pinmux_can();
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/evb/init_can.c $ $Rev: 905752 $")
#endif
