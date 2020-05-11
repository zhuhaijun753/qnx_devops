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
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/ifdrvcom.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <device_qnx.h>
#include <sys/mman.h>
#include <sys/netmgr.h>
#include <sys/sockio.h>
#include <drvr/hwinfo.h>
#include <hw/sysinfo.h>
#include <variant.h>

static int dwceqos_entry(void *dll_hdl, struct _iopkt_self *iopkt, char *options);
static int dwceqos_attach(struct device *, struct device *, void *);
static int dwceqos_detach(struct device *, int);

struct _iopkt_drvr_entry IOPKT_DRVR_ENTRY_SYM(dwceqos) = IOPKT_DRVR_ENTRY_SYM_INIT(dwceqos_entry);

#ifdef VARIANT_a
#include <nw_dl.h>
/* This is what gets specified in the stack's dl.c */
struct nw_dll_syms dwceqos_syms[] = {
    {"iopkt_drvr_entry", &IOPKT_DRVR_ENTRY_SYM(dwceqos)},
    {NULL, NULL}
};
#endif

CFATTACH_DECL(dwceqos,
    sizeof(dwceqos_dev_t),
    NULL,
    dwceqos_attach,
    dwceqos_detach,
    NULL);

static char *dwceqos_opts[] = {
	"receive",
#define DWCOPT_RX	0
	"transmit",
#define DWCOPT_TX	1
        NULL
};

uint32_t        hw_feature1;
uint32_t        mtl_tx_fifo_sz;
uint32_t        mtl_rx_fifo_sz;
uint32_t        tqs,rqs;


/*****************************************************************************/
/* Parse HW syspage info                                                     */
/*****************************************************************************/
static int dwceqos_parse_hw_syspage (dwceqos_dev_t *dwceqos, nic_config_t *cfg)
{
    /* Getting parameters from the Hwinfo Section if available */
    unsigned hwi_off = hwi_find_device("dwc", 0);

     if (hwi_off != HWI_NULL_OFF) {
        unsigned  i = 0;
        hwi_tag  *tag;

        /* Find MAC base address */
        tag = hwi_tag_find(hwi_off, HWI_TAG_NAME_location, &i);
        if (tag) {
            dwceqos->iobase = tag->location.base;
        }

        /* Find IRQ */
        cfg->irq[0] = hwitag_find_ivec(hwi_off, NULL);

        /* Find MAC address */
        hwitag_find_nicaddr(hwi_off, NULL, cfg->current_address);
    } else {
        /* No device in syspage */
        return ENODEV;
    }

    return EOK;
}

/*****************************************************************************/
/* Parse command line options                                                */
/*****************************************************************************/
static int dwceqos_parse_options (dwceqos_dev_t *dwceqos, const char *optstring, nic_config_t *cfg)
{
    char   *value;
    int    opt;
    char   *options, *freeptr;
    char   *c;

    if (optstring == NULL) {
        return EOK;
    }

    /* getsubopt() is destructive */
    freeptr = options = strdup(optstring);

    while (options && *options != '\0') {
        c = options;
        opt = getsubopt(&options, dwceqos_opts, &value);

        switch (opt) {
            case DWCOPT_RX:
	        if (dwceqos != NULL) {
		    dwceqos->rx_desc_num = strtoul(value, 0, 0);
	        }
	        break;

            case DWCOPT_TX:
                if (dwceqos != NULL) {
		    dwceqos->tx_desc_num = strtoul(value, 0, 0);
	        }
	        break;

            default:
                /* Not one of ours, may be a generic driver option */
                if (nic_parse_options(cfg, value) != EOK) {
                    free(freeptr, M_DEVBUF);
                    slogf(_SLOGC_NETWORK, _SLOG_WARNING, "devnp-dwceqos: %s: Unknown option %s", __func__, c);
                    errno = EINVAL;
                    return -1;
                }
                break;
        }
    }

    free (freeptr, M_DEVBUF);
    return EOK;
}

static bool dweqos_tx_dma_complete(dwceqos_dev_t *dwceqos)
{
    uint32_t reg;

    reg = in32(dwceqos->mac_base + DMA_DEBUG_STS0);
    reg = DMA_GET_TX_STATE_CH0(reg);

    return (reg == DMA_TX_CH_SUSPENDED);
}

