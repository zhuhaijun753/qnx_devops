/*
 * $QNXLicenseC:
 * Copyright 2016, QNX Software Systems.
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

/* Module Description: s32v generic board specific interface */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <arm/mx6x.h>
#include <imx6.h>
#include <bs.h>

int s32v_init( sdio_hc_t *hc );

sdio_product_t	sdio_fs_products[] = {
	{ SDIO_DEVICE_ID_WILDCARD, 0, 0, "s32v", s32v_init },
};

sdio_vendor_t	sdio_vendors[] = {
	{ SDIO_VENDOR_ID_WILDCARD, "Freescale", sdio_fs_products },
	{ 0, NULL, NULL }
};

#define SIUL2_BASE		0x4006C000
#define SIUL2_BASE_MSCR		0x240
#define SIUL2_BASE_GPDI		0x1500


volatile uint32_t *mscr_p = NULL, *cd_p = NULL;

/* Handle CD if no card detection, or through uSDHC interrupt */
static int s32v_cd( sdio_hc_t *hc )
{
	imx6_sdhcx_hc_t		*sdhc;
	s32v_ext_t		*ext;
	uint32_t		status;
	uintptr_t		base;
	int			cstate = CD_RMV;

	sdhc = (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base = sdhc->base;
	ext = (s32v_ext_t *)hc->bs_hdl;

	/* SDx_CD and SDx_WP pins are connected so that uSDHCx_PRES_STATE register can tell CD/WP status */
	status = imx6_sdhcx_in32(base + IMX6_SDHCX_PSTATE);

	if (status & (1 << ext->cd_pin)) {
		cstate |= CD_INS;
		if (!(status & (1 << ext->wp_pin))) {
			cstate |= CD_WP;
		} else {
			cstate &= ~CD_WP;
		}
	}
	return (cstate);
}

/* CD through GPIO pins*/
static int s32v_gpio_cd( sdio_hc_t *hc )
{
	s32v_ext_t		*ext;
	int			cstate = CD_RMV;
	int			cd_input_bit = 0;

	ext = (s32v_ext_t *)hc->bs_hdl;

	/* SIUL2_GPDIn pin data in bit */
	cd_input_bit = (3 - (ext->cd_gpio % 4)) * 8;

	/* SIUL2_GPDIn: Get value of CD pin input  */
	/* CD pin low to indicate card inserted */
	if (!(*cd_p & (1 << cd_input_bit))) {
		cstate |= CD_INS;
	} else {
		cstate &= ~CD_INS;
	}

	return (cstate);
}

static int s32v_bs_args(sdio_hc_t *hc, char *options)
{
	char        *value;
	int         opt;
	s32v_ext_t  *ext = (s32v_ext_t *)hc->bs_hdl;

	/* Default values */
	ext->nocd = 0;
	ext->cd_gpio = 0;
	ext->bw = 4;
	ext->vdd1_8 = 0;

	static char     *opts[] = {
#define EMMC                0
	"emmc",             /* Implies "nocd" option is set as well */
#define NOCD                1
	"nocd",             /* No card detection capability */
#define CD_GPIO             2
	"cd_gpio",          /* CD GPIO pin*/
#define BW                  3
	"bw",               /* Data bus width */
#define VDD1_8              4
	"vdd1_8",           /* 1.8 V support capability */
	NULL};

	while (options && *options != '\0') {
		if ((opt = sdio_hc_getsubopt( &options, opts, &value)) == -1) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 0, "%s: invalid BS options %s", __func__, options);
			return EINVAL;
		}

		switch (opt) {
			case EMMC:
				hc->caps |= HC_CAP_SLOT_TYPE_EMBEDDED;
				hc->flags |= HC_FLAG_DEV_MMC;
				ext->vdd1_8 = 1;
				ext->nocd = 1;
				break;
			case NOCD:
				ext->nocd = 1;
				break;
			case CD_GPIO:
				ext->cd_gpio = strtoul(value, NULL, 0);
				break;
			case BW:
				ext->bw = strtoul(value, NULL, 0);
				break;
			case VDD1_8:
				ext->vdd1_8 = 1;
				break;
			default:
				break;
		}
	}

	return EOK;
}

