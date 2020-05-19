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

#include <dwceqos.h>
#include <variant.h>

#ifdef S32G_FLEXCAN
#define PHY_CLOCK_RATE	CR_250
#else
#define PHY_CLOCK_RATE	CR_35
#endif

static void  dwceqos_MDI_MonitorPhy (void *arg);

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static uint16_t dwceqos_mii_read(void *handle, uint8_t phy_id, uint8_t location)
{
    dwceqos_dev_t   *dwceqos = (dwceqos_dev_t *)handle;
    uint32_t        addr;
    int             data;
    int             cnt;

    addr = (PA(phy_id) & PA_MASK) |
           (RDA(location) & RDA_MASK) |
           (PHY_CLOCK_RATE & CR_MASK) |
           GB |
           GOC_READ;

    out32(dwceqos->mac_base + MAC_MDIO_ADDR, addr);

    /* Wait for command to complete, it takes less than 128us  */
    cnt = MAX_MII_RW_TIMEOUT;
    while ((in32(dwceqos->mac_base + MAC_MDIO_ADDR) & GB) && cnt ) {
        cnt--;
        nanospin_ns (1000);
    }

    if( cnt==0 ) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: timeout!", __func__);
    }

    data = in32(dwceqos->mac_base + MAC_MDIO_DATA);

    return (data & GD_MASK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static void dwceqos_mii_write(void *handle, uint8_t phy_id, uint8_t location, uint16_t value)
{
    dwceqos_dev_t  *dwceqos = (dwceqos_dev_t *)handle;
    uint32_t       addr;
    int            cnt;

    out32(dwceqos->mac_base + MAC_MDIO_DATA, (uint32_t)value);

    addr = (PA(phy_id) & PA_MASK) |
           (RDA(location) & RDA_MASK) |
           (PHY_CLOCK_RATE & CR_MASK) |
           GB |
           GOC_WRITE;

    out32(dwceqos->mac_base + MAC_MDIO_ADDR, addr);

    /* Wait for command to complete, it takes less than 128us  */
    cnt = MAX_MII_RW_TIMEOUT;
    while ((in32(dwceqos->mac_base + MAC_MDIO_ADDR) & GB) && cnt) {
        cnt--;
        nanospin_ns (1000);
    }

    if( cnt==0 ) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: timeout!", __func__);
    }
}

static void dwceqos_set_rx_control (dwceqos_dev_t *dwceqos, int enable)
{
    unsigned int reg;

    reg = in32(dwceqos->mac_base + MAC_RX_FLOW_CTRL);
    if (enable) {
        reg |= RFE;
    } else {
        reg &= ~RFE;
    }

    out32(dwceqos->mac_base + MAC_RX_FLOW_CTRL, reg);
}