static void dwceqos_drain_dma(dwceqos_dev_t *dwceqos)
{
    /* Wait for all pending TX buffers to be sent.
     * 2ms is the maximum time for one packet transmission on 10Mbit link
     */
    size_t limit = dwceqos->tx_desc_num;

    while (!dweqos_tx_dma_complete(dwceqos) && limit--) {
        nic_delay(2);
    }
}

/****************************************************************************/
/* Reset Ethernet controller                                                */
/****************************************************************************/
static int dwceqos_reset (dwceqos_dev_t *dwceqos)
{
    int  cnt;

    /* Reset the chip */
    out32 (dwceqos->mac_base + DMA_MODE, SWR);

    /* Wait for it 0.5s to complete */
    cnt = 500;
    while (--cnt) {
        if ((in32(dwceqos->mac_base + DMA_MODE) & SWR) == 0) {
            break;
        }
        nic_delay(1);
    }

    if (cnt <= 0) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: Chip will not come out of reset", __func__);
        return ETIMEDOUT;
    }

    return EOK;
}

/*****************************************************************************/
/* Read MAC address                                                          */
/*****************************************************************************/
static void dwceqos_get_mac_addr (dwceqos_dev_t *dwceqos, uchar_t *addr)
{
    uint32_t mac_paddr;

    mac_paddr = in32(dwceqos->mac_base + MAC_ADDRi_HIGH(0));
    addr[5] = mac_paddr >> 8;
    addr[4] = mac_paddr;

    mac_paddr = in32(dwceqos->mac_base + MAC_ADDRi_LOW(0));

    /* Clear multicast bit for sanity's sake */
    addr[3] = (mac_paddr >> 24) & ~0x01;
    addr[2] = mac_paddr >> 16;
    addr[1] = mac_paddr >> 8;
    addr[0] = mac_paddr;
}

