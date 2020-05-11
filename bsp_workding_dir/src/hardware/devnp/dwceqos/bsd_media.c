/*
 * $QNXLicenseC:
 * Copyright 2018-2019, QNX Software Systems.
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

#include <dwceqos.h>
#include <sys/malloc.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <device_qnx.h>

/*
 * This is a callback, made by the bsd media code. We passed
 * a pointer to this function during the ifmedia_init() call
 * in bsd_mii_initmedia(). Report the active media.
 */
void bsd_mii_mediastatus (struct ifnet *ifp, struct ifmediareq *ifmr)
{
    dwceqos_dev_t *dwceqos = ifp->if_softc;

    dwceqos->bsd_mii.mii_media_active = IFM_ETHER;
    dwceqos->bsd_mii.mii_media_status = IFM_AVALID;

    if ((dwceqos->cfg.flags & NIC_FLAG_LINK_DOWN) == 0) {
        dwceqos->bsd_mii.mii_media_status |= IFM_ACTIVE;
    }

    /* Report back the previously forced values */
    switch(dwceqos->cfg.media_rate) {
        case 1000*10:
            dwceqos->bsd_mii.mii_media_active |= IFM_10_T;
            break;

        case 1000*100:
            dwceqos->bsd_mii.mii_media_active |= IFM_100_TX;
            break;

        case 1000*1000:
            dwceqos->bsd_mii.mii_media_active |= IFM_1000_T;
            break;

        default:    /* This shouldn't really happen, but ... */
            dwceqos->bsd_mii.mii_media_active |= IFM_NONE;
            break;
    }

    if (dwceqos->cfg.duplex == 1) {
        dwceqos->bsd_mii.mii_media_active |= IFM_FDX;
    }

    /* Sort out flow control status */
    dwceqos->bsd_mii.mii_media_active |= dwceqos->flow;

    /* Return the data */
    ifmr->ifm_status = dwceqos->bsd_mii.mii_media_status;
    ifmr->ifm_active = dwceqos->bsd_mii.mii_media_active;
}

/*
 * This is a callback, made by the bsd media code.  We passed
 * a pointer to this function during the ifmedia_init() call
 * in bsd_mii_initmedia(). This function is called when
 * someone makes an ioctl into us, we call into the generic
 * ifmedia source, and it make this callback to actually
 * force the speed and duplex, just as if the user had
 * set the cmd line options.
 */
int bsd_mii_mediachange (struct ifnet *ifp)
{
    dwceqos_dev_t    *dwceqos = ifp->if_softc;
    struct ifmedia   *ifm = &dwceqos->bsd_mii.mii_media;
    int              old_media_rate = dwceqos->cfg.media_rate;
    int              old_duplex = dwceqos->cfg.duplex;
    int              old_flow = dwceqos->flow;
    int              user_duplex = ifm->ifm_media & IFM_FDX ? 1 : 0;
    int              user_media = ifm->ifm_media & IFM_TMASK;
    int              user_flow = ifm->ifm_media & IFM_ETH_FMASK;
    int              user_media_rate;
    int              an_media;

    if (dwceqos->cfg.verbose) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "devnp-dwceqos: %s: change media: 0x%X", __func__, ifm->ifm_media);
    }

    if (!(ifm->ifm_media & IFM_ETHER)) {
        return EOK;    /* should never happen */
    }

    if (!(ifp->if_flags & IFF_UP)) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: interface isn't up, ioctl ignored", __func__);
        return EOK;
    }

    switch (user_media) {
        case IFM_NONE:
            user_media_rate = -1;
            MDI_PowerdownPhy(dwceqos->mdi, dwceqos->cfg.phy_addr);
            dwceqos->cfg.flags |= NIC_FLAG_LINK_DOWN;
            if_link_state_change(ifp, LINK_STATE_DOWN);
            MDI_MonitorPhy(dwceqos->mdi);
            dwceqos_mdi_stop_monitor(dwceqos);
            return EOK;

        case IFM_AUTO:
            user_media_rate = 0;
            an_media = MDI_10bTFD | MDI_10bT |
                       MDI_100bT | MDI_100bTFD |
                       MDI_1000bT | MDI_1000bTFD |
                       MDI_FLOW | MDI_FLOW_ASYM;
            user_flow = IFM_FLOW;
            break;

        case IFM_10_T:        /* force 10baseT */
            user_media_rate = 10 * 1000;
            if (user_duplex) {
                an_media = MDI_10bTFD;
            } else {
                an_media = MDI_10bT;
            }
            break;

        case IFM_100_TX:      /* force 100baseTX */
            user_media_rate = 100 * 1000;
            dwceqos->cfg.media_rate = 100 * 1000;
            if (user_duplex) {
                an_media = MDI_100bTFD;
            } else {
                an_media = MDI_100bT;
            }
            break;

        case IFM_1000_T:      /* force 1000baseT */
            user_media_rate = 1000 * 1000;
            an_media = MDI_1000bTFD;
            break;

        default:              /* should never happen */
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: unknown media: 0x%X", __func__, user_media);
            return EOK;
    }

    if (user_flow & IFM_FLOW) {
        an_media |= MDI_FLOW;
    }

    if (user_flow & IFM_ETH_RXPAUSE) {
        an_media |= MDI_FLOW_ASYM;
    }

    if (user_flow & IFM_ETH_TXPAUSE) {
        an_media |= MDI_FLOW_ASYM;
    }

    /* Does the user want something different than it already is? */
    if ((user_media_rate != old_media_rate)    ||
        (user_duplex     != old_duplex)        ||
        (user_flow       != old_flow)          ||
        (dwceqos->cfg.flags &  NIC_FLAG_LINK_DOWN) ) {
        dwceqos_mdi_stop_monitor(dwceqos);
        MDI_PowerdownPhy(dwceqos->mdi, dwceqos->cfg.phy_addr);
        if_link_state_change(ifp, LINK_STATE_DOWN);
        MDI_PowerupPhy(dwceqos->mdi, dwceqos->cfg.phy_addr);
        dwceqos_mdi_start_monitor(dwceqos);
        MDI_SetAdvert(dwceqos->mdi, dwceqos->cfg.phy_addr, an_media);
        MDI_AutoNegotiate(dwceqos->mdi, dwceqos->cfg.phy_addr, NoWait);
    }

    return EOK;
}

