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
#include <net/ifdrvcom.h>
#include <sys/sockio.h>

static void dwceqos_get_stats (dwceqos_dev_t *dwceqos)
{
    nic_stats_t           *stats = &dwceqos->stats;
    nic_ethernet_stats_t  *estats = &dwceqos->stats.un.estats;
    uintptr_t             mac_base = dwceqos->mac_base;

    dwceqos->stats.media = NIC_MEDIA_802_3;
    dwceqos->stats.revision = NIC_STATS_REVISION;
    /* Generic networking stats we are interested in */
    dwceqos->stats.valid_stats =
        NIC_STAT_TX_FAILED_ALLOCS   |
        NIC_STAT_RX_FAILED_ALLOCS   |
        NIC_STAT_RXED_MULTICAST     |
        NIC_STAT_RXED_BROADCAST     |
        NIC_STAT_TXED_BROADCAST     |
        NIC_STAT_TXED_MULTICAST;
    /* Ethernet stats we are interested in */
    dwceqos->stats.un.estats.valid_stats =
        NIC_ETHER_STAT_ALIGN_ERRORS             |
        NIC_ETHER_STAT_SINGLE_COLLISIONS        |
        NIC_ETHER_STAT_MULTI_COLLISIONS         |
        NIC_ETHER_STAT_FCS_ERRORS               |
        NIC_ETHER_STAT_TX_DEFERRED              |
        NIC_ETHER_STAT_LATE_COLLISIONS          |
        NIC_ETHER_STAT_XCOLL_ABORTED            |
        NIC_ETHER_STAT_INTERNAL_TX_ERRORS       |
        NIC_ETHER_STAT_NO_CARRIER               |
        NIC_ETHER_STAT_INTERNAL_RX_ERRORS       |
        NIC_ETHER_STAT_EXCESSIVE_DEFERRALS      |
        NIC_ETHER_STAT_LENGTH_FIELD_MISMATCH    |
        NIC_ETHER_STAT_LENGTH_FIELD_OUTRANGE    |
        NIC_ETHER_STAT_OVERSIZED_PACKETS        |
        NIC_ETHER_STAT_JABBER_DETECTED          |
        NIC_ETHER_STAT_SHORT_PACKETS            |
        NIC_ETHER_STAT_TOTAL_COLLISION_FRAMES;

    /* Get status */
    stats->txed_ok = in32(mac_base + TX_PKT_COUNT_G); /* rxed_ok is in event */
    stats->octets_txed_ok = in32(mac_base + TX_OCTET_COUNT_G);
    stats->octets_rxed_ok = in32(mac_base + RX_OCTET_COUNT_G);
    stats->txed_multicast = in32(mac_base + TX_MULTICAST_PKTS_G);
    stats->rxed_multicast = in32(mac_base + RX_MULTICAST_PKTS_G);
    stats->txed_broadcast = in32(mac_base + TX_BROADCAST_PKTS_G);
    stats->rxed_broadcast = in32(mac_base + RX_BROADCAST_PKTS_G);

    /* Get estatus */
    estats->align_errors = in32(mac_base + RX_ALIGNMENT_ERROR_PKTS);
    estats->single_collisions = in32(mac_base + TX_SINGLE_COLLISION_G_PKTS);
    estats->multi_collisions = in32(mac_base + TX_MULTIPLE_COLLISION_G_PKTS);
    estats->fcs_errors = in32(mac_base + RX_CRC_ERROR_PKTS);
    estats->tx_deferred = in32(mac_base + TX_DEFERRED_PKTS);
    estats->late_collisions = in32(mac_base + TX_LATE_COLLISION_PKTS);
    estats->xcoll_aborted = in32(mac_base + TX_EXCESSIVE_COLLISION_PKTS);
    estats->internal_tx_errors = in32(mac_base + TX_UNDERFLOW_ERROR_PKTS);
    estats->no_carrier = in32(mac_base + TX_CARRIER_ERROR_PKTS);
    estats->internal_rx_errors = in32(mac_base + RX_FIFO_OVERFLOWPACKETS);
    estats->excessive_deferrals = in32(mac_base + TX_EXCESSIVE_DEFERRAL_ERROR);
    estats->length_field_outrange = in32(mac_base + RX_OUT_OF_RANGE_TYPE_PKTS);
    estats->length_field_mismatch = in32(mac_base + RX_LENGTH_ERROR_PKTS);
    estats->oversized_packets = in32(mac_base + RX_OVERSIZE_PKTS_G);
    /*sqe_errors symbol_errors*/
    estats->jabber_detected = in32(mac_base + RX_JABBER_ERROR_PKTS);
    estats->short_packets = in32(mac_base + RX_UNDERSIZE_PKTS_G);
}