/*****************************************************************************/
/* Write MAC address to registers                                            */
/*****************************************************************************/
static void dwceqos_set_mac_addr (dwceqos_dev_t *dwceqos)
{
    union {
      int    dword;
      char   byte [4];
    } un;

    nic_config_t    *cfg = &dwceqos->cfg;

    un.dword = 0;
    un.byte[0] = cfg->current_address[4];
    un.byte[1] = cfg->current_address[5];
    out32 (dwceqos->mac_base + MAC_ADDRi_HIGH(0), un.dword);

    un.byte[0] = cfg->current_address[0];
    un.byte[1] = cfg->current_address[1];
    un.byte[2] = cfg->current_address[2];
    un.byte[3] = cfg->current_address[3];
    out32 (dwceqos->mac_base + MAC_ADDRi_LOW(0), un.dword);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
static void dwceqos_shutdown (void *arg)
{
    dwceqos_dev_t *dwceqos = (dwceqos_dev_t *)arg;

    /* Detach the interrupt handler */
    InterruptDetach (dwceqos->iid);

    /* Reset hardware to stop the DMA */
    dwceqos_reset (dwceqos);

    /* Set MAC address */
    dwceqos_set_mac_addr(dwceqos);
}

/****************************************************************************/
/* Stop transmission on the interface                                       */
/****************************************************************************/
static void dwceqos_stop (struct ifnet *ifp, int disable)
{
    dwceqos_dev_t          *dwceqos = ifp->if_softc;
    struct _iopkt_self     *iopkt = dwceqos->iopkt;
    struct nw_work_thread  *wtp = WTP;
    uint32_t               value, i;
    struct mbuf            *m;

    callout_stop(&dwceqos->mii_callout);

    /* Lock out the transmit side */
    NW_SIGLOCK_P (&ifp->if_snd_ex, iopkt, wtp);

    /* Mark the interface as down */
    ifp->if_flags_tx &= ~(IFF_RUNNING | IFF_OACTIVE);
    ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

    /* Done with the transmit lock */
    NW_SIGUNLOCK_P (&ifp->if_snd_ex, iopkt, wtp);

    /* Shutdown MAC Rx/Tx */
    value = in32(dwceqos->mac_base + MAC_CFG);
    value &= ~(RE | TE);
    out32(dwceqos->mac_base + MAC_CFG, value);

    /* Wait for DMA to complete */
    dwceqos_drain_dma(dwceqos);

    /* Release transmission queued mbuf */
    if (dwceqos->tq_mbuf) {
        m_freem(dwceqos->tq_mbuf);
        dwceqos->tq_mbuf = NULL;
    }

    /* Release Tx mbuf and reset Tx descriptors */
    for (i = 0; i < dwceqos->tx_desc_num; i++) {
        dwceqos->tx_desc[i].des0 = 0;
        dwceqos->tx_desc[i].des1 = 0;
        dwceqos->tx_desc[i].des2 = 0;
        dwceqos->tx_desc[i].des3 = 0;

        if ((m = dwceqos->tx_desc[i].m) != NULL) {
            m_free (m);
            dwceqos->tx_desc[i].m = NULL;
        }
    }

    IFQ_PURGE(&ifp->if_snd);

    /* Power down PHY and change link to down */
    MDI_PowerdownPhy(dwceqos->mdi, dwceqos->cfg.phy_addr);
    if_link_state_change(ifp, LINK_STATE_DOWN);

    /* Disable PHY monitor */
    MDI_MonitorPhy(dwceqos->mdi);
    MDI_DisableMonitor(dwceqos->mdi);
}

/*****************************************************************************/
/* MAC Initialization                                                        */
/*****************************************************************************/
int dwceqos_init (struct ifnet *ifp)
{
    dwceqos_dev_t           *dwceqos;
    struct nw_work_thread   *wtp;
    struct _iopkt_self      *iopkt;
    uint32_t                cfg;
    uint32_t                value;

    dwceqos = ifp->if_softc;
    iopkt = dwceqos->iopkt;
    wtp = WTP;

    if (dwceqos->dying == 1) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: Dying", __func__);
        return EOK;
    }

    /* MAC address */
    memcpy(dwceqos->cfg.current_address, LLADDR(ifp->if_sadl), ifp->if_addrlen);
    dwceqos_set_mac_addr(dwceqos);

    /* Promiscuous mode if needed */
    cfg = in32(dwceqos->mac_base + MAC_PACKET_FILTER);
    if (ifp->if_flags & IFF_PROMISC) {
        cfg |= PR;
    } else {
        cfg &= ~PR;
    }

    out32(dwceqos->mac_base + MAC_PACKET_FILTER, cfg);

    /* Get MTU from stack for nicinfo */
    dwceqos->cfg.mtu = ifp->if_mtu;
    dwceqos->cfg.mru = ifp->if_mtu;

    /* Lock out the transmit */
    NW_SIGLOCK_P(&ifp->if_snd_ex, iopkt, wtp);

    if ((ifp->if_flags & IFF_RUNNING) == 0) {
        MDI_PowerupPhy(dwceqos->mdi, dwceqos->cfg.phy_addr);
        dwceqos_mdi_start_monitor(dwceqos);

        /* Start MAC RX and TX */
        value = in32(dwceqos->mac_base + MAC_CFG);
        value |= (RE | TE);
        out32(dwceqos->mac_base + MAC_CFG, value);

        /* Set interface flags */
        ifp->if_flags_tx |= IFF_RUNNING;
        ifp->if_flags |= IFF_RUNNING;
    }

    /* Unlock the transmit */
    NW_SIGUNLOCK_P(&ifp->if_snd_ex, iopkt, wtp);

    return EOK;
}

/*****************************************************************************/
/* Convert virtual to physical address                                       */
/*****************************************************************************/
static paddr_t vtophys (void *addr)
{
    off64_t  offset;

    if (mem_offset64 (addr, NOFD, 1, &offset, 0) == -1) {
        return (-1);
    }
    return (offset);
}

