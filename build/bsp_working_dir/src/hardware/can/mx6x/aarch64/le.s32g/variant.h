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

#ifndef _VARIANT_H
#define _VARIANT_H


#define S32G_FLEXCAN
//#define DEBUG_DRVR

#define FLEXCAN_MECR                0xAE0

/* FLEXCAN memory error control register (MECR) bits */
#define FLEXCAN_MECR_ECRWRDIS		(0x01 << 31)
#define FLEXCAN_MECR_HANCEI_MSK		(0x01 << 19)
#define FLEXCAN_MECR_FANCEI_MSK		(0x01 << 18)
#define FLEXCAN_MECR_CEI_MSK		(0x01 << 16)
#define FLEXCAN_MECR_HAERRIE		(0x01 << 15)
#define FLEXCAN_MECR_FAERRIE		(0x01 << 14)
#define FLEXCAN_MECR_EXTERRIE		(0x01 << 13)
#define FLEXCAN_MECR_RERRDIS		(0x01 << 9)
#define FLEXCAN_MECR_ECCDIS		(0x01 << 8)
#define FLEXCAN_MECR_NCEFAFRZ		(0x01 << 7)


/* S32G support 128MB */
#undef RINGO_CAN_REG_SIZE_FLEXCAN
#undef RINGO_CAN_MEM_SIZE_FLEXCAN
#undef RINGO_CANLAM_MEM_SIZE
#undef RINGO_CAN_NUM_MAILBOX_FLEXCAN
#undef RINGO_CAN_MAILBOX_SIZE
#undef FLEXCAN_SET_MODE_RETRIES

#define RINGO_CAN_REG_SIZE_FLEXCAN	0x80
#define RINGO_CAN_MEM_SIZE_FLEXCAN	0x800
#define RINGO_CANLAM_MEM_SIZE		0x200
#define RINGO_CAN_NUM_MAILBOX_FLEXCAN	128
#define RINGO_CAN_MAILBOX_SIZE		4
#define FLEXCAN_SET_MODE_RETRIES	255

#undef RINGO_CANMC_MAXMB_MASK
#define RINGO_CANMC_MAXMB_MASK		0x0000007F    /* 128 MBs */
#undef RINGO_CANMC_MAXMB_MAXVAL
#define RINGO_CANMC_MAXMB_MAXVAL	0x0000007F    /* 128 MBs */
#undef CAN_NUM_MAILBOX_RSVD
#define CAN_NUM_MAILBOX_RSVD		0

#undef CAN_FIRST_USER_MAILBOX_INDEX
#define CAN_FIRST_USER_MAILBOX_INDEX    CAN_FIRST_MAILBOX_INDEX
#undef CAN_LAST_USER_MAILBOX_INDEX
#define CAN_LAST_USER_MAILBOX_INDEX     CAN_LAST_MAILBOX_INDEX

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/can/mx6x/aarch64/le.s32g/variant.h $ $Rev: 905902 $")
#endif