static void dwceqos_set_mac_addr (dwceqos_dev_t *dwceqos, unsigned char *addr, unsigned int i)
{
    unsigned long data;

    data = (addr[5] << 8) | addr[4];
    out32(dwceqos->mac_base + MAC_ADDRi_HIGH(i), data | ADDRi_HIGH_AE);

    data = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
    out32(dwceqos->mac_base + MAC_ADDRi_LOW(i), data);
}

static void dwceqos_disable_mac_addr (dwceqos_dev_t *dwceqos, unsigned int i)
{
    /* 0 is the device's MAC address */
    if (i != 0) {
        out32(dwceqos->mac_base + MAC_ADDRi_HIGH(i), 0);
    }
}

static void dwceqos_set_multicast (dwceqos_dev_t *dwceqos)
{
    struct ethercom         *ec = &dwceqos->ecom;
    struct ifnet            *ifp = &ec->ec_if;
    struct ether_multi      *enm;
    struct ether_multistep  step;
    int                     i;
    uint32_t                reg;

    reg = in32(dwceqos->mac_base + MAC_PACKET_FILTER);
    reg &= ~PM;

    ifp->if_flags &= ~IFF_ALLMULTI;

    /*
     * clear all stored addresses, start from 1, 0 is device's address
     */
    for(i = 1; i < MAC_ADDR_TBL_SZ; i++) {
        dwceqos_disable_mac_addr(dwceqos, i);
    }

    ETHER_FIRST_MULTI(step, ec, enm);

    i = 1;
    while (enm != NULL) {
        if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
            /*
             * For now, just accept all multicasts
             */
            reg |= PM;
            ifp->if_flags |= IFF_ALLMULTI;
            break;
        }

        if (i >= MAC_ADDR_TBL_SZ) {
            reg |= PM;
            ifp->if_flags |= IFF_ALLMULTI;
            break;
        }

        dwceqos_set_mac_addr(dwceqos, enm->enm_addrlo, i);

        ETHER_NEXT_MULTI(step, enm);

        i++;
    }

    out32(dwceqos->mac_base + MAC_PACKET_FILTER, reg);

    return;
}

int dwceqos_ioctl (struct ifnet * ifp, unsigned long cmd, caddr_t data)
{
    int                   error = 0;
    dwceqos_dev_t         *dwceqos = ifp->if_softc;
    struct drvcom_config  *dcfgp;
    struct drvcom_stats   *dstp;
    struct ifdrv_com      *ifdc;

    switch (cmd) {
        case SIOCGDRVCOM:
            ifdc = (struct ifdrv_com *)data;
            switch (ifdc->ifdc_cmd) {
                case DRVCOM_CONFIG:
                    dcfgp = (struct drvcom_config *)ifdc;

                    if (ifdc->ifdc_len != sizeof(nic_config_t)) {
                        error = EINVAL;
                        break;
                    }

                    memcpy(&dcfgp->dcom_config, &dwceqos->cfg, sizeof(dwceqos->cfg));
                    break;

                case DRVCOM_STATS:
                    dstp = (struct drvcom_stats *)ifdc;

                    if (ifdc->ifdc_len != sizeof(nic_stats_t)) {
                        error = EINVAL;
                        break;
                    }

                    dwceqos_get_stats (dwceqos);
                    memcpy (&dstp->dcom_stats, &dwceqos->stats, sizeof(dwceqos->stats));
                    break;

                default:
                    error = ENOTTY;
            }
            break;

        case SIOCSIFMEDIA:
        case SIOCGIFMEDIA: {
            struct ifreq *ifr = (struct ifreq *)data;

            error = ifmedia_ioctl(ifp, ifr, &dwceqos->bsd_mii.mii_media, cmd);
            break;
            }

        default:
            error = ether_ioctl(ifp, cmd, data);
            if (error == ENETRESET) {

                /*
                 * Multicast list has changed; set the
                 * hardware filter accordingly.
                 */
                if ((ifp->if_flags_tx & IFF_RUNNING) == 0) {
                /*
                 * Interface is currently down: dwceqos_init()
                 * will call dwceqos_set_multicast() so
                 * nothing to do
                 */
                } else {
                    dwceqos_set_multicast(dwceqos);
                }

                error = 0;
            }
            break;
    }

    return error;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/dwceqos/devctl.c $ $Rev: 878224 $")
#endif
