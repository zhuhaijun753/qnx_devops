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

#include "bpfilter.h"
#include <dwceqos.h>
#include <netinet/in.h>
#include <net/if_vlanvar.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <stdio.h>
#include <string.h>

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static int dwceqos_receive (dwceqos_dev_t *dwceqos, struct nw_work_thread *wtp)
{
    struct mbuf     *new, *m;
    int             pkt_len;
    struct ifnet    *ifp;
    dwceqos_desc_t  *rdesc;
    uint32_t        rdes1;
    uint32_t        mtu;
    struct ether_vlan_header    *vlan_hdr;

    ifp = &dwceqos->ecom.ec_if;

    while (1) {
        rdesc = &(dwceqos->rx_desc[dwceqos->rx_desc_head]);
        if (rdesc->des3 & RDES3_OWN) {
            break;
        }

        dwceqos->rx_desc_head = (dwceqos->rx_desc_head + 1) % dwceqos->rx_desc_num;

        pkt_len = rdesc->des3 & RDES3_PL_MASK;

        mtu = ifp->if_mtu + ETHER_HDR_LEN;

        /* Extra bytes for VLAN packet */
        vlan_hdr = mtod(rdesc->m, struct ether_vlan_header*);
        if (ntohs(vlan_hdr->evl_encap_proto) == ETHERTYPE_VLAN) {
            mtu += ETHER_VLAN_ENCAP_LEN;
        }

        /* Drop the packet, reinitialize the desc */
        if ((rdesc->des3 & RDES3_ES) || (pkt_len > mtu)) {
            rdesc->des0 = mbuf_phys(rdesc->m);
            rdesc->des1 = 0;
            rdesc->des2 = 0;
            __sync_synchronize();
            rdesc->des3 = (RDES3_OWN | RDES3_BUF1V | RDES3_IOC);
            __sync_synchronize();
            ifp->if_ierrors++;
            continue;
        }

        /* Get a new mbuf for the desc */
        new = m_getcl_wtp (M_DONTWAIT, MT_DATA, M_PKTHDR, wtp);
        if (new == NULL) {
            if (dwceqos->cfg.verbose) {
                slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: Line %d m_getcl_wtp returned NULL", __func__, __LINE__);
            }

            ifp->if_ierrors++;
            dwceqos->stats.rx_failed_allocs++;
            break;
        }

        CACHE_INVAL (&dwceqos->cachectl, new->m_data, mbuf_phys(new), new->m_ext.ext_size);

        m = rdesc->m;
        rdesc->m = new;
        rdes1 = rdesc->des1;

        rdesc->des0 = mbuf_phys(new);
        rdesc->des1 = 0;
        rdesc->des2 = 0;
        __sync_synchronize();
        rdesc->des3 = (RDES3_OWN | RDES3_BUF1V | RDES3_IOC);
        __sync_synchronize();

        m->m_pkthdr.len = pkt_len;
        m->m_len = pkt_len;
        m->m_pkthdr.rcvif = ifp;

        /* Get the check sum error */
        if ((rdes1 & RDES1_IPCB) == 0) {
            if (rdes1 & RDES1_IPCB) {
                m->m_pkthdr.csum_flags |= M_CSUM_IPv4_BAD;
            }

            if (rdes1 & RDES1_IPCE) {
                if ((rdes1 & DES_PT_MASK) == DES_PT_UDP) {
                    m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
                }
                if ((rdes1 & DES_PT_MASK) == DES_PT_TCP) {
                    m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
                }
            }
        }

        CACHE_INVAL(&dwceqos->cachectl, m->m_data, mbuf_phys(m), pkt_len);

#if NBPFILTER > 0
        /* Pass this up to any BPF listeners. */
        if (ifp->if_bpf) {
            bpf_mtap (ifp->if_bpf, m);
        }
#endif
        dwceqos->stats.rxed_ok++;
        ifp->if_ipackets++;
        (*ifp->if_input)(ifp, m);
    }

    /* DMA maybe in suspect mode, poll to wake it */
    out32(dwceqos->mac_base + DMA_CHi_RXDESC_TAIL_PTR(0), (uintptr_t)dwceqos->rx_desc_tail);

    return EOK;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int dwceqos_process_interrupt (void *arg, struct nw_work_thread *wtp)
{
    dwceqos_dev_t   *dwceqos = arg;
    struct ifnet    *ifp;
    uint32_t        status;
    uintptr_t       mac_base;

    ifp = &dwceqos->ecom.ec_if;
    mac_base = dwceqos->mac_base;

    /* Fetch and clear current DMA channel interrupt status */
    status = in32(mac_base + DMA_CHi_STATUS(0));

    do {

        if (status & RI) {
            out32 (mac_base + DMA_CHi_STATUS(0), RI);
            dwceqos_receive (dwceqos, wtp);
        }

        if (status & TBU) {
            out32(mac_base + DMA_CHi_STATUS(0), TBU);
            /* If out of Tx descriptors call start to reap and Tx more */
            NW_SIGLOCK_P(&ifp->if_snd_ex, dwceqos->iopkt, wtp);
            if (ifp->if_flags_tx & IFF_OACTIVE) {
                dwceqos_start(ifp);
            } else {
                dwceqos_reap_pkts (dwceqos);
                NW_SIGUNLOCK_P(&ifp->if_snd_ex, dwceqos->iopkt, wtp);
            }
        }

        if (status & FBE) {
            out32(mac_base + DMA_CHi_STATUS(0), REB | TEB);
        }

        status = in32(mac_base + DMA_CHi_STATUS(0));
    } while (status & (TBU | RI));

    /* Clean other status */
    out32(mac_base + DMA_CHi_STATUS(0), status);

    return 1;
}

/**************************************************************************/
/*                                                                        */
/**************************************************************************/
int dwceqos_enable_interrupt (void *arg)
{
    dwceqos_dev_t  *dwceqos = arg;

    /* Re-enable interrupt enables */
    InterruptUnmask(dwceqos->cfg.irq[0], dwceqos->iid);
    return 1;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
const struct sigevent *dwceqos_isr (void *arg, int iid)
{
    dwceqos_dev_t         *dwceqos = arg;
    struct _iopkt_inter   *ient = &dwceqos->inter;

    /* Mask all dma interrupts */
    InterruptMask(dwceqos->cfg.irq[0], dwceqos->iid);

    return interrupt_queue (dwceqos->iopkt, ient);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/dwceqos/event.c $ $Rev: 878224 $")
#endif