/*****************************************************************************/
/* DMA initialization                                                        */
/*****************************************************************************/
static int dwceqos_dma_init (dwceqos_dev_t *dwceqos)
{
    int                     i = 0;
    int                     size = sizeof (dwceqos_desc_t);
    uint32_t                reg, bw, dma_skip;
    struct nw_work_thread   *wtp = WTP;
    struct mbuf             *m;
    off64_t                 phys;
    int                     pbl;
    dwceqos_desc_t          *tx_desc_phys, *rx_desc_phys;

    /* Allocate descriptors */
    dwceqos->descs = mmap (NULL, sizeof (dwceqos_desc_t) * (dwceqos->tx_desc_num + dwceqos->rx_desc_num),
                           PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_ANON | MAP_PHYS | MAP_SHARED,
                           NOFD, 0);

    if (dwceqos->descs == MAP_FAILED) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: Descriptor memory mmap failed: %s(%d)", __func__, strerror(errno), errno);
        return ENOBUFS;
    }

    memset((char *)dwceqos->descs, 0, size * (dwceqos->tx_desc_num + dwceqos->rx_desc_num));

    dwceqos->tx_desc = dwceqos->descs;
    dwceqos->rx_desc = (dwceqos->descs + dwceqos->tx_desc_num);
    dwceqos->tx_desc_avail = dwceqos->tx_desc_num;

    dwceqos->tx_desc_head = 0;
    dwceqos->rx_desc_head = 0;

    tx_desc_phys = (dwceqos_desc_t *)vtophys((void *)dwceqos->tx_desc);
    rx_desc_phys = (dwceqos_desc_t *)vtophys((void *)dwceqos->rx_desc);
    dwceqos->tx_desc_tail = rx_desc_phys;
    dwceqos->rx_desc_tail = rx_desc_phys + dwceqos->rx_desc_num;

    /* Pre-allocate a receive buffer for each receive descriptor */
    for (i = 0; i < dwceqos->rx_desc_num; i++) {
        m = m_getcl_wtp (M_DONTWAIT, MT_DATA, M_PKTHDR, wtp);
        if (m == NULL) {
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: m_getcl_wtp failed", __func__);
            return ENOBUFS;
        }

        phys = mbuf_phys (m);
        CACHE_INVAL (&dwceqos->cachectl, m->m_data, phys, m->m_ext.ext_size);

        /* Let DMA controller own the Rx Descriptors */
        dwceqos->rx_desc[i].des0 = phys;
        dwceqos->rx_desc[i].des1 = 0;
        dwceqos->rx_desc[i].des2 = 0;
        dwceqos->rx_desc[i].des3 = (RDES3_OWN | RDES3_BUF1V | RDES3_IOC);
        dwceqos->rx_desc[i].m = m;
    }

    /* Probe data bus width, 32/64/128 bits. */
    out32(dwceqos->mac_base + DMA_CHi_TXDESC_TAIL_PTR(0), 0xF);
    reg = in32(dwceqos->mac_base + DMA_CHi_TXDESC_TAIL_PTR(0));
    bw = (reg ^ 0xF) + 1;

    /* Cache-align dma descriptors */
    /* set Descriptor Skip Length to skip between two unchained descriptors */
    dma_skip = (sizeof(dwceqos_desc_t) - 16) / bw;
    out32(dwceqos->mac_base + DMA_CHi_CTRL(0), (DSL_SHIFT(dma_skip) & DSL_MASK) | PBLX8);

    /* Setup DMA descriptor ring */
    out32(dwceqos->mac_base + DMA_CHi_TXDESC_RING_LEN(0), dwceqos->tx_desc_num - 1);
    out32(dwceqos->mac_base + DMA_CHi_RXDESC_RING_LEN(0), dwceqos->rx_desc_num - 1);
    out32(dwceqos->mac_base + DMA_CHi_RXDESC_LIST_ADDR(0), (uintptr_t)rx_desc_phys);
    out32(dwceqos->mac_base + DMA_CHi_TXDESC_LIST_ADDR(0), (uintptr_t)tx_desc_phys);
    out32(dwceqos->mac_base + DMA_CHi_RXDESC_TAIL_PTR(0), (uintptr_t)dwceqos->rx_desc_tail);

    dwceqos->tq_mbuf = NULL;

    pbl = tqs + 1;
    if (pbl > 32) {
        pbl = 32;
    }
    /* Set transmit programmable burst length */
    out32(dwceqos->mac_base + DMA_CHi_TX_CTRL(0), (TXPBL_MASK & TXPBL(pbl)));

    /* Set receive programmable burst length */
    out32(dwceqos->mac_base + DMA_CHi_RX_CTRL(0), (RXPBL_MASK & RXPBL(8)) | (RBSZ_MASK & RBSZ(RX_BUF_SIZE)));

    /* Enable Enhanced Address Mode and set AXI Burst Length */
    reg = EAME | BLEN16 | BLEN8 | BLEN4;
    out32(dwceqos->mac_base + DMA_SYSBUS_MODE, reg);

    /* Start DMA Rx */
    reg = in32(dwceqos->mac_base + DMA_CHi_RX_CTRL(0));
    out32(dwceqos->mac_base + DMA_CHi_RX_CTRL(0), reg | SR);

    /* Start DMA Tx */
    reg = in32(dwceqos->mac_base + DMA_CHi_TX_CTRL(0));
    out32(dwceqos->mac_base + DMA_CHi_TX_CTRL(0), reg | ST);

    /* Clear DMA intr status */
    out32 (dwceqos->mac_base + DMA_CHi_STATUS(0), 0xFFFF);

    return EOK;
}

