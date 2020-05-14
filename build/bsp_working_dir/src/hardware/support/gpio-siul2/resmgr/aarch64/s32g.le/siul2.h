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

/*
 * NXP SIUL2:
 */

#ifndef	__AARCH64_SIUL2_H_INCLUDED
#define	__AARCH64_SIUL2_H_INCLUDED



/*
 * S32G SIUL2
 */
#define SIUL2_0_BASE		0x4009C000	//GPIO 0 ~ 101
#define SIUL2_0_SIZE		0x1800

#define SIUL2_1_BASE		0x44010000	//GPIO 112 ~ 190
#define SIUL2_1_SIZE		0x1800

#define SIUL2_MIDR1		0x04
#define SIUL2_MIDR2		0x08
#define SIUL2_DISR0		0x10
#define SIUL2_DIRER0		0x18
#define SIUL2_DIRSR0		0x20
#define SIUL2_IREER0		0x28
#define SIUL2_IFEER0		0x30
#define SIUL2_IFER0		0x38
#define SIUL2_IFMCRn(i)		(0x40 + 4 * (i))
#define SIUL2_IFCPR		0xC0

#define SIUL2_0_MSCR		0x240
#define SIUL2_0_IMCR		0xA40
#define SIUL2_0_GPDO		0x1300
#define SIUL2_0_GPDI		0x1500
#define SIUL2_0_PGPDO		0x1700
#define SIUL2_0_PGPDI		0x1740
#define SIUL2_0_MPGPDO		0x1780

#define SIUL2_1_MSCR		0x400
#define SIUL2_1_IMCR		0xC1C
#define SIUL2_1_GPDO		0x1370
#define SIUL2_1_GPDI		0x1570
#define SIUL2_1_PGPDO		0x170E
#define SIUL2_1_PGPDI		0x174E
#define SIUL2_1_MPGPDO		0x1780


/* SIUL2_MSCR masks */
#define SIUL2_MSCR_OBE(v)		((v) & 0x00200000)
#define SIUL2_MSCR_OBE_EN		(1 << 21)

#define SIUL2_MSCR_ODE(v)		((v) & 0x00100000)
#define SIUL2_MSCR_ODE_EN		(1 << 20)

#define SIUL2_MSCR_IBE(v)		((v) & 0x00080000)
#define SIUL2_MSCR_IBE_EN		(1 << 19)

#define SIUL2_MSCR_INV(v)		((v) & 0x00020000)
#define SIUL2_MSCR_INV_EN		(1 << 17)

#define SIUL2_MSCR_SRE(v)		((v) & 0x0001C000)
#define SIUL2_MSCR_SRE_SPEED_208M	(0 << 14)
#define SIUL2_MSCR_SRE_SPEED_150M	(0x4 << 14)
#define SIUL2_MSCR_SRE_SPEED_100M	(0x5 << 14)
#define SIUL2_MSCR_SRE_SPEED_50M	(0x6 << 14)
#define SIUL2_MSCR_SRE_SPEED_25M	(0x7 << 14)

#define SIUL2_MSCR_PUE(v)		((v) & 0x00002000)
#define SIUL2_MSCR_PUE_EN		(1 << 13)

#define SIUL2_MSCR_PUS(v)		((v) & 0x00001000)
#define SIUL2_MSCR_PUS_DOWN		(0 << 12)
#define SIUL2_MSCR_PUS_UP		(1 << 12)

#define SIUL2_MSCR_RCVR(v)		((v) & 0x00000400)
#define SIUL2_MSCR_DSE_240ohm		(1 << 10)

#define SIUL2_MSCR_SMC(v)		((v) & 0x00000020)
#define SIUL2_MSCR_SMC_EN		(1 << 5)

#define SIUL2_MSCR_SSS_MASK		(0x7 << 0)

#define SIUL2_0_GPIO_MIN		0
#define SIUL2_0_GPIO_MAX		101

#define SIUL2_1_GPIO_MIN		112
#define SIUL2_1_GPIO_MAX		190


#endif	/* __AARCH64_SIUL2_H_INCLUDED */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/support/gpio-siul2/resmgr/aarch64/s32g.le/siul2.h $ $Rev: 859876 $")
#endif