void bsd_mii_initmedia(dwceqos_dev_t *dwceqos)
{
    int media;
    struct ifmedia    *ifm;

    ifm = &dwceqos->bsd_mii.mii_media;

    dwceqos->bsd_mii.mii_ifp = &dwceqos->ecom.ec_if;

    ifmedia_init(ifm, IFM_IMASK, bsd_mii_mediachange, bsd_mii_mediastatus);

    ifmedia_add(ifm, IFM_ETHER|IFM_NONE, 0, NULL);

    ifmedia_add(ifm, IFM_ETHER|IFM_AUTO, 0, NULL);

    // ifconfig dwc0 10baseT (x23 - half duplex)
    ifmedia_add(ifm, IFM_ETHER|IFM_10_T, 0, NULL);

    // ifconfig dwc0 10baseT-FDX mediaopt fdx (x100023)
    ifmedia_add(ifm, IFM_ETHER|IFM_10_T|IFM_FDX, 0, NULL);

    // ifconfig dwc0 10baseT-FDX mediaopt flow (x100023)
    ifmedia_add(ifm, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_FLOW, 0, NULL);

    // ifconfig dwc0 10baseT-FDX mediaopt rxpause (x100023)
    ifmedia_add(ifm, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_ETH_RXPAUSE, 0, NULL);

    // ifconfig dwc0 10baseT-FDX mediaopt txpause (x100023)
    ifmedia_add(ifm, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_ETH_TXPAUSE, 0, NULL);

    // ifconfig dwc0 100baseTX (x26 - half duplex)
    ifmedia_add(ifm, IFM_ETHER|IFM_100_TX, 0, NULL);

    // ifconfig dwc0 100baseTX-FDX mediaopt fdx (x100026 - full duplex)
    ifmedia_add(ifm, IFM_ETHER|IFM_100_TX|IFM_FDX, 0, NULL);

    // ifconfig dwc0 100baseTX-FDX mediaopt flow (x100026 - full duplex)
    ifmedia_add(ifm, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_FLOW, 0, NULL);

    // ifconfig dwc0 100baseTX-FDX mediaopt rxpause (x100026 - full duplex)
    ifmedia_add(ifm, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_ETH_RXPAUSE, 0, NULL);

    // ifconfig dwc0 100baseTX-FDX mediaopt txpause (x100026 - full duplex)
    ifmedia_add(ifm, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_ETH_TXPAUSE, 0, NULL);

    // ifconfig dwc0 1000baseT mediaopt fdx (x100030 - full duplex)
    ifmedia_add(ifm, IFM_ETHER|IFM_1000_T|IFM_FDX, 0, NULL);

    // ifconfig dwc0 1000baseT mediaopt flow (x100030 - full duplex)
    ifmedia_add(ifm, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_FLOW, 0, NULL);

    // ifconfig dwc0 1000baseT mediaopt rxpause (x100030 - full duplex)
    ifmedia_add(ifm, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_ETH_RXPAUSE, 0, NULL);

    // ifconfig dwc0 1000baseT mediaopt txpause (x100030 - full duplex)
    ifmedia_add(ifm, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_ETH_TXPAUSE, 0, NULL);

    media = IFM_ETHER;

    if ((dwceqos->cfg.media_rate != -1) && (dwceqos->cfg.duplex != -1)) {
        if (dwceqos->cfg.duplex == 1) {
	    media |= IFM_FDX;
        }
        switch (dwceqos->cfg.media_rate) {
            case 10*1000:
	        media |= IFM_10_T;
	        break;
            case 100*1000:
	        media |= IFM_100_TX;
	        break;
            case 1000*1000:
	        media |= IFM_1000_T;
	        break;
            default:			/* should never happen */
	        slogf(_SLOGC_NETWORK, _SLOG_WARNING,"devnp-dwceqos: unknown media rate: 0x%X", dwceqos->cfg.media_rate);
	    return;
        }
    }

    ifmedia_set(ifm, media);
}

/* Free any memory associated with the bsd mii.
 * ifmedia_add() allocates memory and must be freed by
 * ifmedia_delete_instance().
 */
void bsd_mii_finimedia(dwceqos_dev_t *dwceqos)
{
    ifmedia_delete_instance(&dwceqos->bsd_mii.mii_media, IFM_INST_ANY);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/dwceqos/bsd_media.c $ $Rev: 877965 $")
#endif