/*****************************************************************************/
/* Detach                                                                    */
/*****************************************************************************/
static int dwceqos_detach (struct device *dev, int how)
{
    struct ifnet    *ifp;
    struct mbuf     *m;
    uint32_t        i, size, value;
    dwceqos_dev_t   *dwceqos;
    struct _iopkt_self     *iopkt;
    struct nw_work_thread  *wtp = WTP;

    dwceqos = (dwceqos_dev_t *)dev;
    ifp = &dwceqos->ecom.ec_if;
    iopkt = dwceqos->iopkt;

    /* Don't init() while we're dying */
    dwceqos->dying = 1;

    /* Stop any callout that may be running */
    callout_stop(&dwceqos->mii_callout);

    /* Lock out the transmit side */
    NW_SIGLOCK_P (&ifp->if_snd_ex, iopkt, wtp);

    /* Mark the interface as down */
    ifp->if_flags_tx &= ~(IFF_RUNNING | IFF_OACTIVE);
    ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

    /* Done with the transmit lock */
    NW_SIGUNLOCK_P (&ifp->if_snd_ex, iopkt, wtp);

    /* Shutdown MAC Rx/Tx */
    value = in32(dwceqos->mac_base + MAC_CFG);
    value &= ~(RE | TE);
    out32(dwceqos->mac_base + MAC_CFG, value);

    /* Wait for DMA to complete */
    dwceqos_drain_dma(dwceqos);

    /* Detach the interrupt handler */
    if (dwceqos->iid != -1) {
        InterruptDetach (dwceqos->iid);
        dwceqos->iid = -1;
    }

    /* Remove interrupt worker from io-pkt */
    interrupt_entry_remove (&dwceqos->inter, NULL);

    /* Reset hardware to stop the DMA */
    dwceqos_reset (dwceqos);

    /* Set MAC address, otherwise we lose it when remount the driver */
    dwceqos_set_mac_addr(dwceqos);

    /* Unhook from io-pkt, will stop anything calling Tx */
    shutdownhook_disestablish(dwceqos->sd_hook);
    ether_ifdetach(ifp);
    if_detach(ifp);

    /* Media and PHY fini */
    bsd_mii_finimedia(dwceqos);
    dwceqos_fini_phy(dwceqos);

    /* Release transmission queued mbuf */
    if (dwceqos->tq_mbuf) {
        m_freem(dwceqos->tq_mbuf);
        dwceqos->tq_mbuf = NULL;
    }

    /* Release Tx mbuf */
    for (i = 0; i < dwceqos->tx_desc_num; i++) {
        if ((m = dwceqos->tx_desc[i].m) != NULL) {
            m_free (m);
            dwceqos->tx_desc[i].m = NULL;
        }
    }

    /* Release Rx mbuf */
    for (i = 0; i < dwceqos->rx_desc_num; i++) {
        if ((m = dwceqos->rx_desc[i].m) != NULL) {
            m_free (m);
            dwceqos->rx_desc[i].m = NULL;
        }
    }

    /* Free Tx and Rx descriptors */
    if (dwceqos->descs != MAP_FAILED) {
        size = sizeof (dwceqos_desc_t);
        munmap((void*)dwceqos->rx_desc, size * (dwceqos->rx_desc_num + dwceqos->tx_desc_num));
    }

    /* Done with cache control */
    cache_fini (&dwceqos->cachectl);

    /* Unmap MAC registers */
    if (dwceqos->mac_base != (uintptr_t)MAP_FAILED) {
        munmap_device_io (dwceqos->mac_base, ENET_SIZE);
    }

    return EOK;
}

