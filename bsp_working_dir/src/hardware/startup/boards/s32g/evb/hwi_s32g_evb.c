/*
 * $QNXLicenseC:
 * Copyright 2019-2020, QNX Software Systems.
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
#include "hwinfo_private.h"
#include <hw/hwinfo_imx6x.h>
#include <drvr/hwinfo.h>
#include <aarch64/s32g.h>
#include "s32g_startup.h"

/*
 * Add S32G peripherals to the hardware info section of the syspage.
 * Ideally these peripherals will be found in the FDT but the system page can be used as a backup.
 */

/* instead of hwibus_add_can() function use the following function to avoid empty tag creation
that happens inside hwibus_add_can(), as facing difficulty to fill up that empty tag */

static unsigned s32g_hwibus_add_can(unsigned parent_hwi_off, hwiattr_can_t *attr);


void hwi_s32gevb()
{
	unsigned hwi_bus_internal = 0;

	/* Add ENET */
	{
		unsigned hwi_off;
		hwiattr_enet_t attr = HWIATTR_ENET_T_INITIALIZER;
		HWIATTR_ENET_SET_NUM_IRQ(&attr, 1);

		/* Create DWC0 */
		HWIATTR_ENET_SET_LOCATION(&attr, S32G_ENET_BASE, S32G_ENET_SIZE, 0, hwi_find_as(S32G_ENET_BASE, 1));
		hwi_off = hwidev_add_enet("dwc", &attr, hwi_bus_internal);
		ASSERT(hwi_find_unit(hwi_off) == 0);

		/* Add IRQ number */
		hwitag_set_avail_ivec(hwi_off, 0, S32G_ENET_IRQ);

	}

	/* add I2C (unless directed not to) */
	{
		unsigned hwi_off;
		hwiattr_i2c_t attr = HWIATTR_I2C_T_INITIALIZER;
		HWIATTR_I2C_SET_NUM_IRQ(&attr, 1);

		/* create I2C controller 0 */
		HWIATTR_I2C_SET_LOCATION(&attr, S32G_I2C0_BASE, S32G_I2C_SIZE, 0, hwi_find_as(S32G_I2C0_BASE, 1));
		hwi_off = hwibus_add_i2c(hwi_bus_internal, &attr);
		ASSERT(hwi_find_unit(hwi_off) == 0);
		hwitag_set_ivec(hwi_off, 0, S32G_I2C0_IRQ);

		/* create I2C controller 1 */
		HWIATTR_I2C_SET_LOCATION(&attr, S32G_I2C1_BASE, S32G_I2C_SIZE, 0, hwi_find_as(S32G_I2C1_BASE, 1));
		hwi_off = hwibus_add_i2c(hwi_bus_internal, &attr);
		ASSERT(hwi_find_unit(hwi_off) == 1);
		hwitag_set_ivec(hwi_off, 0, S32G_I2C1_IRQ);

		/* create I2C controller 2 */
		HWIATTR_I2C_SET_LOCATION(&attr, S32G_I2C2_BASE, S32G_I2C_SIZE, 0, hwi_find_as(S32G_I2C2_BASE, 1));
		hwi_off = hwibus_add_i2c(hwi_bus_internal, &attr);
		ASSERT(hwi_find_unit(hwi_off) == 2);
		hwitag_set_ivec(hwi_off, 0, S32G_I2C2_IRQ);

		/* create I2C controller 3 */
		HWIATTR_I2C_SET_LOCATION(&attr, S32G_I2C3_BASE, S32G_I2C_SIZE, 0, hwi_find_as(S32G_I2C3_BASE, 1));
		hwi_off = hwibus_add_i2c(hwi_bus_internal, &attr);
		ASSERT(hwi_find_unit(hwi_off) == 3);
		hwitag_set_ivec(hwi_off, 0, S32G_I2C3_IRQ);

		/* create I2C controller 4 */
		HWIATTR_I2C_SET_LOCATION(&attr, S32G_I2C4_BASE, S32G_I2C_SIZE, 0, hwi_find_as(S32G_I2C4_BASE, 1));
		hwi_off = hwibus_add_i2c(hwi_bus_internal, &attr);
		ASSERT(hwi_find_unit(hwi_off) == 4);
		hwitag_set_ivec(hwi_off, 0, S32G_I2C4_IRQ);
	}

	/* add DSPI device */
	{
		unsigned hwi_off;
		hwiattr_spi_t spi_attr = HWIATTR_SPI_T_INITIALIZER;

		/* All MCSPIs opearte on 133MHz only */
		struct hwi_inputclk clksrc = {.clk = 133000000, .div = 1};

		/* each SPI has an interrupt */
		HWIATTR_SPI_SET_NUM_IRQ(&spi_attr, 1);
		HWIATTR_SPI_SET_NUM_CLK(&spi_attr, 1);

		/* create spi0 and set the clock source */
		HWIATTR_SPI_SET_LOCATION(&spi_attr, S32G_SPI0_BASE, S32G_SPI_SIZE, 0,
                   hwi_find_as(S32G_SPI0_BASE, 1));
		hwi_off = hwibus_add_spi(HWI_NULL_OFF, &spi_attr);
		ASSERT(hwi_find_unit(hwi_off) == 0);
		hwitag_set_inputclk(hwi_off, 0, &clksrc);
		hwitag_set_ivec(hwi_off, 0, S32G_SPI0_IRQ);

		/* create spi1 and set the clock source */
		HWIATTR_SPI_SET_LOCATION(&spi_attr, S32G_SPI1_BASE, S32G_SPI_SIZE, 0,
                   hwi_find_as(S32G_SPI1_BASE, 1));
		hwi_off = hwibus_add_spi(HWI_NULL_OFF, &spi_attr);
		ASSERT(hwi_find_unit(hwi_off) == 1);
		hwitag_set_inputclk(hwi_off, 0, &clksrc);
		hwitag_set_ivec(hwi_off, 0, S32G_SPI1_IRQ);

		/* create spi2 and set the clock source */
		HWIATTR_SPI_SET_LOCATION(&spi_attr, S32G_SPI2_BASE, S32G_SPI_SIZE, 0,
                   hwi_find_as(S32G_SPI2_BASE, 1));
		hwi_off = hwibus_add_spi(HWI_NULL_OFF, &spi_attr);
		ASSERT(hwi_find_unit(hwi_off) == 2);
		hwitag_set_inputclk(hwi_off, 0, &clksrc);
		hwitag_set_ivec(hwi_off, 0, S32G_SPI2_IRQ);

		/* create spi3 and set the clock source */
		HWIATTR_SPI_SET_LOCATION(&spi_attr, S32G_SPI3_BASE, S32G_SPI_SIZE, 0,
                   hwi_find_as(S32G_SPI3_BASE, 1));
		hwi_off = hwibus_add_spi(HWI_NULL_OFF, &spi_attr);
		ASSERT(hwi_find_unit(hwi_off) == 3);
		hwitag_set_inputclk(hwi_off, 0, &clksrc);
		hwitag_set_ivec(hwi_off, 0, S32G_SPI3_IRQ);

		/* create spi4 and set the clock source */
		HWIATTR_SPI_SET_LOCATION(&spi_attr, S32G_SPI4_BASE, S32G_SPI_SIZE, 0,
                   hwi_find_as(S32G_SPI4_BASE, 1));
		hwi_off = hwibus_add_spi(HWI_NULL_OFF, &spi_attr);
		ASSERT(hwi_find_unit(hwi_off) == 4);
		hwitag_set_inputclk(hwi_off, 0, &clksrc);
		hwitag_set_ivec(hwi_off, 0, S32G_SPI4_IRQ);

		/* create spi5 and set the clock source */
		HWIATTR_SPI_SET_LOCATION(&spi_attr, S32G_SPI5_BASE, S32G_SPI_SIZE, 0,
                   hwi_find_as(S32G_SPI5_BASE, 1));
		hwi_off = hwibus_add_spi(HWI_NULL_OFF, &spi_attr);
		ASSERT(hwi_find_unit(hwi_off) == 5);
		hwitag_set_inputclk(hwi_off, 0, &clksrc);
		hwitag_set_ivec(hwi_off, 0, S32G_SPI5_IRQ);
	}

	/* add the WATCHDOG device */
	{
		unsigned hwi_off;
		hwiattr_timer_t attr = HWIATTR_TIMER_T_INITIALIZER;
		const struct hwi_inputclk clksrc_kick = {.clk = 10, .div = 1};
		HWIATTR_TIMER_SET_NUM_CLK(&attr, 1);
		HWIATTR_TIMER_SET_LOCATION(&attr, S32G_SWT4_BASE, S32G_SWT_SIZE, 0, hwi_find_as(S32G_SWT4_BASE, 1));
		hwi_off = hwidev_add_timer("wdog", &attr,  HWI_NULL_OFF);
		ASSERT(hwi_off != HWI_NULL_OFF);
		hwitag_set_inputclk(hwi_off, 0, (struct hwi_inputclk *)&clksrc_kick);

		hwi_off = hwidev_add("wdt,options", 0, HWI_NULL_OFF);
		hwitag_add_regname(hwi_off, "write_width", 32);
		hwitag_add_regname(hwi_off, "enable_width", 32);
		hwitag_add_regname(hwi_off, "enable_mask", 0x1);
		hwitag_add_regname(hwi_off, "enable_condition", 0x1);
		hwitag_add_regname(hwi_off, "enable_offset", 0x0);

		hwi_off = hwidev_add("wdt,regwrite", 0, HWI_NULL_OFF);
		hwitag_add_regname(hwi_off, "offset", 0x10);
		hwitag_add_regname(hwi_off, "value", 0xA602);
		hwitag_add_regname(hwi_off, "offset", 0x10);
		hwitag_add_regname(hwi_off, "value", 0xB480);
	}
	/* add CAN device*/
	{
		unsigned hwi_off;
		hwiattr_can_t attr = HWIATTR_CAN_T_INITIALIZER;
		HWIATTR_CAN_SET_NUM_IRQ(&attr, 1);
		HWIATTR_CAN_SET_NUM_MEMADDR(&attr, 1);

		/* create CAN0 */
		HWIATTR_CAN_SET_LOCATION(&attr, S32G_CAN0_PORT, S32G_CAN_SIZE, 0, hwi_find_as(S32G_CAN0_PORT, 1));
		hwi_off = s32g_hwibus_add_can(hwi_bus_internal, &attr);
		hwitag_add_location(hwi_off, S32G_CAN0_MEM, S32G_CAN_SIZE, 0, 0);
		ASSERT(hwi_find_unit(hwi_off) == 0);
		hwitag_set_ivec(hwi_off, 0, S32G_CAN0_IRQ);

		/* create CAN1 */
		HWIATTR_CAN_SET_LOCATION(&attr, S32G_CAN1_PORT, S32G_CAN_SIZE, 0, hwi_find_as(S32G_CAN1_PORT, 1));
		hwi_off = s32g_hwibus_add_can(hwi_bus_internal, &attr);
		hwitag_add_location(hwi_off, S32G_CAN1_MEM, S32G_CAN_SIZE, 0, 0);
		ASSERT(hwi_find_unit(hwi_off) == 1);
		hwitag_set_ivec(hwi_off, 0, S32G_CAN1_IRQ);

		/* create CAN2 */
		HWIATTR_CAN_SET_LOCATION(&attr, S32G_CAN2_PORT, S32G_CAN_SIZE, 0, hwi_find_as(S32G_CAN2_PORT, 1));
		hwi_off = s32g_hwibus_add_can(hwi_bus_internal, &attr);
		hwitag_add_location(hwi_off, S32G_CAN2_MEM, S32G_CAN_SIZE, 0, 0);
		ASSERT(hwi_find_unit(hwi_off) == 2);
		hwitag_set_ivec(hwi_off, 0, S32G_CAN2_IRQ);

		/* create CAN3 */
		HWIATTR_CAN_SET_LOCATION(&attr, S32G_CAN3_PORT, S32G_CAN_SIZE, 0, hwi_find_as(S32G_CAN3_PORT, 1));
		hwi_off = s32g_hwibus_add_can(hwi_bus_internal, &attr);
		hwitag_add_location(hwi_off, S32G_CAN3_MEM, S32G_CAN_SIZE, 0, 0);
		ASSERT(hwi_find_unit(hwi_off) == 3);
		hwitag_set_ivec(hwi_off, 0, S32G_CAN3_IRQ);
	}

}

static unsigned s32g_hwibus_add_can(unsigned parent_hwi_off, hwiattr_can_t *attr)
{
	unsigned hwi_off = hwibus_add(HWI_ITEM_BUS_CAN, parent_hwi_off);
	if ((hwi_off != HWI_NULL_OFF) && (attr != NULL))
	{
		unsigned i;
		hwitag_add_common(hwi_off, &attr->common);
		for (i=0; i<attr->num_clks; i++)
			hwitag_add_inputclk(hwi_off, 0, 1);
	}
	return hwi_off;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/s32g/evb/hwi_s32g_evb.c $ $Rev: 905644 $")
#endif
