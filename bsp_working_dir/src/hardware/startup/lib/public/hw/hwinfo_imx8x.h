/*
 * $QNXLicenseC:
 * Copyright 2016, QNX Software Systems.
 * Copyright 2016, Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
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
* i.MX SOC devices
*
* This file contains names for all of the devices that may exists on any i.MX
* processor variant. Specifically, these
* are the internal devices that are part of the IMX8X SOC and not board specific
* devices. Board specific devices are defined in a similarly named board specific file
*
* The names are added to the hwinfo section of the syspage (at the discretion of
* the startup writer) so that drivers can use hwi_find_device() to determine
* the existence of SOC device instances in a portable fashion and optionally
* retrieve device specific information like interrupt vectors, etc.
*
* Not all of the devices are on every processor so hwi_find_device() may return
* HWI_NULL_OFF if a device, or device instance (ie. unit) is not found.
*
*/

#ifndef __IMX_HWINFO_H
#define __IMX_HWINFO_H

#include <hw/sysinfo.h>

/*
 * =============================================================================
 *
 *                                B U S E S
 *
 * =============================================================================
*/
/* I2C - 1 interrupt */
#define IMX_HWI_I2C                         HWI_ITEM_BUS_I2C

/* USB */
#define IMX_HWI_USB                         HWI_ITEM_BUS_USB

/* SPI */
#define IMX_HWI_SPI                         HWI_ITEM_BUS_SPI

/*
 * =============================================================================
 *
 *                              D E V I C E S
 *
 * =============================================================================
*/
/* SDHC */
#define IMX_HWI_SDHC                        "sdhc"

/* DMA - multiple interrupts are assigned per available channel for each device */
#define IMX_HWI_DMA                         "dma"

/* FEC - interrupts are assigned */
#define IMX_HWI_FEC                         "FEC"  // "fec"
#define IMX_HWI_LEGACY_FEC                  "fec"  // "tsec"

/* DUART - 1 interrupt */
#define IMX_HWI_UART                        "uart"

/* Security - 1 interrupt per device */
#define IMX_HWI_SECURITY                    "security"

/* GPIO - 1 interrupt per device */
#define IMX_HWI_GPIO                        "gpio"

/* SSI BUS  */
#define IMX_HWI_SSI                         "ssi"

/* SDRAM  */
#define IMX_HWI_SDRAM                       "sdram"

/* Watchdog  */
#define IMX_HWI_WDOG                         "wdog"

/* MLB  */
#define IMX_HWI_MLB                         "mlb"

/* Graphics 2D  */
#define IMX_HWI_GPU_2D                      "gpu2d"

/* Graphics 3D  */
#define IMX_HWI_GPU_3D                      "gpu3d"

/* Image Processing Unit  */
#define IMX_HWI_IPU                         "ipu"

/* Video Processing Unit  */
#define IMX_HWI_VPU                         "vpu"

/* ESAI  */
#define IMX_HWI_ESAI                        "esai"

/* ESAI  */
#define IMX_HWI_SAI                         "sai"

/* ASRC  */
#define IMX_HWI_ASRC                        "asrc"

#endif  /* __IMX_HWINFO_H */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/public/hw/hwinfo_imx8x.h $ $Rev: 886103 $")
#endif