/*****************************************************************************/
/* Attach                                                                    */
/*****************************************************************************/
static int dwceqos_attach (struct device *parent, struct device *self, void *aux)
{
    dwceqos_dev_t          *dwceqos;
    struct ifnet           *ifp;
    nic_config_t           *cfg;
    char                   *options;
    int                    err;
    const char             zaddr[6] = {0,};
    uint32_t               reg;
    uintptr_t              mac_base;

#ifdef S32G_FLEXCAN
    if (ThreadCtl(_NTO_TCTL_IO_PRIV, 0) == -1) {
	slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s Setting thread proprity failed", __func__);
	return EXIT_FAILURE;
    }
#endif
    dwceqos = (dwceqos_dev_t *)self;
    dwceqos->iopkt = iopkt_selfp;

    /* Setup ethercomm */
    ifp = &dwceqos->ecom.ec_if;
    ifp->if_softc = dwceqos;

    /* Set the configuration of nic */
    cfg = &dwceqos->cfg;
    cfg->revision = NIC_CONFIG_REVISION;
    cfg->flags |= NIC_FLAG_MULTICAST;
    dwceqos->rx_desc_num = DEFAULT_NUM_RX_DESCRIPTORS;
    dwceqos->tx_desc_num = DEFAULT_NUM_TX_DESCRIPTORS;
    cfg->mtu = ETHERMTU;
    cfg->mru = ETHERMTU;
    cfg->lan = dwceqos->dev.dv_unit;
    cfg->mac_length = ETHER_ADDR_LEN;
    cfg->connector = NIC_CONNECTOR_MII;
    cfg->media_rate = cfg->duplex = -1;

    strcpy(ifp->if_xname, self->dv_xname);
    strlcpy((char *)cfg->device_description, "dwceqos", sizeof(cfg->device_description));
    strlcpy((char *) cfg->uptype, "en", sizeof(cfg->uptype));

    /* Get IO base and IRQ from HW syspage */
    err = dwceqos_parse_hw_syspage(dwceqos, cfg);
    if (err != EOK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: dwceqos_parse_hw_syspage() failed", __func__);
        return err;
    }

    /* Parse command line options */
    options = (char *)aux;
    if (dwceqos_parse_options (dwceqos, options, cfg) == -1) {
        err = errno;
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: dwceqos_parse_options() failed", __func__);
        goto _fail;
    }

    cfg->media = NIC_MEDIA_802_3;
    cfg->num_irqs = 1;
    cfg->num_mem_windows = 1;
    cfg->mem_window_base[0] = dwceqos->iobase;
    cfg->mem_window_size[0] = ENET_SIZE;
    cfg->iftype = IFT_ETHER;
    cfg->priority = IRUPT_PRIO_DEFAULT;

    callout_init(&dwceqos->mii_callout);

    /* check if specify either of speed or duplex on the command line */
    if ((cfg->media_rate != -1) || (cfg->duplex != -1)) {
	if ((cfg->media_rate == -1) || (cfg->duplex == -1)) {
	    slogf(_SLOGC_NETWORK, _SLOG_ERROR,
		  "devnp-dwceqos: must also specify speed when duplex is specified");
	    err = EINVAL;
            goto _fail;
	}
    }

    /* Map registers */
    mac_base = mmap_device_io(cfg->mem_window_size[0], cfg->mem_window_base[0]);
    if (mac_base == MAP_DEVICE_FAILED) {
        err = errno;
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: mmap_device_io() for dwceqos mac failed", __func__);
        goto _fail;
    }
    dwceqos->mac_base = mac_base;

    /* Default MAC address to current ENET hardware setting (comes from boot loader on first boot) */
    dwceqos_get_mac_addr(dwceqos, cfg->permanent_address);

    /* check for command line or hwinfo override */
    if (memcmp (cfg->current_address, zaddr, 6) == 0) {
        /* check if the default mac was set */
        if (memcmp (cfg->permanent_address, zaddr, ETHER_ADDR_LEN) == 0) {
            /* Don't start the driver if there is no valid MAC address available */
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: MAC address not specified on cmdline.", __func__);
            err = EINVAL;
            goto _fail;
        } else {
            /* default mac override */
            memcpy (cfg->current_address, cfg->permanent_address, ETHER_ADDR_LEN);
        }
    }

    /* Hook up so media devctls work */
    bsd_mii_initmedia(dwceqos);

    /* Setup ethercomm */
    ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
    ifp->if_ioctl = dwceqos_ioctl;
    ifp->if_start = dwceqos_start;
    ifp->if_init = dwceqos_init;
    ifp->if_stop = dwceqos_stop;
    IFQ_SET_READY (&ifp->if_snd);

    /* Set checksum capabilities */
    reg = in32(mac_base + MAC_HW_FEATURE0);
    if (reg & TXCOESEL) {
        ifp->if_capabilities_tx = IFCAP_CSUM_IPv4 | IFCAP_CSUM_TCPv4 |
                                  IFCAP_CSUM_UDPv4 | IFCAP_CSUM_TCPv6 |
                                  IFCAP_CSUM_UDPv6;
    }
    if (reg & RXCOESEL) {
        ifp->if_capabilities_rx = IFCAP_CSUM_IPv4 | IFCAP_CSUM_TCPv4 |
                                  IFCAP_CSUM_UDPv4 | IFCAP_CSUM_TCPv6 |
                                  IFCAP_CSUM_UDPv6;
    }

    /* Setup interrupt related info */
    dwceqos->inter.func = dwceqos_process_interrupt;
    dwceqos->inter.enable = dwceqos_enable_interrupt;
    dwceqos->isrp = dwceqos_isr;
    dwceqos->inter.arg = dwceqos;
    dwceqos->iid = -1; /* Not yet attached */

    if ((err = interrupt_entry_init (&dwceqos->inter, 0, NULL, IRUPT_PRIO_DEFAULT)) != EOK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: interrupt_entry_init failed", __func__);
        goto _fail;
    }

    /* Cache init */
    if (cache_init (0, &dwceqos->cachectl, NULL) == -1) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: cache_init() failed", __func__);
        err = errno;
        goto _fail;
    }

    /* Reset the chip */
    err = dwceqos_reset(dwceqos);
    if (err != EOK) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "devnp-dwceqos: %s: dwceqos_parse_options failed", __func__);
        goto _fail;
    }

    /* CRC */
    out32(mac_base + MAC_CFG, ACS | CST);

    /* Clear interrupts, make sure there is no interrupt */
    out32(mac_base + DMA_CHi_INTR_EN(0), 0);
    out32(mac_base + MAC_INTR_ENABLE, 0);

