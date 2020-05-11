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

#include "proto.h"
#include <hw/dcmd_gpio_siul2.h>

static int gpio_num_valid(uint8_t gpio_pin)
{
	if((gpio_pin < SIUL2_0_GPIO_MIN)||(gpio_pin > SIUL2_1_GPIO_MAX)||
	((gpio_pin > SIUL2_0_GPIO_MAX) && (gpio_pin < SIUL2_1_GPIO_MIN)))
	{
		gpio_slogf("GPIO pin number range is [%d ~ %d] and [%d ~ %d ]. Invalid GPIO pin number:%d",
		SIUL2_0_GPIO_MIN,SIUL2_0_GPIO_MAX,SIUL2_1_GPIO_MIN, SIUL2_1_GPIO_MAX, gpio_pin);
		return -1;
	}

	return 0;
}


static void siul2_config(siul2_reg_t *siul2, uint8_t gpio_pin)
{
	if ((gpio_pin < SIUL2_0_GPIO_MAX) && (gpio_pin > SIUL2_0_GPIO_MIN)) {
		//SIUL2_0
		siul2->base = siul2_0_base;
		siul2->mscr = SIUL2_0_MSCR;
		siul2->gpdo = SIUL2_0_GPDO;
		siul2->gpdi = SIUL2_0_GPDI;
		siul2->gpio_num = gpio_pin;
	} else {
		//SIUL2_1
		siul2->base = siul2_1_base;
		siul2->mscr = SIUL2_1_MSCR;
		siul2->gpdo = SIUL2_1_GPDO;
		siul2->gpdi = SIUL2_1_GPDI;
		siul2->gpio_num = gpio_pin - SIUL2_1_GPIO_MIN;
	}
}

gpio_dev_t *
siul2_hw_init(gpio_dev_t *dev)
{
	siul2_reg_t    *siul2 = NULL;

	if (-1 == ThreadCtl(_NTO_TCTL_IO_PRIV, 0)) {
		gpio_slogf("ThreadCtl");
		return NULL;
	}

	siul2_0_base = mmap_device_io(SIUL2_0_SIZE, SIUL2_0_BASE);
	if (siul2_0_base == (uintptr_t)MAP_FAILED) {
		gpio_slogf("mmap_device_io SIUL2_0_BASE failed.");
		munmap_device_io(siul2_0_base, SIUL2_0_SIZE );
		return NULL;
        }

	siul2_1_base = mmap_device_io(SIUL2_1_SIZE, SIUL2_1_BASE);
	if (siul2_1_base == (uintptr_t)MAP_FAILED) {
		gpio_slogf("mmap_device_io SIUL2_1_BASE failed.");
		munmap_device_io(siul2_1_base, SIUL2_1_SIZE );
		return NULL;
        }

	siul2 = (siul2_reg_t *)calloc(1, sizeof(siul2_reg_t));
	if (!siul2) {
		gpio_slogf("calloc SIUL2 failed.");
		return NULL;
	}

	dev->hdl = siul2;

	return dev;
}


void siul2_hw_fini(gpio_dev_t *dev)
{
	siul2_reg_t    *siul2 = dev->hdl;

	munmap_device_io(siul2_0_base, SIUL2_0_SIZE);
	munmap_device_io(siul2_1_base, SIUL2_1_SIZE);
	dev->hdl = NULL;
	free(siul2);
}

int hw_cmd_read(void *hdl, void *buf)
{
	uint8_t         gpio_pin;
	siul2_reg_t     *siul2 = hdl;
	uint32_t  gpd_off, gpd_num;
	uint32_t val;
	gpio_devctl_t*  devmsg_ptr = (gpio_devctl_t*)buf;

	gpio_pin=devmsg_ptr->cmd_read.pin_num;

	if(gpio_num_valid(gpio_pin) == -1){
		return -1;
	}

	siul2_config(siul2, gpio_pin);

	val = in32(siul2->base + siul2->mscr + siul2->gpio_num * 4);

	if(val & SIUL2_MSCR_SSS_MASK){
		gpio_slogf("Not GPIO pin.");
		return -1;
	}
	if(!(val & SIUL2_MSCR_IBE_EN)){
		gpio_slogf("GPIO pin isn't set as input mode. Invalid for read.");
	}

	gpd_num = siul2->gpio_num / 4;
	gpd_off = 3 - (siul2->gpio_num % 4);

	devmsg_ptr->cmd_read.data = in8(siul2->base + siul2->gpdi + gpd_num * 4 + gpd_off);

	return 0;
}


int hw_cmd_write(void *hdl, void *buf)
{
	uint8_t  gpio_pin;
	uint8_t  value;
	uint8_t  gpd_off, gpd_num;
	uint32_t val;
	siul2_reg_t    *siul2 = hdl;

	gpio_devctl_t*  devmsg_ptr = (gpio_devctl_t*)buf;

	gpio_pin=devmsg_ptr->cmd_read.pin_num;
	value = devmsg_ptr->cmd_write.data;

	if(gpio_num_valid(gpio_pin) == -1){
		return -1;
	}

	siul2_config(siul2, gpio_pin);

	val = in32(siul2->base + siul2->mscr + siul2->gpio_num * 4);

	if(val & SIUL2_MSCR_SSS_MASK){
		gpio_slogf("Not GPIO pin.");
		return -1;
	}
	if(!(val & SIUL2_MSCR_OBE_EN)){
		gpio_slogf("GPIO pin isn't set as output mode. Invalid for write.");
	}

	gpd_num = siul2->gpio_num / 4;
	gpd_off = 3 - (siul2->gpio_num % 4);

	out8(siul2->base + siul2->gpdo + gpd_num * 4 + gpd_off, value);

	return 0;
}

int hw_set_input(void *hdl, uint8_t gpio_pin)
{
	uint32_t val;
	siul2_reg_t    *siul2 = hdl;

	if(gpio_num_valid(gpio_pin) == -1){
		return -1;
	}

	siul2_config(siul2, gpio_pin);

	//Set GPIO pin and input mode
	val = in32(siul2->base + siul2->mscr + siul2->gpio_num * 4);
	val &= ~(0x7 << 0);
	val &= ~SIUL2_MSCR_OBE_EN;
	val |= SIUL2_MSCR_IBE_EN;
	out32(siul2->base + siul2->mscr + siul2->gpio_num * 4, val);

	return 0;
}


int hw_set_output(void *hdl, uint8_t gpio_pin)
{
	uint32_t val;
	siul2_reg_t    *siul2 = hdl;

	if(gpio_num_valid(gpio_pin) == -1){
		return -1;
	}

	siul2_config(siul2, gpio_pin);

	//Set GPIO pin and output mode
	val = in32(siul2->base + siul2->mscr + siul2->gpio_num * 4);
	val &= ~(0x7 << 0);
	val |= SIUL2_MSCR_OBE_EN;
	val &= ~SIUL2_MSCR_IBE_EN;
	out32(siul2->base + siul2->mscr + siul2->gpio_num * 4, val);

	return 0;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/support/gpio-siul2/resmgr/init.c $ $Rev: 900026 $")
#endif