static void dwceqos_set_tx_control (dwceqos_dev_t *dwceqos, int enable)
{
    uint32_t reg;

    /* MTL */
    reg = in32(dwceqos->mac_base + MTL_RXQi_OPERATION_MODE(0));
    if (enable) {
        reg |= EHFC;
    } else {
        reg &= ~EHFC;
    }

    out32(dwceqos->mac_base + MTL_RXQi_OPERATION_MODE(0), reg);

    /* MAC */
    reg = in32(dwceqos->mac_base + MAC_Qi_TX_FLOW_CTRL(0));
    if (enable) {
        reg |= TFE;
    } else {
        reg &= ~TFE;
    }

    out32(dwceqos->mac_base + MAC_Qi_TX_FLOW_CTRL(0), reg);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static void dwceqos_linkup(dwceqos_dev_t *dwceqos, uint32_t mode, uint32_t flow)
{
    const char      *s;
    nic_config_t    *cfg = &dwceqos->cfg;
    uint32_t        maccontrol;

    cfg->media_rate = -1;   /* Unknown */
    cfg->duplex = -1;       /* Unknown */

    switch (mode) {
        case MDI_10bT:
            s = "10BTHD";
            cfg->media_rate = 10*1000;
            cfg->duplex = 0;
            break;
        case MDI_10bTFD:
            s = "10BTFD";
            cfg->media_rate = 10*1000;
            cfg->duplex = 1;
            break;
        case MDI_100bT:
            s = "100BTHD";
            cfg->media_rate = 100*1000;
            cfg->duplex = 0;
            break;
        case MDI_100bTFD:
            s = "100BTFD";
            cfg->media_rate = 100*1000;
            cfg->duplex = 1;
            break;
        case MDI_100bT4:
            s = "100BT4";
            cfg->media_rate = 100*1000;
            cfg->duplex = 0;
            break;
        case MDI_1000bT:
            s = "1000 BaseT Half Duplex !!!NOT SUPPORTED!!!";
            cfg->duplex = 0;
            cfg->media_rate = 1000 * 1000L;
            break;
        case MDI_1000bTFD:
            s = "1000 BaseT Full Duplex";
            cfg->duplex = 1;
            cfg->media_rate = 1000 * 1000L;
            break;
        default:
            s = "Unknown";
            break;
    }

    if (flow & IFM_FLOW) {
        dwceqos_set_rx_control(dwceqos, 1);
        dwceqos_set_tx_control(dwceqos, 1);
    }

    if (flow & IFM_ETH_RXPAUSE) {
        dwceqos_set_rx_control(dwceqos, 1);
    } else {
        dwceqos_set_rx_control(dwceqos, 0);
    }

    if (flow & IFM_ETH_TXPAUSE) {
        dwceqos_set_tx_control(dwceqos, 1);
    } else {
        dwceqos_set_tx_control(dwceqos, 0);
    }

    if (!flow) {
        dwceqos_set_rx_control(dwceqos, 0);
        dwceqos_set_tx_control(dwceqos, 0);
    }

    /* Speed and duplex */
    if (cfg->media_rate != -1) {
        maccontrol = in32(dwceqos->mac_base + MAC_CFG);
        maccontrol &= ~(DM | PS | FES);

        if (cfg->duplex) {
            maccontrol |= DM;
        }

        if (cfg->media_rate == 10 * 1000L) {
            maccontrol |= PS;
        } else if (cfg->media_rate == 100 * 1000L) {
            maccontrol |= PS | FES;
        }

        out32(dwceqos->mac_base + MAC_CFG, maccontrol);

        if (cfg->verbose) {
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "devnp-dwceqos: Link up (%s) %s", __func__, s);
        }
    }
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static void dwceqos_mii_callback (void *handle, uchar_t phy, uchar_t newstate)
{
    dwceqos_dev_t   *dwceqos = (dwceqos_dev_t *)handle;
    nic_config_t    *cfg = &dwceqos->cfg;
    int             i, mode;
    struct ifnet    *ifp = &dwceqos->ecom.ec_if;
    uint32_t        advert, lpadvert;

    if (dwceqos->dying == 1) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: Dying", __func__);
        return;
    }

    switch (newstate) {
        case MDI_LINK_UP:
            if ((i = MDI_GetActiveMedia(dwceqos->mdi, phy, &mode)) != MDI_LINK_UP) {
                if (cfg->verbose) {
                    slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: callback GetActiveMedia returned %x", __func__, i);
                }
                mode = MDI_10bT;
            }

            advert = dwceqos_mii_read(dwceqos, phy, MDI_ANAR);
            lpadvert = dwceqos_mii_read(dwceqos, phy, MDI_ANLPAR);

            if (advert & MDI_FLOW) {
                if (lpadvert & MDI_FLOW) {
                    /* Enable Tx and Rx of Pause */
                    dwceqos->flow = IFM_FLOW;
                } else if ((advert & MDI_FLOW_ASYM) &&
                        (lpadvert & MDI_FLOW_ASYM)) {
                    /* Enable Rx of Pause */
                    dwceqos->flow = IFM_ETH_RXPAUSE;
                } else {
                    /* Disable all pause */
                    dwceqos->flow = 0;
                }
            } else if ((advert & MDI_FLOW_ASYM) &&
                    (lpadvert & MDI_FLOW) &&
                    (lpadvert & MDI_FLOW_ASYM)) {
                /* Enable Tx of Pause */
                dwceqos->flow = IFM_ETH_TXPAUSE;
            } else {
                /* Disable all pause */
                dwceqos->flow = 0;
            }

            dwceqos_linkup(dwceqos, mode, dwceqos->flow);

            if ((cfg->media_rate != -1) && (cfg->duplex != -1)) {
                cfg->flags &= ~NIC_FLAG_LINK_DOWN;
                if_link_state_change(ifp, LINK_STATE_UP);
            }
            break;

        case MDI_LINK_DOWN:
            dwceqos->flow = 0;
            cfg->media_rate = cfg->duplex = -1;
            cfg->flags |= NIC_FLAG_LINK_DOWN;
            MDI_AutoNegotiate(dwceqos->mdi, phy, NoWait);
            if_link_state_change(ifp, LINK_STATE_DOWN);
            break;

        default:
            break;
    }
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static void dwceqos_MDI_MonitorPhy (void *arg)
{
    dwceqos_dev_t    *dwceqos = arg;

    MDI_MonitorPhy(dwceqos->mdi);

    callout_msec(&dwceqos->mii_callout, 3 * 1000, dwceqos_MDI_MonitorPhy, arg);
}

void dwceqos_mdi_start_monitor(dwceqos_dev_t *dwceqos)
{
    MDI_EnableMonitor(dwceqos->mdi, 0);
    dwceqos_MDI_MonitorPhy(dwceqos);
}

void dwceqos_mdi_stop_monitor(dwceqos_dev_t *dwceqos)
{
    callout_stop(&dwceqos->mii_callout);
    MDI_DisableMonitor(dwceqos->mdi);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static void	dwceqos_setup_phy(dwceqos_dev_t *dwceqos)
{
    int             an_capable;
    nic_config_t    *cfg = &dwceqos->cfg;

    MDI_GetMediaCapable(dwceqos->mdi, cfg->phy_addr, &an_capable);

    if (an_capable) {
        if ((dwceqos->mdi->PhyData[cfg->phy_addr]->VendorOUI == KENDIN) &&
            (dwceqos->mdi->PhyData[cfg->phy_addr]->Model == KSZ9031)) {
                /* Bug in KSZ9031 PHY: Fails to autoneg with ASYM */
                an_capable |= MDI_FLOW;
        } else {
                an_capable |= MDI_FLOW | MDI_FLOW_ASYM;
        }
        MDI_PowerupPhy(dwceqos->mdi, cfg->phy_addr);
	nic_delay(5);
        MDI_EnableMonitor(dwceqos->mdi, 0);
        MDI_SetAdvert(dwceqos->mdi, cfg->phy_addr, an_capable);
        MDI_AutoNegotiate(dwceqos->mdi, cfg->phy_addr, NoWait);
        dwceqos_MDI_MonitorPhy(dwceqos);
    } else {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "devnp-dwceqos: %s, Phy auto negotiate not capable", __func__);
    }
    return;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void dwceqos_init_phy(dwceqos_dev_t *dwceqos)
{
    struct ifnet    *ifp = &dwceqos->ecom.ec_if;
    nic_config_t    *cfg = &dwceqos->cfg;

    MDI_Register_Extended(dwceqos, dwceqos_mii_write, dwceqos_mii_read,
        dwceqos_mii_callback, (mdi_t **)&dwceqos->mdi, NULL, 0, 0);

    for (dwceqos->cfg.phy_addr = 0; dwceqos->cfg.phy_addr < 32; dwceqos->cfg.phy_addr++) {
        if (MDI_FindPhy(dwceqos->mdi, dwceqos->cfg.phy_addr) == MDI_SUCCESS &&
            MDI_InitPhy(dwceqos->mdi, dwceqos->cfg.phy_addr) == MDI_SUCCESS) {

            if (cfg->verbose) {
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "devnp-dwceqos: %s: MII transceiver found at address %d.", __func__, dwceqos->cfg.phy_addr);
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "devnp-dwceqos: %s: Phy Type %s", __func__, dwceqos->mdi->PhyData[dwceqos->cfg.phy_addr]->Desc);
            }

            MDI_ResetPhy(dwceqos->mdi, cfg->phy_addr, WaitBusy);
            MDI_PowerdownPhy(dwceqos->mdi, cfg->phy_addr);
            cfg->flags |= NIC_FLAG_LINK_DOWN;
            if_link_state_change(ifp, LINK_STATE_DOWN);

            return dwceqos_setup_phy(dwceqos);
        }
    }

    if (cfg->verbose)
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: Can not locate MII transceiver", __func__);

    cfg->phy_addr = -1;
}

void dwceqos_fini_phy(dwceqos_dev_t *dwceqos)
{
    MDI_DeRegister(&dwceqos->mdi);
    dwceqos->mdi = NULL;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/dwceqos/mii.c $ $Rev: 906190 $")
#endif