#ifndef S32G_FLEXCAN
    /* Get HW_FEATURE1 value and MTL TX/RX FIFO size */
    hw_feature1 = in32(mac_base + MAC_HW_FEATURE1);
    mtl_tx_fifo_sz = (hw_feature1 & TXFIFOSIZE_MASK) >> TXFIFOSIZE_SHIF;
    mtl_rx_fifo_sz = (hw_feature1 & RXFIFOSIZE_MASK) >> RXFIFOSIZE_SHIF;

    /*
     * r/tx_fifo_sz is encoded as log2(n / 128). Undo that by shifting.
     * r/tqs is encoded as (n / 256) - 1.
     */
     tqs = (128 << mtl_tx_fifo_sz) / 256 - 1;
     rqs = (128 << mtl_rx_fifo_sz) / 256 - 1;

#else
    /* FIFO queue size is reported as 32KB, but actual size is 20KB. Hence tqs & rqs are calculated differently */
    rqs = tqs = MTL_MEMORY_SIZE / 256 - 1;
#endif

    /* Initialize DMA */
    if ((err = dwceqos_dma_init(dwceqos)) != EOK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: dwceqos_dma_init failed", __func__);
        goto _fail;
    }

    /* Initialize the queue */
    /* Pause time */
    out32(mac_base + MAC_Qi_TX_FLOW_CTRL(0), (PT_SHIFT(256) & PT_MASK) | TFE);
    out32(mac_base + MAC_RX_FLOW_CTRL, RFE);

    /* Enable receive queue */
    out32(mac_base + MAC_RXQ_CTRL0, 2 & RXQ0EN_MASK);

    /* Initialize MTL */
    out32(mac_base + MTL_TXQi_OPERATION_MODE(0), (TQS(tqs) & TQS_MASK) | TXQEN(TX_EN) | TSF | EHFC);
    out32(mac_base + MTL_TXQi_QUANTUM_WEIGHT(0), 0x10);
    out32(mac_base + MTL_RXQi_OPERATION_MODE(0), (RQS(rqs) & RQS_MASK) | RSF | FEP);

    /* Initialize PHY */
    dwceqos_init_phy(dwceqos);

    /* Attach the interrupt handler */
    if ((dwceqos->iid = InterruptAttach (cfg->irq[0], dwceqos->isrp, dwceqos, 0, _NTO_INTR_FLAGS_TRK_MSK)) == -1) {
          slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: InterruptAttach failed: %s", __func__, strerror(errno));
        goto _fail;
    }

    /* Enable the DMA RX/TX interrupts */
    out32(dwceqos->mac_base + DMA_CHi_INTR_EN(0),(NIE| RIE | TBUE | FBEE));

   if (cfg->verbose) {
        nic_dump_config (cfg);
    }

    /* Attach interface */
    if_attach(ifp);

    ether_ifattach(ifp, dwceqos->cfg.current_address);

    /* Hook up shutdown */
    if ((dwceqos->sd_hook = shutdownhook_establish(dwceqos_shutdown, dwceqos)) == NULL) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: shutdownhook_establish() failed", __func__);
        return ENOMEM;
    }
    return EOK;