static int s32v_dinit( sdio_hc_t *hc )
{
	s32v_ext_t *ext = (s32v_ext_t *)hc->bs_hdl;

	if (ext) {
		if (mscr_p)
			munmap_device_memory((void *)mscr_p, 4);
		if (cd_p)
			munmap_device_memory((void *)cd_p, 4);
		free( ext );
	}

	return (imx6_sdhcx_dinit(hc));
}

int s32v_init(sdio_hc_t *hc) {

	s32v_ext_t *ext;
	int status;
	sdio_hc_cfg_t  *cfg = &hc->cfg;
	void *p;
	const int prot_flags = PROT_READ | PROT_WRITE | PROT_NOCACHE;

	if (!(ext = calloc(1, sizeof(s32v_ext_t))))
		return ENOMEM;

	hc->bs_hdl = ext;
	ext->cd_pin = CD_PIN;
	ext->wp_pin = WP_PIN;

	if (s32v_bs_args(hc, cfg->options)) {
		return EINVAL;
	}

	if (EOK != (status = imx6_sdhcx_init(hc))) {
		return status;
	}

	if (ext->cd_gpio){
		/* SIUL2_BASE_MSCRn */
		p = mmap_device_memory(NULL, 4, prot_flags, 0, SIUL2_BASE + SIUL2_BASE_MSCR + (ext->cd_gpio * 4));
		if (p != MAP_FAILED){
			mscr_p = (volatile uint32_t *)p;
			/*MUX_MODE = 0 for CD pin (s32v: SD_CD_B: EIRQ[29]) */
			*mscr_p &= ~(0xF << 0);
			/*Clear OBE and ODE*/
			*mscr_p &= ~(0x3 << 20);
			/*SET IBE */
			*mscr_p |= (0x1 << 19);

			/* SIUL2_BASE_GPDIn */
			p = mmap_device_memory(NULL, 4, prot_flags, 0, SIUL2_BASE + SIUL2_BASE_GPDI + (ext->cd_gpio & ~3));
			if (p != MAP_FAILED){
				cd_p = (volatile uint32_t *)p;
			} else {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 0, "%s: SIUL2_BASE_GPDIn mmap_device_io failed.", __func__);
				s32v_dinit(hc);
				return ENOMEM;
			}
		} else {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 0, "%s: SIUL2_BASE_MSCRn mmap_device_io failed.", __func__);
			s32v_dinit(hc);
			return ENOMEM;
		}
	}


	/* overwrite cd/dinit functions */
	hc->entry.dinit = s32v_dinit;
	if (ext->cd_gpio)
		hc->entry.cd = s32v_gpio_cd;
	else
		hc->entry.cd = s32v_cd;

	/* Overwrite some of the capabilities that are set by imx6_sdhcx_init() */
	hc->caps	&= ~HC_CAP_CD_INTR;

	/* "bs=vdd1_8" must be set in order to enable 1.8v operations */
	if (!ext->vdd1_8) {
		hc->ocr		&= ~OCR_VDD_17_195;
		hc->caps	&= ~HC_CAP_SV_1_8V;
		hc->caps	&= ~( HC_CAP_SDR12 | HC_CAP_SDR25 | HC_CAP_SDR50 | HC_CAP_SDR104 | HC_CAP_DDR50 | HC_CAP_HS200 );
	} else {
		hc->ocr		|= OCR_VDD_17_195;
	}

	if (ext->bw == 8) {
		hc->caps |= (HC_CAP_BW4 | HC_CAP_BW8);
	} else if (ext->bw == 4) {
		hc->caps |= HC_CAP_BW4;
		hc->caps &= ~HC_CAP_BW8;
	} else if (ext->bw == 1) {
		hc->caps &= ~(HC_CAP_BW4 | HC_CAP_BW8);
	}

	return EOK;
}

int bs_event(sdio_hc_t *hc, sdio_event_t *ev) {
	int	status;

	switch(ev->code) {
		case HC_EV_CD:
			status = sdio_hc_event( hc, HC_EV_CD );
			break;

		default:
			status = ENOTSUP;
			break;
	}

	return status;
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/aarch64/s32v.le/bs.c $ $Rev: 886103 $")
#endif

