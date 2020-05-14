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

#define SRC_GMAC_0_CTRL_STS	0x4007C004

#define PHY_MODE		(0x1 << 0)
#define PHY_INTF_SEL_MII	(0x0 << 1)
#define PHY_INTF_SEL_RGMII	(0x1 << 1)
#define PHY_INTF_SEL_RMII	(0x4 << 1)
#define FTM0_SEL		(0x1 << 10)
#define FTM1_SEL		(0x1 << 11)



void init_gmac()
{
	/* PHY Interface Select RGMII mode */
	out32(SRC_GMAC_0_CTRL_STS, PHY_INTF_SEL_RGMII);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/evb/init_gmac.c $ $Rev: 905752 $")
#endif