_fail:
    slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: attach failed: %s", __func__, strerror(errno));

    dwceqos_detach((struct device *)dwceqos, 0);

    return err;
}

/*****************************************************************************/
/* Detect if driver is already mounted                                       */
/* Returns EOK if driver is not mounted yet                                  */
/*****************************************************************************/
static int dwceqos_detect(void)
{
    int                   ret;
    struct ifnet          *ifp;
    struct drvcom_config  *dcon;

    /* Check if driver is already mounted by doing a "nicinfo" on each interface */
    dcon = (malloc)(sizeof(*dcon));
    if (dcon == NULL) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: %d: malloc failed", __func__, __LINE__);
        return ENOMEM;
    }

    IFNET_FOREACH(ifp) {
        dcon->dcom_cmd.ifdc_cmd = DRVCOM_CONFIG;
        dcon->dcom_cmd.ifdc_len = sizeof(dcon->dcom_config);
        ret = ifp->if_ioctl(ifp, SIOCGDRVCOM, (caddr_t)dcon);
        if (ret == EOK) {
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: Driver already loaded for %s", __func__, ifp->if_xname);
            (free)(dcon);
            return EBUSY;
        }
    }

    (free)(dcon);

    return EOK;
}

/*****************************************************************************/
/* Initial driver entry point.                                               */
/* Returns -1 on failure; errno will be set to ENODEV if no devices detected.*/
/*****************************************************************************/
int dwceqos_entry (void *dll_hdl, struct _iopkt_self *iopkt, char *options)
{
    int            single = 0;
    struct device  *dev = NULL;

    if (dwceqos_detect() != EOK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: Driver already loaded!", __func__);
        return ENODEV;
    }

    if (dev_attach("dwc", options, &dwceqos_ca, options, &single, &dev, NULL) != EOK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devnp-dwceqos: %s: dev_attach failed", __func__);
        return ENODEV;
    }

    dev->dv_dll_hdl = dll_hdl;

    return EOK;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/dwceqos/dwceqos.c $ $Rev: 905611 $")
#endif
