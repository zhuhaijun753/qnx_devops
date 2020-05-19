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

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <stdio.h>
#include <string.h>

/************************************************************************/
/*                                                                      */
/************************************************************************/
void fdump (char *buffer, long cnt, FILE *fp)
{
    char *ptr, output [80];
    int  res, i, j, k, n;
    long l;
    int  skipping;
    char ref [80];

    l = 0L;
    res = cnt;
    ptr = buffer;
    skipping = 0;

    while (res) {
#define BL 16
#define AL BL/2*5+4
        sprintf (output, "%08lx:  %-*.*s\"%-*.*s\"", l, AL, AL, "  ", BL, BL, "  ");
        k = strlen (output) - (BL + 1);
        j = l % BL;
        i = 12 + (j << 1) + (j >> 1) + (j >= (BL / 2) ? 2 : 0);

        for (; j < BL && res; j++, res--, ptr++, l++) {
            n = ((int) *ptr >> 4) & 0xf;
            output [i++] = (n < 10 ? (char) n + '0': (char) n + ('a' - 10));
            n = (int) *ptr & 0xf;
            output [i++] = (n < 10 ? (char) n + '0': (char) n + ('a' - 10));

            if (j & 1) {
                i++;
            }

            if (j == BL / 2 - 1) {
                i += 2;
            }
#define UC unsigned char
            output [j + k] = ((UC) *ptr < ' ' || (UC) *ptr > '~') ? '.' : *ptr;
#undef UC
        }

        if (res && !strcmp (&ref [10], &output [10])) {
            if (!skipping) {
                fprintf (fp, "...\n");
            }
            skipping = 1;
        } else {
            fprintf (fp, "%s\n", output);
            skipping = 0;
        }

        memcpy (ref, output, 80);
#undef BL
#undef AL
    }

    fprintf (fp, "\n");
    fflush (fp);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void dwceqos_reap_pkts (dwceqos_dev_t *dwceqos)
{
    int             i, idx;
    dwceqos_desc_t  *tdesc;
    int             expect = dwceqos->tx_desc_num - dwceqos->tx_desc_avail;

    for(i = 0; i < expect; i++)
    {
        idx = (dwceqos->tx_desc_head + dwceqos->tx_desc_avail) % dwceqos->tx_desc_num;
        tdesc = &(dwceqos->tx_desc[idx]);
        if (tdesc->des3 & TDES3_OWN){
            break;
        }

        if (tdesc->m != NULL) {
            m_free(tdesc->m);
            tdesc->m = NULL;
        }

        tdesc->des0 = 0;
        tdesc->des1 = 0;
        tdesc->des2 = 0;
        tdesc->des3 = 0;

        dwceqos->tx_desc_avail++;
    }
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static struct mbuf* dwceqos_send_mbuf (struct ifnet *ifp, struct mbuf *mb)
{
    dwceqos_dev_t           *dwceqos = ifp->if_softc;
    struct mbuf             *m = mb, *m_temp;
    dwceqos_desc_t          *tdesc;

    dwceqos_reap_pkts(dwceqos);

    while (m && (dwceqos->tx_desc_avail > 0)) {
        if (!m->m_len) {
            m_temp = m;
            m = m->m_next;
            /* Have to free even it is 0 length */
            m_free(m_temp);
            continue;
        }

        tdesc = &(dwceqos->tx_desc[dwceqos->tx_desc_head]);
        dwceqos->tx_desc_head = (dwceqos->tx_desc_head + 1) % dwceqos->tx_desc_num;

        tdesc->m = m;
        tdesc->des0 = mbuf_phys(m);
        tdesc->des1 = 0;
        tdesc->des2 = m->m_len;
        tdesc->des3 = dwceqos->tq_pkt_len;

        /* First packet */
        if (dwceqos->tq_pkt_xbytes == 0) {
            tdesc->des3 |= TDES3_FD;
        }

        dwceqos->tq_pkt_xbytes += m->m_len;

        /* M_CSUM_IPv4 */
        if (m->m_pkthdr.csum_flags != 0) {
            tdesc->des3 |= DES_IPPPCSUM;
        }

        CACHE_FLUSH (&dwceqos->cachectl, m->m_data, mbuf_phys(m), m->m_len);

        if (dwceqos->cfg.verbose > 11) {
            fprintf(stderr, "send the pack len=%d\n", m->m_len);
            fdump (m->m_data, m->m_len, stderr);
        }

#if NBPFILTER > 0
        /* Pass the packet to any BPF listeners */
        if (ifp->if_bpf) {
            bpf_mtap (ifp->if_bpf, m);
        }
#endif
        m = m->m_next;

        /* Last packet */
        if (m == NULL || dwceqos->tq_pkt_xbytes >= dwceqos->tq_pkt_len) {
            ifp->if_opackets++;
            tdesc->des3 |= TDES3_LD;
        }

        dwceqos->tx_desc_avail--;
        dwceqos->stats.txed_ok++;

        __sync_synchronize();
        tdesc->des3 |= TDES3_OWN;
        __sync_synchronize();
        out32(dwceqos->mac_base + DMA_CHi_TXDESC_TAIL_PTR(0), (uintptr_t)dwceqos->tx_desc_tail);
    }

    return m;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void dwceqos_start (struct ifnet *ifp)
{
    dwceqos_dev_t           *dwceqos = ifp->if_softc;
    struct _iopkt_self      *iopkt = dwceqos->iopkt;
    struct nw_work_thread   *wtp = WTP;
    struct mbuf             *m;

    if ((ifp->if_flags_tx & IFF_RUNNING) == 0 ||
        (dwceqos->cfg.flags & NIC_FLAG_LINK_DOWN) != 0) {
        IFQ_PURGE(&ifp->if_snd);
        NW_SIGUNLOCK_P (&ifp->if_snd_ex, iopkt_selfp, wtp);
        return;
    }

    ifp->if_flags_tx |= IFF_OACTIVE;

    while (1) {
        if (!dwceqos->tq_mbuf) {
            IFQ_DEQUEUE(&ifp->if_snd, dwceqos->tq_mbuf);

            m = dwceqos->tq_mbuf;
            if (m) {
                dwceqos->tq_pkt_len = m->m_pkthdr.len;
                dwceqos->tq_pkt_xbytes = 0;
            }
        }

        m = dwceqos->tq_mbuf;

        if (m == NULL) {
            break;
        }

        /* Just part of the m is sent, let's come back from TX interrupt */
        dwceqos->tq_mbuf = dwceqos_send_mbuf(ifp, m);
        if (dwceqos->tq_mbuf) {
            NW_SIGUNLOCK_P(&ifp->if_snd_ex, iopkt, wtp);
            return;
        }
    }

    ifp->if_flags_tx &= ~IFF_OACTIVE;
    NW_SIGUNLOCK_P(&ifp->if_snd_ex, iopkt, wtp);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/dwceqos/transmit.c $ $Rev: 877965 $")
#endif
