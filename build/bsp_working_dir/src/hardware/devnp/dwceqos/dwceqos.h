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

#ifndef    __DWCEQOS_H__
#define    __DWCEQOS_H__

#include <io-pkt/iopkt_driver.h>
#include <stdio.h>
#include <errno.h>
#include <atomic.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/siginfo.h>
#include <sys/syspage.h>
#include <sys/neutrino.h>
#include <sys/mbuf.h>
#include <sys/slogcodes.h>
#include <sys/types.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <sys/io-pkt.h>
#include <sys/cache.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <hw/inout.h>
#include <netdrvr/mdi.h>
#include <netdrvr/nicsupport.h>
#include <netdrvr/ptp.h>
#include <hw/nicinfo.h>
#include <sys/device.h>

#include "dma_descs.h"

#ifdef  __cplusplus
    extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Driver definitions
 * -------------------------------------------------------------------------
 */

#define DEFAULT_NUM_RX_DESCRIPTORS  32
#define DEFAULT_NUM_TX_DESCRIPTORS  32

#define MAX_MII_RW_TIMEOUT          128
#define RX_BUF_SIZE                 2048

#define ENET_SIZE                   0x1500
#define MTL_MEMORY_SIZE             0x5000

/* -------------------------------------------------------------------------
 * Hardware registers
 * -------------------------------------------------------------------------
 */
#define START_MAC_REG_OFFSET        0x0000
#define MAX_MAC_REG_OFFSET          0x0bd0
#define START_MTL_REG_OFFSET        0x0c00
#define MAX_MTL_REG_OFFSET          0x0d7c
#define START_DMA_REG_OFFSET        0x1000
#define MAX_DMA_REG_OFFSET          0x117C
#define DMA_CHAN_NUM                7
#define HW_FEATURE_NUM              4

/* -------------------------------------------------------------------------
 * MAC registers
 * -------------------------------------------------------------------------
 */
#define MAC_CFG               0x0000           /* MAC configuration Register */
    #define ARPEN                 (1 << 31)        /* ARP Offload Enable */
    #define SARC_MASK             (0x7 << 28)      /* Source Address insertion or Replacement Control */
    #define IPC                   (1 << 27)        /* Checksum Offload */
    #define IPG_MASK              (0x7 << 24)      /* Inter-Packet Gap */
    #define GPSLCE                (1 << 23)        /* Giant Packet Size Limit Control Enable */
    #define S2XP                  (1 << 22)        /* IEEE 802.3as support for 2K packets */
    #define CST                   (1 << 21)        /* CRC stripping for type frames */
    #define ACS                   (1 << 20)        /* Automatic Pad or CRC Stripping */
    #define WD                    (1 << 19)        /* Watchdog Disable */
    #define BE                    (1 << 18)        /* Packet Burst Enable */
    #define JD                    (1 << 17)        /* Jabber Disable */
    #define JE                    (1 << 16)        /* Jumbo Packet Enable */
    #define PS                    (1 << 15)        /* Port Select */
    #define FES                   (1 << 14)        /* Speed */
    #define DM                    (1 << 13)        /* Duplex Mode */
    #define LM                    (1 << 12)        /* LoopBack Mode */
    #define ECRSFD                (1 << 11)        /* Enable Carrier Sense Before Transmission in Full-Duplex Mode */
    #define DO                    (1 << 10)        /* Disable Receive Own */
    #define DCRS                  (1 << 9)         /* Disable Carrier Sense During Transmission */
    #define DR                    (1 << 8)         /* Disable Retry */
    #define BL_MASK               (0x3 << 5)       /* Back-off Limit */
    #define DC                    (1 << 4)         /* Deferral Check */
    #define PRELEN_MASK           (0x3 << 2)       /* Preamble Length for Transmit packets */
    #define TE                    (1 << 1)         /* Transmitter Enable */
    #define RE                    (1 << 0)         /* Receiver Enable */

#define MAC_EXT_CFG           0x0004           /* The MAC Extended Configuration register */
    #define EIPG_MASK             (0x1F << 25)     /* Extended Inter-Packet Gap */
    #define EIPGEN                (1 << 24)        /* Extended Inter-Packet Gap Enable */
    #define HDSMS                 (0x7 << 20)      /* Maximum Size for Splitting the Header Data */
    #define USP                   (1 << 18)        /* Unicast Slow Protocol Packet Detect */
    #define SPEN                  (1 << 17)        /* Slow Protocol Detection Enable */
    #define DCRCC                 (1 << 16)        /* Disable CRC Checking for Received Packets */
    #define GPSL_MASK             (0x3FFF << 0)    /* Giant Packet Size Limit */

#define MAC_PACKET_FILTER     0x0008           /* The MAC Packet Filter register */
    #define RA                    (1 << 31)        /* Receive All */
    #define DNTU                  (1 << 21)        /* Drop Non-TCP/UDP over IP Packets */
    #define IPFE                  (1 << 20)        /* Layer 3 and Layer 4 Filter Enable */
    #define VTFE                  (1 << 16)        /* VLAN Tag Filter Enable */
    #define SAFE                  (1 << 9)         /* Source Address Filter Enable */
    #define SAIF                  (1 << 8)         /* SA Inverse Filtering */
    #define PCF_MASK              (0x3 << 6)       /* Pass Control Packets */
    #define DBF                   (1 << 5)         /* Disable Broadcast Packets */
    #define PM                    (1 << 4)         /* Pass All Multicast */
    #define DAIF                  (1 << 3)         /* DA Inverse Filtering */
    #define PR                    (1 << 0)         /* Promiscuous Mode */

#define MAC_WATCHGOD_TIMEOUT  0x000C           /* The Watchdog Timeout register */
    #define PWE                   (1 << 8)         /* Programmable Watchdog Enable */
    #define WTO_MASK              (0xF << 0)       /* Watchdog Timeout */

#define HASTABLE_LO           0x0010
#define HASTABLE_HI           0x0014

#define MAC_VLAN_TAG          0x0050           /* The VLAN Tag register */
    #define EIVLRXS               (1 << 31)        /* Enable Inner VLAN Tag in Rx Status */
    #define EIVLS_MASK            (0x3 << 28)      /* Enable Inner VLAN Tag Stripping on Receive */
    #define ERIVLT                (1 << 27)        /* Enable Inner VLAN Tag */
    #define EDVLP                 (1 << 26)        /* Enable Double VLAN Processing */
    #define EVLRXS                (1 << 24)        /* Enable VLAN Tag in Rx status */
    #define EVLS_MASK             (0x3 << 21)      /* Enable VLAN Tag Stripping on Receive */
    #define DOVLTC                (1 << 20)        /* Disable VLAN Type Check */
    #define ERSVLM                (1 << 19)        /* Enable Receive S-VLAN Match */
    #define ESVL                  (1 << 18)        /* Enable S-VLAN */
    #define VTIM                  (1 << 17)        /* VLAN Tag Inverse Match Enable */
    #define ETV                   (1 << 16)        /* Enable 12-bit VLAN Tag Comparison */
    #define VL_MASK               (0xFFFF << 0)    /* VLAN Tag Identifier for Receive Packets */

#define EVLS_NO_STRIP         0x0
#define EVLS_STRIP_IF_PASS    0x1
#define EVLS_STRIP_IF_FAIL    0x2
#define EVLS_ALWAYS_STRIP     0x3

#define MAC_VLAN_INCL         0x0060           /* The VLAN Tag Inclusion or Replacement register */
    #define BUSY                  (1 << 31)        /* Busy */
    #define RDWR                  (1 << 30)        /* Read write control */
    #define ADDR_MASK             (0x3 << 24)      /* Address */
    #define CBTI                  (1 << 21)        /* Channel based tag insertion */
    #define VLTI                  (1 << 20)        /* VLAN Tag Input */
    #define CSVL                  (1 << 19)        /* C-VLAN or S-VLAN */
    #define VLP                   (1 << 18)        /* VLAN Priority Control */
    #define VLC_MASK              (0x3 << 16)      /* VLAN Tag Control in Transmit Packets */
    #define VLT_MASK              (0xFFFF << 0)    /* VLAN Tag for Transmit Packets */

#define MAC_Q0_TX_FLOW_CTRL   0x0070           /* The Tx Flow Control register of Q0 */
    #define PT_MASK               (0xFFFF << 16)   /* Pause Time */
    #define DZPQ                  (1 << 7)         /* Disable Zero-Quanta Pause */
    #define PLT_MASK              (0x7 << 4)       /* Pause Low Threshold */
    #define TFE                   (1 << 1)         /* Transmit Flow Control Enable */
    #define FCB_BPA               (1 << 0)         /* Flow Control Busy or Backpressure Activate */
    #define PT_SHIFT(x)           (x << 16)

#define MAC_Qi_TX_FLOW_CTRL(i)                (MAC_Q0_TX_FLOW_CTRL + ((i) * 0x0004)) /* Tx Flow Control register of Qi: i = {1...TXQCNT-1} */

#define MAC_Q0_TX_FLOW_CTRL_PLT_4_SLOTS        MAC_Q0_TX_FLOW_CTRL_PLT(0)
#define MAC_Qi_TX_FLOW_CTRL_PLT_4_SLOTS        MAC_Qi_TX_FLOW_CTRL_PLT(0)

#define MAC_RX_FLOW_CTRL      0x0090           /* The Receive Flow-Control register */
    #define PFCE                  (1 << 8)         /* Priority Based Flow-Control Enable */
    #define UP                    (1 << 1)         /* Unicast Pause Packet Detect */
    #define RFE                   (1 << 0)         /* Receive Flow Control Enable */

#define MAC_RXQ_CTRL4         0x0094           /* The Receive Queue Control 4 register */
    #define MFFQ_MASK             (0x7 << 9)       /* Multicast Address Filter Fail Packets Queue. */
    #define MFFQE                 (1 << 8)         /* Multicast Address Filter Fail Packets Queuing Enable. */
    #define UFFQ_MASK             (0x7 << 1)       /* Unicast Address Filter Fail Packets Queue. */
    #define UFFQE                 (1 <<  0)        /* Unicast Address Filter Fail Packets Queuing Enable. */

#define MAC_TXQ_PRTY_MAP0     0x0098           /* The Transmit Queue Priority Mapping 0 register */
    #define PSTQ3_MASK            (0xFF << 24)     /* Priorities Selected in Transmit Queue 3 */
    #define PSTQ2_MASK            (0xFF << 16)     /* Priorities Selected in Transmit Queue 2 */
    #define PSTQ1_MASK            (0xFF << 8)      /* Priorities Selected in Transmit Queue 1 */
    #define PSTQ0_MASK            (0xFF << 0)      /* Priorities Selected in Transmit Queue 0 */

#define MAC_TXQ_PRTY_MAP1     0x009C           /* The Transmit Queue Priority Mapping 1 register */
    #define PSTQ7_MASK            (0xFF << 24)     /* Priorities Selected in Transmit Queue 7 */
    #define PSTQ6_MASK            (0xFF << 16)     /* Priorities Selected in Transmit Queue 6 */
    #define PSTQ5_MASK            (0xFF << 8)      /* Priorities Selected in Transmit Queue 5 */
    #define PSTQ4_MASK            (0xFF << 0)      /* Priorities Selected in Transmit Queue 4 */

#define MAC_RXQ_CTRL0         0x00A0           /* The Receive Queue Control 0 register */
    #define RXQ7EN_MASK           (0x3 << 14)      /* Receive Queue 7 Enable */
    #define RXQ6EN_MASK           (0x3 << 12)      /* Receive Queue 6 Enable */
    #define RXQ5EN_MASK           (0x3 << 10)      /* Receive Queue 5 Enable */
    #define RXQ4EN_MASK           (0x3 << 8)       /* Receive Queue 4 Enable */
    #define RXQ3EN_MASK           (0x3 << 6)       /* Receive Queue 3 Enable */
    #define RXQ2EN_MASK           (0x3 << 4)       /* Receive Queue 2 Enable */
    #define RXQ1EN_MASK           (0x3 << 2)       /* Receive Queue 1 Enable */
    #define RXQ0EN_MASK           (0x3 << 0)       /* Receive Queue 0 Enable */

#define MAC_RXQ_CTRL0_RXQEN_OFF   0x0
#define MAC_RXQ_CTRL0_RXQEN_AV    0x1
#define MAC_RXQ_CTRL0_RXQEN_DCB   0x2

#define MAC_RXQ_CTRL1         0x00A4           /* The Receive Queue Control 1 register */
    #define TPQC_MASK             (0x3 << 22)      /* Tagged PTP over Ethernet Packets Queuing Control. */
    #define TACPQE                (1 << 21)        /* Tagged AV Control Packets Queuing Enable */
    #define MCBCQEN               (1 << 20)        /* Multicast and Broadcast Queue Enable */
    #define MCBCQ_MASK            (0x7 << 16)      /* Multicast and Broadcast Queue */
    #define UPQ_MASK              (0x7 << 12)      /* Untagged Packet Queue */
    #define DCBCPQ_MASK           (0x7 << 8)       /* DCB Control Packets Queue */
    #define PTPQ_MASK             (0x7 << 4)       /* PTP Packets Queue */
    #define AVCPQ_MASK            (0x7 << 0)       /* AV Untagged Control Packets Queue */

#define MAC_RXQ_CTRL2         0x00A8           /* The Receive Queue Control 2 register */
    #define PSRQ3_MASK            (0xFF << 24)     /* Priorities Selected in the Receive Queue 3. */
    #define PSRQ2_MASK            (0xFF << 16)     /* Priorities Selected in the Receive Queue 2. */
    #define PSRQ1_MASK            (0xFF << 8)      /* Priorities Selected in the Receive Queue 1. */
    #define PSRQ0_MASK            (0xFF << 0)      /* Priorities Selected in the Receive Queue 0. */

#define MAC_RXQ_CTRL3         0x00AC           /* The Receive Queue Control 3 register */
    #define PSRQ6_MASK            (0xFF << 16)     /* Priorities Selected in the Receive Queue 6 */
    #define PSRQ5_MASK            (0xFF << 8)      /* Priorities Selected in the Receive Queue 5 */
    #define PSRQ4_MASK            (0xFF << 0)      /* Priorities Selected in the Receive Queue 4 */

#define MAC_INTR_STATUS       0x00B0           /* The Interrupt Status register */
    #define RXSTSIS               (1 << 14)        /* Receive Status Interrupt */
    #define TXSTSIS               (1 << 13)        /* Transmit Status Interrupt */
    #define TSIS                  (1 << 12)        /* Timestamp Interrupt Status */
    #define MMCRXPIS              (1 << 11)        /* MMC Receive Checksum Offload Interrupt Status */
    #define MMCTXIS               (1 << 10)        /* MMC Transmit Interrupt Status */
    #define MMCRXIS               (1 << 9)         /* MMC Receive Interrupt Status */
    #define MMCIS                 (1 << 8)         /* MMC Interrupt Status */
    #define LPIIS                 (1 << 5)         /* LPI Interrupt Status */
    #define PMTIS                 (1 << 4)         /* PMT Interrupt Status */
    #define PHYIS                 (1 << 3)         /* PHY Interrupt */
    #define PCSANCIS              (1 << 2)         /* PCS Auto-Negotiation Complete */
    #define PCSLCHGIS             (1 << 1)         /* PCS Link Status Changed */
    #define RGSMIIIS              (1 << 0)         /* RGMII or SMI Interrupt Status */

#define MAC_INTR_ENABLE       0x00B4            /* The Interrupt Enable register */
    #define RXSTSIE               (1 << 14)        /* Receive Status Interrupt Enable */
    #define TXSTSIE               (1 << 13)        /* Transmit Status Interrupt Enable */
    #define TSIE                  (1 << 12)        /* Timestamp Interrupt Enable */
    #define LPIIE                 (1 << 5)         /* LPI Interrupt Enable */
    #define PMTIE                 (1 << 4)         /* PMT Interrupt Enable */
    #define PHYIE                 (1 << 3)         /* PHY Interrupt Enable */
    #define PCSANCIE              (1 << 2)         /* PCS AN Completion Interrupt Enable */
    #define PCSLCHGIE             (1 << 1)         /* PCS Link Status Interrupt Enable */
    #define RGSMIIIE              (1 << 0)         /* RGMII or SMI Interrupt Enable */

#define MAC_RX_TX_STATUS      0x00B8           /* The Receive Transmit Status register */
    #define MAC_RWT               (1 << 8)         /* Receive Watchdog Timeout */
    #define EXCOL                 (1 << 5)         /* Excessive Collisions */
    #define LCOL                  (1 << 4)         /* Late Collision */
    #define EXDEF                 (1 << 3)         /* Excessive Defernal */
    #define LCARR                 (1 << 2)         /* Loss of Carrier */
    #define NCARR                 (1 << 1)         /* No Carrier */
    #define TJT                   (1 << 0)         /* Transmit Jabber Timeout */

#define MAC_PMT_CTRL_STATUS   0x00C0           /* The PMT Control and Status Register */
    #define RWKFILTRST            (1 << 31)        /* Remote Wake-Up Packet Filter Register Pointer Reset */
    #define RWKPTR_MASK           (0x1F << 24)     /* Remote Wake-Up FIFO Pointer */
    #define RWKPFE                (1 << 10)        /* Remote Wake-Up Packet Forwarding Enable */
    #define GLBLUCAST             (1 << 9)         /* Global Unicast */
    #define RWKPRCVD              (1 << 6)         /* Remote Wake-Up Packet Received */
    #define MGKPRCVD              (1 << 5)         /* Magic Packet Received */
    #define RWKPKTEN              (1 << 2)         /* Remote Wake-Up Packet Enable */
    #define MGKPKTEN              (1 << 1)         /* Magic Packet Enable */
    #define PWRDWN                (1 << 0)         /* Power Down */

#define MAC_RWK_PKT_FILTER    0x00C4           /* The Wake-up Packet Filter register */
    #define WKUPFRMFTR_MASK       (0xFFFFFFFF)     /* RWK Packet Filter */

#define MAC_LPI_CTRL_STATUS   0x00D0           /* The LPI Control and Status Register */
    #define LPITCSE               (1 << 21)        /* LPI Tx Clock Stop Enable */
    #define LPIATE                (1 << 20)        /* LPI Timer Enable */
    #define LPITXA                (1 << 19)        /* LPI Tx Automate */
    #define PLSEN                 (1 << 18)        /* PHY Link Status Enable */
    #define PLS                   (1 << 17)        /* PHY Link Status */
    #define LPIEN                 (1 << 16)        /* LPI Enable */
    #define RLPIST                (1 << 9)         /* Receive LPI Status */
    #define TLPIST                (1 << 8)         /* Transmit LPI Status */
    #define RLPIEX                (1 << 3)         /* Receive LPI Exit */
    #define RLPIEN                (1 << 2)         /* Receive LPI Entry */
    #define TLPIEX                (1 << 1)         /* Transmit LPI Exit */
    #define TLPIEN                (1 << 0)         /* Transmit LPI Entry */

#define MAC_LPI_CTRL_ENABLE_EEE   (LPIATE | LPITXA | LPIEN)

#define MAC_LPI_TIMERS_CTRL   0x00D4           /* The LPI Timers Control register */
    #define LST_MASK              (0x3FF << 16)    /* LPI LS Timer */
    #define TWT_MASK              (0xFFFF << 0)    /* LPI TW Timer */

#define MAC_LPI_ENTRY_TIMER   0x00D8           /* The LPI Entry Timer register */
    #define LPIET_MASK            (0x1FFFF << 3)   /* LPI Entory Timer */

#define MAC_1US_TIC_CNT       0x00DC           /* The 1US Tic Counter register */
    #define CNT_MASK              (0xFFF << 0)     /* 1US TIC Counter */

#define MAC_PHYIF_CTRL_STATUS 0x00F8           /* The PHY Interface Control and Status register */
    #define FALSCARDET            (1 << 21)        /* False Carrier Detected */
    #define JABTO                 (1 << 20)        /* Jabber Timeout */
    #define LNKSTS                (1 << 19)        /* LInk Status */
    #define LNKSPEED_MASK         (0x3 << 17)      /* Link Speed */
    #define LNKMOD                (1 << 16)        /* Link Mode */
    #define LUD                   (1 << 1)         /* Link Up or Down */
    #define TC                    (1 << 0)         /* Transmit Configuration in RGMII */

#define MAC_VERSION           0x0110
    #define IMPLEMENTOR           (0xFF << 8)
    #define SNPS                  (0xFF << 0)

#define MAC_HW_FEATURE0       0x011C           /* The HW Feature0 Register */
    #define ACTPHYSEL_MASK        (0x7 << 28)      /* Active PHY Selected */
    #define SAVLANINS             (1 << 27)        /* Source Address or VLAN Insertion Enable */
    #define TSSTSSEL_MASK         (0x3 << 25)      /* Timestamp System Time Source */
    #define TSSTSSEL_SHIFT        25               /* Timestamp System Time Source Shifter */
    #define MACADR64SEL           (1 << 24)        /* MAC Addresses 64-127 Selected */
    #define MACADR32SEL           (1 << 23)        /* MAC Addresses 32-63 Selected */
    #define ADDMACADRSEL_MASK     (0x1F << 18)     /* MAC Addresses 1-31 Selected */
    #define RXCOESEL              (1 << 16)        /* Receive Checksum Offload Enabled */
    #define TXCOESEL              (1 << 14)        /* Transmit Checksum Offload Enabled */
    #define EEESEL                (1 << 13)        /* Energy Efficient Ethernet Enabled */
    #define TSSEL                 (1 << 12)        /* IEEE 1588-2008 Timestamp Enabled */
    #define ARPOFFSEL             (1 << 9)         /* ARP Offload Enabled */
    #define MMCSEL                (1 << 8)         /* RMON Module Enable */
    #define MGKSEL                (1 << 7)         /* PMT Magic Packet Enable */
    #define RWKSEL                (1 << 6)         /* PMT Remote Wake-Up Packet Enable */
    #define SMASEL                (1 << 5)         /* SMA(MDIO) Interface */
    #define VLHASH                (1 << 4)         /* VLAN Hash Filter Selected */
    #define PCSSEL                (1 << 3)         /* PCS Register */
    #define HDSEL                 (1 << 2)         /* Half-duplex suport */
    #define GMIISEL               (1 << 1)         /* 1000 Mbps Support */
    #define MIISEL                (1 << 0)         /* 10 or 100 Mbps Support */

#define MAX_PERFECT_ADDRESSES(feature0)     (1 + (((feature0) & 0x1fc0000) >> 18))

#define MAC_HW_FEATURE1       0x0120           /* The HW Feature1 Register */
    #define L3L4FNUM_MASK         (0xF << 27)      /* Total number of L3 or L4 Filters */
    #define HASHTBLSZ_MASK        (0x3 << 24)      /* Hash Table Size */
    #define POUOST                (1 << 23)        /* One Step for PTP over UDP/IP Feature Enable */
    #define RAVSEL                (1 << 21)        /* Rx Side Only AV Feature Enable */
    #define AVSEL                 (1 << 20)        /* AV Feature Enable */
    #define DBGMEMA               (1 << 19)        /* DMA Debug Registers Enable */
    #define TSOEN                 (1 << 18)        /* TCP Segmentation Offload Enable */
    #define SPHEN                 (1 << 17)        /* Split Header Feature Enable */
    #define DCBEN                 (1 << 16)        /* DCB Feature Enable */
    #define ADDR64_MASK           (0x3 << 14)      /* Address Width */
    #define ADVTHWORD             (1 << 13)        /* IEEE 1588 High Word Register Enable */
    #define HW_FTR_PTOEN          (1 << 12)        /* PTP Offload Enable */
    #define OSTEN                 (1 << 11)        /* One-Step Timestamping Enable */
    #define TXFIFOSIZE_SHIF       6
    #define TXFIFOSIZE_MASK       (0x1F << 6)      /* MTL Transmit FIFO Size */
    #define SPRAM                 (1 << 5)         /* Single Port RAM Enable */
    #define RXFIFOSIZE_MASK       (0x1F << 0)      /* MTL Receive FIFO Size */
    #define RXFIFOSIZE_SHIF       0

#define MAC_HW_FEATURE2       0x0124           /* The HW Feature2 Register */
    #define AUXSNAPSUM_MASK       (0x7 << 28)      /* Number of Auxiliary Snapshot Inputs */
    #define PPSOUTNUM_MASK        (0x7 << 24)      /* Number of PPS Outputs */
    #define TXCHCNT_MASK          (0xF << 18)      /* Number of DMA Transmit Channels */
    #define RXCHCNT_MASK          (0xF << 12)      /* Number of DMA Receive Channels */
    #define TXQCNT_MASK           (0xF << 6)       /* Number of MTL Transmit Queues */
    #define RXQCNT_MASK           (0xF << 0)       /* Number of MTL Receive Queues */

#define MAC_HW_FEATURE3       0x0128           /* The HW Feature3 register */
    #define CBTISEL               (1 << 4)         /* Queues/Channel based VLAN tag insertion on Tx Enable */
    #define NRVF_MASK             (0x7 << 0)       /* Number of Extended VLAN Tag Filters Enabled */

#define MAC_MDIO_ADDR         0x0200           /* The MDIO Address register */
    #define PSE                   (1 << 27)        /* Preamble Suppression Enable */
    #define BTB                   (1 << 26)        /* Back to Back Transactions */
    #define PA(x)                 (x << 21)        /* Physical Layer Address */
    #define PA_MASK               (0x1F << 21)     /* Physical Layer Address */
    #define RDA(x)                (x << 16)        /* Register/Device Address */
    #define RDA_MASK              (0x1F << 16)     /* Register/Device Address */
    #define NTC_MASK              (0x7 << 12)      /* Number of Trailing Clocks */
    #define CR_MASK               (0xF << 8)       /* CSR Clock Range */
    #define SKAP                  (1 << 4)         /* Skip Address Packet */
    #define GOC_1                 (1 << 3)         /* GMIII Operation Command 1 */
    #define GOC_0                 (1 << 2)         /* GMIII Operation Command 0 */
    #define C45E                  (1 << 1)         /* Clause 45 PHY Enable */
    #define GB                    (1 << 0)         /* GMII Busy */

    #define CR_60                  0
    #define CR_100                 (1 << 8)
    #define CR_20                  (2 << 8)
    #define CR_35                  (3 << 8)
    #define CR_150                 (4 << 8)
    #define CR_250                 (5 << 8)

    #define GOC_READ          GOC_1 | GOC_0
    #define GOC_WRITE         GOC_0

#define MAC_MDIO_DATA         0x0204           /* The MDIO Data register */
    #define RA_MASK               (0x3F << 16)     /* Register Address */
    #define GD_MASK               (0xFFFF << 0)    /* GMII Address */

#define MAC_ARP_ADDRESS       0x0210  /* The ARP Address register */
    #define ARPPA_MASK            (0xFFFFFFFF)    /* ARP Protocol Address */

#define MAC_CSR_SW_CTRL       0x0230           /* The CSR SW control register */
    #define SEEN                  (1 << 8)         /* Slave Error Response Enable */
    #define RCWE                  (1 << 0)         /* Register Clear on Write 1 Enable */

#define MAC_ADDR_TBL_SZ            32

#define MAC_ADDR0_HIGH        0x0300           /* The MAC Address0 High register */
    #define ADDR0_HIGH_AE          (1 << 31)       /* Address Enable */
    #define ADDR0_HIGH_DCS_MASK    (0x3F << 16)    /* DMA Channel Select */
    #define ADDR0_HIGH_ADDRHI_MASK (0xFFFF << 0)   /* MAC Address0[47:32] */

#define MAC_ADDR0_LOW         0x0304           /* The MAC Address0 Low register */
    #define ADDR0_LOW_ADDRLO_MASK  (0xFFFFFFFF)    /* MAC Address0[31:0] */

#define MAC_ADDRi_HIGH(i)     (MAC_ADDR0_HIGH + ((i) * 0x8)) /* The MAC Address i High register. i = {1...MAX_MAC_ADDR-1} */
    #define ADDRi_HIGH_AE          (1 << 31)       /* Address Enable */
    #define ADDRi_HIGH_SA          (1 << 30)       /* Source Address */
    #define ADDRi_HIGH_MBC_MASK    (0x3F << 24)    /* Mask Byte Control */
    #define ADDRi_HIGH_DCS_MASK    (0x3F << 16)    /* DMA Channel Select */
    #define ADDRi_HIGH_ADDRHI_MASK (0xFFFF << 0)   /* MAC Address1[47:32] */

#define MAC_ADDRi_LOW(i)      (MAC_ADDR0_LOW + ((i) * 0x8)) /* The MAC Address i Low register. i = {1...MAX_MAC_ADDR-1} */
    #define ADDRi_LOW_ADDRLO_MASK  (0xFFFFFFFF)    /* MAC Address1[31:0] */

/* -------------------------------------------------------------------------
 * MAC registers
 * -------------------------------------------------------------------------
 */
#define MMC_CONTROL           0x0700           /* The MMC control register */
    #define UCDBC                 (1 << 8)         /* Update MMC Counters for Dropped Broadcast Packets */
    #define CNTPRSTLVL            (1 << 5)         /* Full-Half Preset */
    #define MMC_CNTPRST           (1 << 4)         /* Counters Preset */
    #define CNTFREEZ              (1 << 3)         /* MMC Counter Freeze */
    #define RSTONRD               (1 << 2)         /* Reset on Read */
    #define CNTSTOPRO             (1 << 1)         /* Counter Stop Rollover */
    #define CNTRST                (1 << 0)         /* Counters Reset */

#define MMC_RX_INTR           0x0704            /* The MMC Rx Interrupt register */
    #define RXLPITRCIS            (1 << 27)        /* MMC Receive LPI transition counter interrupt status. */
    #define RXLPIUSCIS            (1 << 26)        /* MMC Receive LPI microsecond counter interrupt status. */
    #define RXCTRLPIS             (1 << 25)        /* MMC Receive Control Packet Counter Interrupt Status. */
    #define RXRCVERRPIS           (1 << 24)        /* MMC Receive Error Packet Counter Interrupt Status. */
    #define RXWDOGPIS             (1 << 23)        /* MMC Receive Watchdog Error Packet Counter Interrupt Status. */
    #define RXVLANGBPIS           (1 << 22)        /* MMC Receive VLAN Good Bad Packet Counter Interrupt Status. */
    #define RXFOVPIS              (1 << 21)        /* MMC Receive FIFO Overflow Packet Counter Interrupt Status. */
    #define RXPAUSPIS             (1 << 20)        /* MMC Receive Pause Packet Counter Interrupt Status. */
    #define RXORANGEPIS           (1 << 19)        /* MMC Receive Out of Range Error Packet Counter Interrut Status. */
    #define RXLENERPIS            (1 << 18)        /* MMC Receive Length Error Packet Counter Interrupt Status. */
    #define RXUCGPIS              (1 << 17)        /* MMC Receive Unicast Good Packet Counter Interrupt Status. */
    #define RX1024TMAXOCTGBPIS    (1 << 16)        /* MMC Receive 1024 to maximum Octet Good Bad Packet Counter Interrupt Status. */
    #define RX512T1023OCTGBPIS    (1 << 15)        /* MMC Receive 512 to 1023 Octet Good Bad Packet Counter Interrupt Status. */
    #define RX256T511OCTGBPIS     (1 << 14)        /* MMC Receive 256 to 511 Octet Good Bad Packet Counter Interrupt Status. */
    #define RX128T255OCTGBPIS     (1 << 13)        /* MMC Receive 128 to 255 Octet Good Bad Packet Counter Interrupt Status. */
    #define RX65T127OCTGBPIS      (1 << 12)        /* MMC Receive 65 to 127 Octet Good Bad Packet Counter Interrupt Status. */
    #define RX64OCTGBPIS          (1 << 11)        /* MMC Receive 64 Octet Good Bad Packet Counter Interrupt Status. */
    #define RXOSIZEGPIS           (1 << 10)        /* MMC Receive Oversize Good Packet Counter Interrupt Status. */
    #define RXUSIZEGPIS           (1 << 9)         /* MMC Receive Undersize Good Packet Counter Interrupt Status. */
    #define RXJABERPIS            (1 << 8)         /* MMC Receive Jabber Error Packet Counter Interrupt Status. */
    #define RXRUNTPIS             (1 << 7)         /* MMC Receive Runt Packet Counter Interrupt Status. */
    #define RXALGNERPIS           (1 << 6)         /* MMC Receive Alignment Error Packet Counter Interrupt Status. */
    #define RXCRCERPIS            (1 << 5)         /* MMC Receive CRC Error Packet Counter Interrupt Status. */
    #define RXMCGPIS              (1 << 4)         /* MMC Receive Multicast Good Packet Counter Interrupt Status. */
    #define RXBCGPIS              (1 << 3)         /* MMC Receive Broadcast Good Packet Counter Interrupt Status. */
    #define RXGOCTIS              (1 << 2)         /* MMC Receive Good Octet Counter Interrupt Status. */
    #define RXGBOCTIS             (1 << 1)         /* MMC Receive Good Bad Octet Counter Interrupt Status. */
    #define RXGBPKTIS             (1 << 0)         /* MMC Receive Good Bad Packet Counter Interrupt Status. */

#define MMC_TX_INTR           0x0708           /* The MMC Tx Interrupt register */
    #define TXLPITRCIS            (1 << 27)        /* MMC Transmit LPI transmission counter interrupt status. */
    #define TXLPIUSCIS            (1 << 26)        /* MMC Transmit LPI microsecond counter interrupt status. */
    #define TXOSIZEGPIS           (1 << 25)        /* MMC Transmit Oversize Good Packet Counter Interrupt Status. */
    #define TXVLANGPIS            (1 << 24)        /* MMC Transmit VLAN Good Packet Counter Interrupt Status. */
    #define TXPAUSPIS             (1 << 23)        /* MMC Transmit Pause Packet Counter Interrupt Status. */
    #define TXEXDEFPIS            (1 << 22)        /* MMC Transmit Excessive Defernal Packet Counter Interrupt Status. */
    #define TXGPKTIS              (1 << 21)        /* MMC Transmit Good Packet Counter Interrupt Status. */
    #define TXGOCTIS              (1 << 20)        /* MMC Transmit Good Octet Counter Interrupt Status. */
    #define TXCARERPIS            (1 << 19)        /* MMC Transmit Carrier Error Packet Counter Interrupt Status. */
    #define TXEXCOLPIS            (1 << 18)        /* MMC Transmit Excessive Collision Packet Counter Interrupt Status. */
    #define TXLATCOLPIS           (1 << 17)        /* MMC Transmit Late Collision Packet Counter Interrupt Status. */
    #define TXDEFPIS              (1 << 16)        /* MMC Transmit Defferd Packet Counter Interrupt Status. */
    #define TXMCOLGPIS            (1 << 15)        /* MMC Transmit Multiple Collision Good Packet Counter Interrupt Status. */
    #define TXSCOLGPIS            (1 << 14)        /* MMC Transmit Single Collision Good Packet Counter Interrupt Status. */
    #define TXUFLOWERPIS          (1 << 13)        /* MMC Transmit Underflow Error Packet Counter Interrupt Status. */
    #define TXBCGBPIS             (1 << 12)        /* MMC Transmit Broadcast Good Bad Packet Counter Interrupt Status. */
    #define TXMCGPIS              (1 << 11)        /* MMC Transmit Multicast Good Bad Packet Counter Interrupt Status. */
    #define TXUCGPIS              (1 << 10)        /* MMC Transmit Unicast Good Bad Packet Counter Interrupt Status. */
    #define TX1024TMAXOCTGBPIS    (1 << 9)         /* MMC Transmit 1024 to Maximum Octet Good Bad Packet Counter Interrupt Status. */
    #define TX512T1023OCTGBPIS    (1 << 8)         /* MMC Transmit 512 to 1023 Octet Good Bad Packet Counter Interrupt Status. */
    #define TX256T511OCTGBPIS     (1 << 7)         /* MMC Transmit 256 to 511 Octet Good Bad Packet Counter Interrupt Status. */
    #define TX128T255OCTGBPIS     (1 << 6)         /* MMC Transmit 128 to 255 Octet Good Bad Packet Counter Interrupt Status. */
    #define TX65T127OCTGBPIS      (1 << 5)         /* MMC Transmit 65 to 127 Octet Good Bad Packet Counter Interrupt Status. */
    #define TX64OCTGBPIS          (1 << 4)         /* MMC Transmit 64 Octet Good Bad Packet Counter Interrupt Status. */
    #define RXBCGPIS              (1 << 3)         /* MMC Transmit Multicast Good Packet Counter Interrupt Status. */
    #define TXBCGPIS              (1 << 2)         /* MMC Transmit Broadcast Good Packet Counter Interrupt Status. */
    #define TXGBPKTIS             (1 << 1)         /* MMC Transmit Good Bad Packet Counter Interrupt Status. */
    #define TXGBOCTIS             (1 << 0)         /* MMC Transmit Good Bad Octet Counter Interrupt Status. */

#define MMC_RX_INTR_MASK      0x070C           /* The MMC Rx Interrupt Mask register */
    #define RXLPITRCIM            (1 << 27)        /* MMC Receive LPI transition counter interrupt Mask */
    #define RXLPIUSCIM            (1 << 26)        /* MMC Receive LPI microsecond counter interrupt Mssk */
    #define RXCTRLPIM             (1 << 25)        /* MMC Receive Control Packet Counter Interrupt Mask */
    #define RXRCVERRPIM           (1 << 24)        /* MMC Receive Error Packet Counter Interrupt Mask */
    #define RXWDOGPIM             (1 << 23)        /* MMC Receive Watchdog Error Packet Counter Interrupt Mask */
    #define RXVLANGBPIM           (1 << 22)        /* MMC Receive VLAN Good Bad Packet Counter Interrupt Mask. */
    #define RXFOVPIM              (1 << 21)        /* MMC Receive FIFO Overflow Packet Counter Interrupt Mask. */
    #define RXPAUSPIM             (1 << 20)        /* MMC Receive Pause Packet Counter Interrupt Mask. */
    #define RXORANGEPIM           (1 << 19)        /* MMC Receive Out of Range Error Packet Counter Interrut Mask. */
    #define RXLENERPIM            (1 << 18)        /* MMC Receive Length Error Packet Counter Interrupt Mask. */
    #define RXUCGPIM              (1 << 17)        /* MMC Receive Unicast Good Packet Counter Interrupt Mask. */
    #define RX1024TMAXOCTGBPIM    (1 << 16)        /* MMC Receive 1024 to maximum Octet Good Bad Packet Counter Interrupt Mask. */
    #define RX512T1023OCTGBPIM    (1 << 15)        /* MMC Receive 512 to 1023 Octet Good Bad Packet Counter Interrupt Mask. */
    #define RX256T511OCTGBPIM     (1 << 14)        /* MMC Receive 256 to 511 Octet Good Bad Packet Counter Interrupt Mask. */
    #define RX128T255OCTGBPIM     (1 << 13)        /* MMC Receive 128 to 255 Octet Good Bad Packet Counter Interrupt Mask. */
    #define RX65T127OCTGBPIM      (1 << 12)        /* MMC Receive 65 to 127 Octet Good Bad Packet Counter Interrupt Mask. */
    #define RX64OCTGBPIM          (1 << 11)        /* MMC Receive 64 Octet Good Bad Packet Counter Interrupt Mask. */
    #define RXOSIZEGPIM           (1 << 10)        /* MMC Receive Oversize Good Packet Counter Interrupt Mask. */
    #define RXUSIZEGPIM           (1 << 9)         /* MMC Receive Undersize Good Packet Counter Interrupt Mask. */
    #define RXJABERPIM            (1 << 8)         /* MMC Receive Jabber Error Packet Counter Interrupt Mask. */
    #define RXRUNTPIM             (1 << 7)         /* MMC Receive Runt Packet Counter Interrupt Mask. */
    #define RXALGNERPIM           (1 << 6)         /* MMC Receive Alignment Error Packet Counter Interrupt Mask. */
    #define RXCRCERPIM            (1 << 5)         /* MMC Receive CRC Error Packet Counter Interrupt Mask. */
    #define RXMCGPIM              (1 << 4)         /* MMC Receive Multicast Good Packet Counter Interrupt Mask. */
    #define RXBCGPIM              (1 << 3)         /* MMC Receive Broadcast Good Packet Counter Interrupt Mask. */
    #define RXGOCTIM              (1 << 2)         /* MMC Receive Good Octet Counter Interrupt Mask. */
    #define RXGBOCTIM             (1 << 1)         /* MMC Receive Good Bad Octet Counter Interrupt Mask. */
    #define RXGBPKTIM             (1 << 0)         /* MMC Receive Good Bad Packet Counter Interrupt Mask. */

#define MMC_TX_INTR_MASK      0x0710           /* The MMC Tx Interrupt Mask register */
    #define TXLPITRCIM            (1 << 27)        /* MMC Transmit LPI transmission counter interrupt Mask. */
    #define TXLPIUSCIM            (1 << 26)        /* MMC Transmit LPI microsecond counter interrupt Mask. */
    #define TXOSIZEGPIM           (1 << 25)        /* MMC Transmit Oversize Good Packet Counter Interrupt Mask. */
    #define TXVLANGPIM            (1 << 24)        /* MMC Transmit VLAN Good Packet Counter Interrupt Mask. */
    #define TXPAUSPIM             (1 << 23)        /* MMC Transmit Pause Packet Counter Interrupt Mask. */
    #define TXEXDEFPIM            (1 << 22)        /* MMC Transmit Excessive Defernal Packet Counter Interrupt Mask. */
    #define TXGPKTIM              (1 << 21)        /* MMC Transmit Good Packet Counter Interrupt Mask. */
    #define TXGOCTIM              (1 << 20)        /* MMC Transmit Good Octet Counter Interrupt Mask. */
    #define TXCARERPIM            (1 << 19)        /* MMC Transmit Carrier Error Packet Counter Interrupt Mask. */
    #define TXEXCOLPIM            (1 << 18)        /* MMC Transmit Excessive Collision Packet Counter Interrupt Mask. */
    #define TXLATCOLPIM           (1 << 17)        /* MMC Transmit Late Collision Packet Counter Interrupt Mask. */
    #define TXDEFPIM              (1 << 16)        /* MMC Transmit Defferd Packet Counter Interrupt Mask. */
    #define TXMCOLGPIM            (1 << 15)        /* MMC Transmit Multiple Collision Good Packet Counter Interrupt Mask. */
    #define TXSCOLGPIM            (1 << 14)        /* MMC Transmit Single Collision Good Packet Counter Interrupt Mask. */
    #define TXUFLOWERPIM          (1 << 13)        /* MMC Transmit Underflow Error Packet Counter Interrupt Mask. */
    #define TXBCGBPIM             (1 << 12)        /* MMC Transmit Broadcast Good Bad Packet Counter Interrupt Mask. */
    #define TXMCGPIM              (1 << 11)        /* MMC Transmit Multicast Good Bad Packet Counter Interrupt Mask. */
    #define TXUCGPIM              (1 << 10)        /* MMC Transmit Unicast Good Bad Packet Counter Interrupt Mask. */
    #define TX1024TMAXOCTGBPIM    (1 << 9)         /* MMC Transmit 1024 to Maximum Octet Good Bad Packet Counter Interrupt Mask. */
    #define TX512T1023OCTGBPIM    (1 << 8)         /* MMC Transmit 512 to 1023 Octet Good Bad Packet Counter Interrupt Mask. */
    #define TX256T511OCTGBPIM     (1 << 7)         /* MMC Transmit 256 to 511 Octet Good Bad Packet Counter Interrupt Mask */
    #define TX128T255OCTGBPIM     (1 << 6)         /* MMC Transmit 128 to 255 Octet Good Bad Packet Counter Interrupt Mask. */
    #define TX65T127OCTGBPIM      (1 << 5)         /* MMC Transmit 65 to 127 Octet Good Bad Packet Counter Interrupt Mask. */
    #define TX64OCTGBPIM          (1 << 4)         /* MMC Transmit 64 Octet Good Bad Packet Counter Interrupt Mask. */
    #define RXBCGPIM              (1 << 3)         /* MMC Transmit Multicast Good Packet Counter Interrupt Mask. */
    #define TXBCGPIM              (1 << 2)         /* MMC Transmit Broadcast Good Packet Counter Interrupt Mask. */
    #define TXGBPKTIM             (1 << 1)         /* MMC Transmit Good Bad Packet Counter Interrupt Mask. */
    #define TXGBOCTIM             (1 << 0)         /* MMC Transmit Good Bad Octet Counter Interrupt Mask. */

/* This register provides the number of bytes, exclusive of preamble and retried bytes, in good and bad packets of Tx */
#define TX_OCTET_COUNT_GB     0x0714
    #define TXOCTGB_MASK          (0xFFFFFFFF)     /* Tx Octet Count Good Bad. */

/* This register provides the number of good and bad packets of Tx, exclusive of retried packets */
#define TX_PACKET_COUNT_GB    0x0718
    #define TXPKTGB_MASK          (0xFFFFFFFF)     /* Tx Packets Count Good. Bad. */

/* This register provides the number of good broadcast packets of Tx */
#define TX_BROADCAST_PKTS_G   0x071C
    #define TXBCASTG_MASK         (0xFFFFFFFF)     /* Tx Broadcast Packets Good. */

/* This register provides the number of good multicast packets of Tx */
#define TX_MULTICAST_PKTS_G   0x0720
    #define TXMCASTG_MASK         (0xFFFFFFFF)     /* Tx Multicast Packets Good. */

/* This register provides the number of good and bad packets of Tx with length 64 bytes, exclusive of preamble and retried packets */
#define TX_64OCTETS_PKTS_GB   0x0724
    #define TX64OCTGB_MASK        (0xFFFFFFFF)     /* Tx 64Octets Packets Good_Bad. */

/* This register provides the number of good and bad packets of Tx with length between 65 and 127 bytes, exclusive of preamble and retried packets */
#define TX_65TO127OCTETS_PKTS_GB      0x0728
    #define TX65_127OCTGB_MASK            (0xFFFFFFFF)     /* Tx 65To127Octets Packets Good_Bad. */

/* This register provides the number of good and bad packets of Tx with length between 128 to 255 bytes, exclusive of preamble and retried packets */
#define TX_128TO255OCTETS_PKTS_GB     0x072C
    #define TX128_255OCTGB_MASK           (0xFFFFFFFF)     /* Tx 128To255Octets Packets Good_Bad. */

/* This register provides the number of good and bad packets of Tx with length between 256 to 511 bytes, exclusive of preamble and retried packets */
#define TX_256TO511OCTETS_PKTS_GB     0x0730
    #define TX256_511OCTGB_MASK           (0xFFFFFFFF)     /* Tx 256To511Octets Packets Good_Bad. */

/* This register provides the number of good and bad packets of Tx with length 512 to 1023 bytes, exclusive of preamble and retried packets */
#define TX_512TO1023OCTETS_PKTS_GB    0x0734
    #define TX512_1023OCTGB_MASK          (0xFFFFFFFF)            /* Tx 512To1023Octets Packets Good_Bad. */

/* This register provides the number of good and bad packets of Tx with length 1024 to maxsize bytes, exclusive of preamble and retried packets */
#define TX_1024TOMAXOCTETS_PKTS_GB    0x0738
    #define TX1024_MAXOCTGB_MASK          (0xFFFFFFFF)            /* Tx 1024ToMAXOctets Packets Good_Bad. */

/* This register provides the number of good and bad unicast packets of Tx */
#define TX_UNICAST_PKTS_GB            0x073C
    #define TXUCASTTGB_MASK               (0xFFFFFFFF)            /* Tx Unicast Packets Good Bad. */

/* This register provides the number of good and bad multicast packets of Tx */
#define TX_MULTICAST_PKTS_GB          0x0740
    #define TXMCATGB_MASK                 (0xFFFFFFFF)            /* Tx Multicast Packets Good Bad. */

/* This register provides the number of good and bad broadcast packets of Tx. */
#define TX_BROADCAST_PKTS_GB          0x0744
    #define TXBCATGB_MASK                 (0xFFFFFFFF)            /* Tx Broadcast Packets Good Bad. */

/* This register provides the number of Tx packets aborted */
#define TX_UNDERFLOW_ERROR_PKTS       0x0748
    #define TXUNDRFLW_MASK                (0xFFFFFFFF)            /* Tx Underflow Error Packets */

/* This register provides the number of successfully Tx packets after a single collision in the half-duplex mode */
#define TX_SINGLE_COLLISION_G_PKTS    0x074C
    #define TXSNGLCOLG_MASK               (0xFFFFFFFF)            /* Tx Single Collision Good Packets */

/* This register provides the number of successfully Tx packets after multiple collisions in the half-duplex mode */
#define TX_MULTIPLE_COLLISION_G_PKTS  0x0750
    #define TXMULTCOLG_MASK               (0xFFFFFFFF)            /* Tx Multie Collesion Good Packets */

/* This register provides the number of successfully transmitted after a deferral in the half-duplex mode */
#define TX_DEFERRED_PKTS              0x0754
    #define TXDEFRD_MASK                  (0xFFFFFFFF)            /* Tx Deferred Packets */

/* This register provides the number of Tx packets aborted because of late collision error */
#define TX_LATE_COLLISION_PKTS        0x0758
    #define TXLATECOL_MASK                (0xFFFFFFFF)           /* Tx Late Collision Packets */

/* This register provides the number of Tx packets aborted because of excessive (16) collision errors */
#define TX_EXCESSIVE_COLLISION_PKTS   0x075C
    #define TXEXSCOL_MASK                 (0xFFFFFFFF)           /* Tx Excessive Collision Packets */

/* This register provides the number of Tx packets aborted because of carrier sense error */
#define TX_CARRIER_ERROR_PKTS         0x0760
    #define TXCARR_MASK                   (0xFFFFFFFF)           /* Tx Carrier Error Packets */

/* This register provides the number of bytes, exclusive of preamble, only in good packets of Tx */
#define TX_OCTET_COUNT_G              0x0764
    #define TXOCTG_MASK                    (0xFFFFFFFF)          /* Tx Octet Count Good */

/* This register provides the number of good Tx packets. */
#define TX_PKT_COUNT_G                0x0768
    #define TXPKTG_MASK                    (0xFFFFFFFF)          /* Tx Packet Count Good */

/* This register provides the number of Tx packets aborted because of excessive deferral error */
#define TX_EXCESSIVE_DEFERRAL_ERROR   0x076C
    #define TXEXSDEF_MASK                  (0xFFFFFFFF)            /* Tx Excessive Deferral Error */

/* This register provides the number of good Pause packets of Tx */
#define TX_PAUSEPACKETS               0x0770
    #define TXPAUSE_MASK                   (0xFFFFFFFF)            /* Tx Pause Packets */

/* This register provides the number of good VLAN packets of Tx */
#define TX_VLANPACKETS_G              0x0774
    #define TXVLANG_MASK                   (0xFFFFFFFF)            /* Tx VLAN Packets Good. */

/* This register provides the number of Tx packets without errors and with length greater than the maxsize */
#define TX_OSIZEPACKETS_G             0x0778
    #define TXOSIGG_MASK                   (0xFFFFFFFF)            /* Tx OSize Packets Good. */

/* This register provides the number of good and bad packets of Rx */
#define RX_PKT_COUNT_GB               0x0780
    #define RXPKTGB_MASK                   (0xFFFFFFFF)            /* Rx Packets Count Good. Bad. */

/* This register provides the number of bytes, exclusive of preamble, in good and bad packets of Rx */
#define RX_OCTET_COUNT_GB             0x0784
    #define RXOCTGB_MASK                   (0xFFFFFFFF)            /* Rx Octet Count Good Bad. */

/* This register provides the number of bytes, exclusive of preamble, only in good packets of Rx */
#define RX_OCTET_COUNT_G              0x0788
    #define RXOCTG_MASK                    (0xFFFFFFFF)            /* Rx Octet Count Good */

/* This register provides the number of good broadcast packets of Rx */
#define RX_BROADCAST_PKTS_G           0x078C
    #define RXBCASTG_MASK                  (0xFFFFFFFF)            /* Rx Broadcast Packets Good. */

/* This register provides the number of good multicast packets of Rx */
#define RX_MULTICAST_PKTS_G           0x0790

/* This register provides the number of Rx packets with CRC error */
#define RX_CRC_ERROR_PKTS             0x0794

/* This register provides the number of Rx packets with alignment error */
#define RX_ALIGNMENT_ERROR_PKTS       0x0798

/* This register provides the number of Rx packets with runt error */
#define RX_RUNT_ERROR_PKTS            0x079C

/* This register provides the number of giant packets of Rx with length greater than 1,518 bytes and with CRC error */
#define RX_JABBER_ERROR_PKTS          0x07A0

/* This register provides the number of Rx packets with length less than 64 bytes, without any errors */
#define RX_UNDERSIZE_PKTS_G           0x07A4

/* This register provides the number of Rx packets without errors, with length greater than the maxsize */
#define RX_OVERSIZE_PKTS_G            0x07A8

/* This register provides the number of good and bad packets of Rx with length 64 bytes, exclusive of the preamble */
#define RX_64OCTETS_PKTS_GB           0x07AC

/* This register provides the number of good and bad packets of Rx with length between 65 and 127bytes, exclusive of the preamble */
#define RX_65TO127OCTETS_PKTS_GB      0x07B0

/* This register provides the number of good and bad packets of Rx with length between 128 and 255 bytes, exclusive of the preamble. */
#define RX_128TO255OCTETS_PKTS_GB     0x07B4

/* This register provides the number of good and bad packets of Rx with length between 256 and 511 bytes, exclusive of the preamble */
#define RX_256TO511OCTETS_PKTS_GB     0x07B8

/* This register provides the number of good and bad packets of Rx with length between 512 and 1023 bytes, exclusive of the preamble */
#define RX_512TO1023OCTETS_PKTS_GB    0x07BC

/* This register provides the number of good and bad packets of Rx with length between 1024 and maxsize bytes, exclusive of the preamble */
#define RX_1024TOMAXOCTETS_PKTS_GB    0x07C0

/* This register provides the number of good unicast packets received by DWC_ether_qos. */
#define RX_UNICAST_PKTS_G             0x07C4

/* This register provides the number of Rx packets with length error, for all packets with valid length field */
#define RX_LENGTH_ERROR_PKTS          0x07C8

/* This register provides the number of Rx packets with length field not equal to the valid packet size */
#define RX_OUT_OF_RANGE_TYPE_PKTS     0x07CC

/* This register provides the number of good and valid Pause packets of Rx. */
#define RX_PAUSEPACKETS               0x07D0

/* This register provides the number of missed Rx packets because of FIFO overflow */
#define RX_FIFO_OVERFLOWPACKETS       0x07D4

/* This register provides the number of good and bad VLAN packets of Rx */
#define RX_VLANPACKETS_GB             0x07D8

/* This register provides the number of Rx packets with watchdog timeout error */
#define RX_WATCHDOG_ERRORPACKETS      0x07DC

/* This register provides the number of Rx packets with Receive error or Packet Extension error on the GMII or MII interface */
#define RX_RECEIVE_ERRORPACKETS       0x07E0

/* This register provides the number of good control packets of Rx */
#define RX_CONTROL_PKTS_G             0x07E4

/* This register provides the number of microseconds Tx LPI is asserted. */
#define TX_LPI_USEC_CNTR              0x07EC

/* This register provides the number of entered times of Tx LPI */
#define TX_LPI_TRAN_CNTR              0x07F0

/* This register provides the number of microseconds Rx LPI is sampled. */
#define RX_LPI_USEC_CNTR              0x07F4

/* This register provides the number of entered times of Rx LPI */
#define RX_LPI_TRAN_CNTR              0x07F8

/* This register maintains the mask for the interrupt generated from the receive IPC statistic counters */
#define MMC_IPC_RX_INTR_MASK          0x0800
    #define RXICMPEROIM                   (1 << 29)                /* MMC Receive ICMP Error Octet Counter Interrupt Mask. */
    #define RXICMPGOIM                    (1 << 28)                /* MMC Receive ICMP Good Octet Counter Interrupt Mask. */
    #define RXTCPEROIM                    (1 << 27)                /* MMC Receive TCP Error Octet Counter Interrupt Mask. */
    #define RXTCPGOIM                     (1 << 26)                /* MMC Receive TCP Good Octet Counter Interrupt Mask. */
    #define RXUDPEROIM                    (1 << 25)                /* MMC Receive UDP Error Octet Counter Interrupt Mask. */
    #define RXUDPGOIM                     (1 << 24)                /* MMC Receive UDP Good Octet Counter Interrupt Mask. */
    #define RXIPV6NOPAYOIM                (1 << 23)                /* MMC Receive IPV6 No Payload Octet Counter Interrupt Mask. */
    #define RXIPV6HEROIM                  (1 << 22)                /* MMC Receive IPV6 Header Error Octet Counter Interrupt Mask. */
    #define RXIPV6GOIM                    (1 << 21)                /* MMC Receive IPV6 Good Octet Counter Interrupt Mask. */
    #define RXIPV4UDSBLOIM                (1 << 20)                /* MMC Receive IPV4 UDP Checksum Disabled Octet Counter Interrupt Mask. */
    #define RXIPV4FRAGOIM                 (1 << 19)                /* MMC Receive IPV4 Fragmented Octet Counter Interrupt Mask. */
    #define RXIPV4NOPAYOIM                (1 << 18)                /* MMC Receive IPV4 No Payload Octet Counter Interrupt Mask. */
    #define RXIPV4HEROIM                  (1 << 17)                /* MMC Receive IPV4 Header Error Octet Counter Interrupt Mask. */
    #define RXIPV4GOIM                    (1 << 16)                /* MMC Receive IPV4 Good Octet Counter Interrupt Mask. */
    #define RXICMPERPIM                   (1 << 13)                /* MMC Receive ICMP Error Packet Counter Interrupt Mask. */
    #define RXICMPGPM                     (1 << 12)                /* MMC Receive ICMP Good Packet Counter Interrupt Mask. */
    #define RXTCPERPIM                    (1 << 11)                /* MMC Receive TCP Error Packet Counter Interrupt Mask. */
    #define RXTCPGPIM                     (1 << 10)                /* MMC Receive TCP Good Packet Counter Interrupt Mask. */
    #define RXUDPERPIM                    (1 <<  9)                /* MMC Receive UDP Error Packet Counter Interrupt Mask. */
    #define RXUDPGPIM                     (1 <<  8)                /* MMC Receive UDP Good Packet Counter Interrupt Mask. */
    #define RXIPV6NOPAYPIM                (1 <<  7)                /* MMC Receive IPV6 No Payload Packet Counter Interrupt Mask. */
    #define RXIPV6HERPIM                  (1 <<  6)                /* MMC Receive IPV6 Header Error Packet Counter Interrupt Mask. */
    #define RXIPV6GPIM                    (1 <<  5)                /* MMC Receive IPV6 Good Packet Counter Interrupt Mask. */
    #define RXIPV4UDSBLPIM                (1 <<  4)                /* MMC Receive IPV4 UDP Checksum Disable Counter Interrupt Mask. */
    #define RXIPV4FRAGPIM                 (1 <<  3)                /* MMC Receive IPV4 Fragmented Packet Counter Interrupt Mask. */
    #define RXIPV4NOPAYPIM                (1 <<  2)                /* MMC Receive IPV4 No Payload Packet Counter Interrupt Mask. */
    #define RXIPV4HERPIM                  (1 <<  1)                /* MMC Receive IPV4 Header Error Packet Counter Interrupt Mask. */
    #define RXIPV4GPIM                    (1 <<  0)                /* MMC Receive IPV4 Good Packet Counter Interrupt Mask. */

/* This register maintains the interrupt that the receive IPC statistic counters generate */
#define MMC_IPC_RX_INTR               0x0808
    #define RXICMPEROIS                   (1 << 29)                /* MMC Receive ICMP Error Octet Counter Interrupt Status. */
    #define RXICMPGOIS                    (1 << 28)                /* MMC Receive ICMP Good Octet Counter Interrupt Status. */
    #define RXTCPEROIS                    (1 << 27)                /* MMC Receive TCP Error Octet Counter Interrupt Status. */
    #define RXTCPGOIS                     (1 << 26)                /* MMC Receive TCP Good Octet Counter Interrupt Status. */
    #define RXUDPEROIS                    (1 << 25)                /* MMC Receive UDP Error Octet Counter Interrupt Status. */
    #define RXUDPGOIS                     (1 << 24)                /* MMC Receive UDP Good Octet Counter Interrupt Status. */
    #define RXIPV6NOPAYOIS                (1 << 23)                /* MMC Receive IPV6 No Payload Octet Counter Interrupt Status. */
    #define RXIPV6HEROIS                  (1 << 22)                /* MMC Receive IPV6 Header Error Octet Counter Interrupt Status. */
    #define RXIPV6GOIS                    (1 << 21)                /* MMC Receive IPV6 Good Octet Counter Interrupt Status. */
    #define RXIPV4UDSBLOIS                (1 << 20)                /* MMC Receive IPV4 UDP Checksum Disabled Octet Counter Interrupt Status. */
    #define RXIPV4FRAGOIS                 (1 << 19)                /* MMC Receive IPV4 Fragmented Octet Counter Interrupt Status. */
    #define RXIPV4NOPAYOIS                (1 << 18)                /* MMC Receive IPV4 No Payload Octet Counter Interrupt Status. */
    #define RXIPV4HEROIS                  (1 << 17)                /* MMC Receive IPV4 Header Error Octet Counter Interrupt Status. */
    #define RXIPV4GOIS                    (1 << 16)                /* MMC Receive IPV4 Good Octet Counter Interrupt Status. */
    #define RXICMPERPIS                   (1 << 13)                /* MMC Receive ICMP Error Packet Counter Interrupt Status. */
    #define RXICMPGPS                     (1 << 12)                /* MMC Receive ICMP Good Packet Counter Interrupt Status. */
    #define RXTCPERPIS                    (1 << 11)                /* MMC Receive TCP Error Packet Counter Interrupt Status. */
    #define RXTCPGPIS                     (1 << 10)                /* MMC Receive TCP Good Packet Counter Interrupt Status. */
    #define RXUDPERPIS                    (1 <<  9)                /* MMC Receive UDP Error Packet Counter Interrupt Status. */
    #define RXUDPGPIS                     (1 <<  8)                /* MMC Receive UDP Good Packet Counter Interrupt Status. */
    #define RXIPV6NOPAYPIS                (1 <<  7)                /* MMC Receive IPV6 No Payload Packet Counter Interrupt Status. */
    #define RXIPV6HERPIS                  (1 <<  6)                /* MMC Receive IPV6 Header Error Packet Counter Interrupt Status. */
    #define RXIPV6GPIS                    (1 <<  5)                /* MMC Receive IPV6 Good Packet Counter Interrupt Status. */
    #define RXIPV4UDSBLPIS                (1 <<  4)                /* MMC Receive IPV4 UDP Checksum Disable Counter Interrupt Status. */
    #define RXIPV4FRAGPIS                 (1 <<  3)                /* MMC Receive IPV4 Fragmented Packet Counter Interrupt Status. */
    #define RXIPV4NOPAYPIS                (1 <<  2)                /* MMC Receive IPV4 No Payload Packet Counter Interrupt Status. */
    #define RXIPV4HERPIS                  (1 <<  1)                /* MMC Receive IPV4 Header Error Packet Counter Interrupt Status. */
    #define RXIPV4GPIS                    (1 <<  0)                /* MMC Receive IPV4 Good Packet Counter Interrupt Status. */

/* This register provides the number of good IPv4 datagrams of Rx with the TCP, UDP, or ICMP payload */
#define RXIPV4_GOOD_PKTS              0x0810

/* This register provides the number of IPv4 datagrams of Rx with header errors. */
#define RXIPV4_HEADER_ERROR_PKTS      0x0814

/* This register provides the number of IPv4 datagram packets of Rx that did not have a TCP, UDP, or ICMP payload. */
#define RXIPV4_NO_PAYLOAD_PKTS        0x0818

/* This register provides the number of good IPv4 datagrams of Rx with fragmentation */
#define RXIPV4_FRAGMENTED_PKTS        0x081C

/* This register provides the number of good IPv4 datagrams of Rx that had a UDP payload with checksum disabled */
#define RXIPV4_UDP_CHECKSUM_DISABLE_PKTS    0x0820

/* This register provides the number of good IPv6 datagrams of Rx with the TCP, UDP, or ICMP payload */
#define RXIPV6_GOOD_PKTS              0x0824

/* This register provides the number of IPv6 datagrams of Rx with header errors */
#define RXIPV6_HEADER_ERROR_PKTS      0x0828

/* This register provides the number of IPv6 datagram packets of Rx that did not have a TCP, UDP, or ICMP payload */
#define RXIPV6_NO_PAYLOAD_PKTS        0x082C

/* This register provides the number of good IP datagrams of Rx with a good UDP payload */
#define RXUDP_GOOD_PKTS               0x0830

/* This register provides the number of good IP datagrams of Rx whose UDP payload has a checksum error. */
#define RXUDP_ERROR_PKTS              0x0834

/* This register provides the number of good IP datagrams of Rx with a good TCP payload */
#define RXTCP_GOOD_PKTS               0x0838

/* This register provides the number of good IP datagrams of Rx whose TCP payload has a checksum error */
#define RXTCP_ERROR_PKTS              0x083C

/* This register provides the number of good IP datagrams of Rx with a good ICMP payload */
#define RXICMP_GOOD_PKTS              0x0840

/* This register provides the number of good IP datagrams of Rx whose ICMP payload has a checksum error */
#define RXICMP_ERROR_PKTS             0x0844

/* This register provides the number of bytes in good IPv4 datagrams of Rx encapsulating TCP, UDP, or ICMP data */
#define RXIPV4_GOOD_OCTETS            0x0850

/* This register provides the number of bytes in IPv4 datagrams of Rx with header errors */
#define RXIPV4_HEADER_ERROR_OCTETS    0x0854

/* This register provides the number of bytes in IPv4 datagrams of Rx that did not have a TCP, UDP, or ICMP payload */
#define RXIPV4_NO_PAYLOAD_OCTETS      0x0858

/* This register provides the number of bytes in fragmented IPv4 datagrams of Rx */
#define RXIPV4_FRAGMENTED_OCTETS      0x085C

/* This register provides the number of bytes in a UDP segment of Rx that had the UDP checksum disabled */
#define RXIPV4_UDP_CHECKSUM_DISABLE_OCTETS    0x0860

/* This register provides the number of bytes in good IPv6 datagrams of Rx encapsulating TCP, UDP, or ICMP data. */
#define RXIPV6_GOOD_OCTETS            0x0864

/* This register provides the number of bytes in IPv6 datagrams of Rx with header errors */
#define RXIPV6_HEADER_ERROR_OCTETS    0x0868

/* This register provides the number of bytes in IPv6 datagrams of Rx that did not have a TCP, UDP, or ICMP payload */
#define RXIPV6_NO_PAYLOAD_OCTETS      0x086C

/* This register provides the number of bytes in a good UDP segment of Rx */
#define RXUDP_GOOD_OCTETS             0x0870

/* This register provides the number of bytes in a UDP segment of Rx that had checksum errors */
#define RXUDP_ERROR_OCTETS            0x0874

/* This register provides the number of bytes in a good TCP segment of Rx */
#define RXTCP_GOOD_OCTETS             0x0878

/* This register provides the number of bytes in a TCP segment of Rx that had checksum errors */
#define RXTCP_ERROR_OCTETS            0x087C

/* This register provides the number of bytes in a good ICMP segment of Rx. */
#define RXICMP_GOOD_OCTETS            0x0880

/* This register provides the number of bytes in a ICMP segment of Rx that had checksum errors */
#define RXICMP_ERROR_OCTETS           0x0884

/* The Layer 3 and Layer 4 Control register */
#define MAC_L3_L4_CTRL(i)         (0x0900 + ((i)*0x0030)) /* i={0...L3_L4_FILTER_NUM-1} */
    #define DMCHEN0                   (1 << 28)                /* DMA Channel Select Enable */
    #define DMCHN0_MASK               (0x7 << 24)              /* DMA Channel Number */
    #define L4DPIM0                   (1 << 21)                /* Layer 4 Destination Port Inverse Match Enable. */
    #define L4DPM0                    (1 << 20)                /* Layer 4 Destination Port Match Enable. */
    #define L4SPIM0                   (1 << 19)                /* Layer 4 Source Port Inverse Match Enable  */
    #define L4SPM0                    (1 << 18)                /* Layer 4 Source Port Match Enable  */
    #define L4PEND                    (1 << 16)                /* Layer 4 Protocol Enable */
    #define L3HDBM0_MASK              (0x1F << 11)             /* Layer 3 IP DA Higher Bits Match */
    #define L3HSBM0_MASK              (0x1F << 6)              /* Layer 3 IP SA Higher Bits Match */
    #define L3DAIM0                   (1 <<  5)                /* Layer 3 IP DA Inverse Match Enable */
    #define L3DAM0                    (1 <<  4)                /* Layer 3 IP DA Match Enable */
    #define L3SAIM0                   (1 <<  3)                /* Layer 3 IP SA Inverse Match Enable */
    #define L3SAM0                    (1 <<  2)                /* Layer 3 IP SA Match Enable */
    #define L3PEND                    (1 <<  0)                /* Layer 3 Protocol Enable */

/* The Layer 4 Address register */
#define MAC_LAYER4_ADDR(i)        (0x0904 + ((i)*0x0030)) /* i={0...L3_L4_FILTER_NUM-1} */
    #define L4DP0_MASK                (0xFFFF << 16)           /* Layer 4 Destination Port Number Field */
    #define L4SP0_MASK                (0xFFFF << 0)            /* Layer 4 Source Port Number Field */

/* The Layer 3 Address 0 Register */
#define MAC_LAYER3_ADDR0(i)       (0x0910 + ((i)*0x0030)) /* i={0...L3_L4_FILTER_NUM-1} */

/* The Layer 3 Address 1 Register */
#define MAC_LAYER3_ADDR_1(i)      (0x0914 + ((i)*0x0030)) /* i={0...L3_L4_FILTER_NUM-1} */

/* The Layer 3 Address 2 Register */
#define MAC_LAYER3_ADDR_2(i)      (0x0918 + ((i)*0x0030)) /* i={0...L3_L4_FILTER_NUM-1} */

/* The Layer 3 Address 3 Register */
#define MAC_LAYER3_ADDR_3(i)      (0x091C + ((i)*0x0030)) /* i={0...L3_L4_FILTER_NUM-1} */

/* This register controls the operation of the System Time generator and processing of PTP packets for timestamping in the Receiver */
#define MAC_TIMESTAMP_CTRL        0x0B00
    #define AV8021ASMEN               (1 << 28)                /* AV 802.1AS Mode Enable */
    #define TXTSSTSM                  (1 << 24)                /* Transmit Timetamp Status Mode */
    #define ESTI                      (1 << 20)                /* External System Time Input */
    #define CSC                       (1 << 19)                /* Enable checksum correction during OST to PTP over UDP/IPv4 packets. */
    #define TSENMACADDR               (1 << 18)                /* Enable MAC Address to PTP Packet Filtering. */
    #define SNAPTYPSEL_MASK           (0x3 << 16)              /* Select PTP packets for Taking Snapshots */
    #define TSMSTRENA                 (1 << 15)                /* Enable Snapshot to Messages Relevant to Master */
    #define TSEVNTENA                 (1 << 14)                /* Enable Timestamp Snapshot for Event Messages */
    #define TSIPV4ENA                 (1 << 13)                /* Enable Processing of PTP Packets Sent over IPv4-UDP */
    #define TSIPV6ENA                 (1 << 12)                /* Enable Processing of PTP Packets Sent over IPv6-UDP */
    #define TSIPENA                   (1 << 11)                /* Enable Proceesing of PTP over Ethernet Packets */
    #define TSVER2ENA                 (1 << 10)                /* Enable PTP Packet Processing for Version 2 Format */
    #define TSCTRLSSR                 (1 <<  9)                /* Timestamp Digital or Binary Rollover Control */
    #define TSENALL                   (1 <<  8)                /* Enable Timestamp for All Packets. */
    #define TSADDREG                  (1 <<  5)                /* Update Addend Register */
    #define TSTRIG                    (1 <<  4)                /* Enable Timestamp Interrupt Trigger */
    #define TSUPDT                    (1 <<  3)                /* Update Timestamp */
    #define TSINIT                    (1 <<  2)                /* Initialize Timestamp */
    #define TSCFUPDT                  (1 <<  1)                /* Fine or Coarse Timestamp Update */
    #define TSENA                     (1 <<  0)                /* Enable Timestamp */

#define MAC_SUB_SECOND_INC        0x0B04       /* The Sub-Second Increment register */
    #define SSINC_MASK                (0xFF << 16)             /* Sub-second Increment Value. */
    #define SNSINC_MASK               (0xFF << 8)              /* Sub-nanosecond Incremnet Value */

#define MAC_SYSTEM_TIME_SEC       0x0B08       /* The System Time Seconds register */

#define MAC_SYSTEM_TIME_NANO      0x0B0C       /* The System Time Nanoseconds register */
    #define TSSS_MASK                 (0x7FFFFFFF)             /* Timestamp Sub Seconds */

#define MAC_SYSTEM_TIME_SEC_UP    0x0B10      /* The System Time Seconds Update register */

#define MAC_SYSTEM_TIME_NANO_UP   0x0B14      /* The System Time Nanoseconds Update register */
    #define ADDSUB                    (1 << 31)                /* Add or Subtract Time */
    #define TSSS_MASK                 (0x7FFFFFFF)             /* Timestamp Sub Seconds */

/* The Timestamp Addend register */
#define MAC_TIMESTAMPADDEND       0x0B18

/* The Timestamp Status register */
#define MAC_TIMESTAMPSTATUS       0x0B20
    #define ATSNS_MASK                (0x1F << 25)             /* Number of Auxiliary Timestamp Snapshots */
    #define ATSSTM                    (1 << 24)                /* Auxiliary Timestamp Snapshot Trigger Missed */
    #define ATSSTN_MASK               (0xF << 16)              /* Auxiliary Timestamp Snapshot Trigger Identifier */
    #define TXTSSIS                   (1 << 15)                /* Tx Timestamp Status Interrupt Status */
    #define TSTRGTERR3                (1 <<  9)                /* Timestamp Target Time Error */
    #define TSTARGT3                  (1 <<  8)                /* Timestamp Target Time Reached for Target Time PPS3 */
    #define TSTRGTERR2                (1 <<  7)                /* Timestamp Target Time Error */
    #define TSTARGT2                  (1 <<  6)                /* Timestamp Target Time Reached for Target Time PPS2 */
    #define TSTRGTERR1                (1 <<  5)                /* Timestamp Target Time Error */
    #define TSTARGT1                  (1 <<  4)                /* Timestamp Target Time Reached for Target TIme PPS1 */
    #define TSTRGTERR0                (1 <<  3)                /* Timestamp Target Time Error */
    #define AUXTSTRIG                 (1 <<  2)                /* Auxiliary Timestamp Trigger Snapshot */
    #define TSTARGT0                  (1 <<  1)                /* Timestamp Target Time Reached */
    #define TSSOVF                    (1 <<  0)                /* Timestamp Seconds Overflow */

/* This register contains the nanosecond part of timestamp captured for Transmit packets when Tx status is disabled */
#define MAC_TX_TIMESTAMP_STS_NANO  0x0B30
    #define TXTSSMIS                      (1 << 31)            /* Transmit Timestamp Status Missed */
    #define TXTSSLO_MASK                  (0x7FFFFFFF)         /* Transmit Timestamp Status Low */

/* The register contains the higher 32 bits of the timestamp captured when a PTP packet is transmitted */
#define MAC_TX_TIMESTAMP_STS_SEC   0x0B34

#define MAC_AUXILIARY_CTRL         0x0B40      /* The Auxiliary Timestamp Control register */
    #define ATSEN3                     (1 << 7)                /* Auxiliary Snapshot 3 Enable */
    #define ATSEN2                     (1 << 6)                /* Auxiliary Snapshot 2 Enable */
    #define ATSEN1                     (1 << 5)                /* Auxiliary Snapshot 1 Enable */
    #define ATSEN0                     (1 << 4)                /* Auxiliary Snapshot 0 Enable */
    #define ATSFC                      (1 << 0)                /* Auxiliary Snapshot FIFO Clear */

/* The Auxiliary Timestamp Nanoseconds register */
#define MAC_AUXILIARY_TIMESTAMP_NANO 0x0B48
    #define AUXTSLO_MASK                 (0x7FFFFFFF)          /* Auxiliary Timestamp */

/* The Auxiliary Timestamp - Seconds register */
#define MAC_AUXILIARY_TIMESTAMP_SEC  0x0B4C
    #define AUXTSHI_MASK                 (0x7FFFFFFF)          /* Auxiliary Timestamp */

/* The MAC Timestamp Ingress Asymmetry Correction register */
#define MAC_TIMESTAMP_INGRESS_ASYM_CORR  0x0B50

/* The MAC Timestamp Egress Asymmetry Correction register. */
#define MAC_TIMESTAMP_EGRESS_ASYM_CORR   0x0B54

/* The MAC Timestamp Ingress Correction Nanosecond register */
#define MAC_TIMESTAMP_INGRESS_CORR_NANO  0x0B58

/* The MAC Timestamp Egress Correction Nanosecond register */
#define MAC_TIMESTAMP_EGRESS_CORR_NANO   0x0B5C

/* The MAC Timestamp Ingress Correction Sub-Nanosecond register. */
#define MAC_TIMESTAMP_INGRESS_CORR_SUBNANO     0x0B60
    #define TSICSNS_MASK                           (0xFFFF << 8)            /* Timestamp Ingress Correction, sub-nanoseconds */

/* The MAC Timestamp Egress Correction Sub-Nanosecond register */
#define MAC_TIMESTAMP_EGRESS_CORR_SUBNANO      0x0B64
    #define TSECSNS_MASK                           (0xFFFF << 8)            /* Timestamp Egress Correction, sub-nanoseconds. */

#define MAC_PPS_CONTROL            0x0B70      /* The PPS Control register */
    #define TRGTMODSEL3_MASK           (0x3 << 29)            /* Target Time Register Mode for PPS3 Output */
    #define PPSCMD3_MASK               (0x7 << 24)            /* Flexible PPS3 Output Control */
    #define TRGTMODSEL2_MASK           (0x3 << 21)            /* Target Time Register Mode for PPS2 Output */
    #define PPSCMD2_MASK               (0x7 << 16)            /* Flexible PPS2 Output Control */
    #define TRGTMODSEL1_MASK           (0x3 << 13)            /* Target Time Register Mode for PPS1 Output */
    #define PPSCMD1_MASK               (0x7 << 8)             /* Flexible PPS1 Output Control */
    #define TRGTMODSEL0_MASK           (0x3 << 5)             /* Target Time Register Mode for PPS0 Output */
    #define PPSEN0                     (1 << 4)               /* Flexible PPS Output Mode Enable */
    #define PPSCTRL_PPSCMD_MASK        (0xF << 0)             /* PPS Output Frequency Control */

#define MAC_PPS0_TARGET_TIME_SECS  0x0B80      /* The PPS Target Time Seconds register */

#define MAC_PPS0_TARGET_TIME_NANOS 0x0B84      /* The PPS0 Target Time Nanoseconds register */
    #define TRGTBUSY0                  (1 << 31)              /* PPS Target Time Register Busy */
    #define TTSL0_MASK                 (0x7FFFFFFF)           /* Target Time Low to PPS Register. */

#define MAC_PPS0_INTERVAL          0x0B88      /* The PPS0 Interval register */

/* The PPS0 Width register */
#define MAC_PPS0_WIDTH             0x0B8C

#define MAC_PTO_CONTROL            0x0BC0     /* This register controls the PTP Offload Engine operation */
    #define DN_MASK                    (0xFFFF << 8)           /* Domain Number */
    #define PDRDIS                     (1 << 7)                /* Reserved */
    #define DRRDIS                     (1 << 6)                /* Disable PTO Delay Request/Response response generation */
    #define APDREQTRIG                 (1 << 5)                /* Automatic PTP Pdelay_Req message Trigger */
    #define ASYNCTRG                   (1 << 4)                /* Automatic PTP SYNC message Trigger */
    #define APDREQEN                   (1 << 2)                /* Automatic PTP Pdelay_Req message Enable */
    #define ASYNCEN                    (1 << 1)                /* Automatic PTP SYNC message Enable */
    #define PTO_PTOEN                  (1 << 0)                /* PTP Offload Enable */

/* This register contains Bits[31:0] of the 80-bit Source Port Identity of the PTP node */
#define MAC_SOURCE_PORT_IDENT0    0x0BC4

/* This register contains Bits[63:32] of the 80-bit Source Port Identity of the PTP node. */
#define MAC_SOURCE_PORT_IDENT1    0x0BC8

/* This register contains Bits[79:64] of the 80-bit Source Port Identity of the PTP node. */
#define MAC_SOURCE_PORT_IDENT2    0x0BCC

/* This register contains the periodic intervals for automatic PTP packet generation. */
#define MAC_LOG_MESSAGE_INTERVAL  0x0BD0
    #define LMPDRI_MASK               (0xFF << 24)            /* Log Min Pdelay_Req Interval */
    #define DRSYNCR_MASK              (0x7 << 8)              /* Delay_Req to SYNC Ratio */
    #define LSI_MASK                  (0xFF << 0)             /* Log Sync Interval */

#define MTL_OPERATION_MODE        0x0C00  /* The Operation Mode register */
    #define CNTCLR                    (1 << 9)                /* Counters Reset */
    #define MTL_CNTPRST               (1 << 8)                /* Counters Preset */
    #define SCHALG_MASK               (0x3 << 5)              /* Tx Scheduling Algorithm */
    #define SCHALG(x)                 ((x << 5) & SCHALG_MASK)    /* Tx Scheduling Algorithm */
    #define RAA                       (1 <<  2)               /* Receive Arbitration Algorithm */

#define MTL_OPERATION_MODE_SCHALG_STRICT     SCHALG(0x3)

#define MTL_DEBUG_STATUS          0x0C0C

#define MTS_INTR_STATUS           0x0C20       /* The interrupt status register */
    #define DBGIS                     (1 << 17)                /* Debug Interrupt Status */
    #define MTS_MACIS                 (1 << 16)                /* MAC Interrupt Status */
    #define Q7IS                      (1 <<  7)                /* Queue 7 Interrupt status */
    #define Q6IS                      (1 <<  6)                /* Queue 6 Interrupt status */
    #define Q5IS                      (1 <<  5)                /* Queue 5 Interrupt status */
    #define Q4IS                      (1 <<  4)                /* Queue 4 Interrupt status */
    #define Q3IS                      (1 <<  3)                /* Queue 3 Interrupt status */
    #define Q2IS                      (1 <<  2)                /* Queue 2 Interrupt status */
    #define Q1IS                      (1 <<  1)                /* Queue 1 Interrupt status */
    #define Q0IS                      (1 <<  0)                /* Queue 0 Interrupt status */

/* The Receive QUeue and DMA Channel Mapping 0 register */
#define MTL_RXQ_DMA_MAP0          0x0C30
    #define Q3DDMACH                  (1 << 28)                /* Queue 3 Enabled to Dynamic (per packet) DMA Channel Selection */
    #define Q3MDMACH_MASK             (0x7 << 24)              /* Queue 3 Mapped to DMA Channel */
    #define Q2DDMACH                  (1 << 20)                /* Queue 2 Enabled to Dynamic (per packet) DMA Channel Selection */
    #define Q2MDMACH_MASK             (0x7 << 16)              /* Queue 2 Mapped to DMA Channel */
    #define Q1DDMACH                  (1 << 12)                /* Queue 1 Enabled to Dynamic (per packet) DMA Channel Selection */
    #define Q1MDMACH_MASK             (0x7 << 8)               /* Queue 1 Mapped to DMA Channel */
    #define Q0DDMACH                  (1 << 4)                 /* Queue 0 Enabled to Dynamic (per packet) DMA Channel Selection */
    #define Q0MDMACH_MASK             (0x7 << 0)               /* Queue 0 Mapped to DMA Channel */

/* The Receive QUeue and DMA Channel Mapping 1 register */
#define MTL_RXQ_DMA_MAP1          0x0C34
    #define Q7DDMACH                  (1 << 28)                /* Queue 7 Enabled to Dynamic (per packet) DMA Channel Selection */
    #define Q7MDMACH_MASK             (0x7 << 24)              /* Queue 7 Mapped to DMA Channel */
    #define Q6DDMACH                  (1 << 20)                /* Queue 6 Enabled to Dynamic (per packet) DMA Channel Selection */
    #define Q6MDMACH_MASK             (0x7 << 16)              /* Queue 6 Mapped to DMA Channel */
    #define Q5DDMACH                  (1 << 12)                /* Queue 5 Enabled to Dynamic (per packet) DMA Channel Selection */
    #define Q5MDMACH_MASK             (0x7 << 8)               /* Queue 5 Mapped to DMA Channel */
    #define Q4DDMACH                  (1 << 4)                 /* Queue 4 Enabled to Dynamic (per packet) DMA Channel Selection */
    #define Q4MDMACH_MASK             (0x7 << 0)               /* Queue 4 Mapped to DMA Channel */

/* The Queue i Transmit Operation Mode register */
#define MTL_TXQi_OPERATION_MODE(i)      (0x0D00 + ((i) * 0x0040))   /* i = {1...TXQCNT-1} */
    #define TQS_MASK                        (0x7F << 16)            /* Transmit Queue Size */
    #define TTC_MASK                        (0x7 << 4)              /* Transmit Threshold Control */
    #define TXQEN_MASK                      (0x3 << 2)              /* Transmit Queue Enable */
    #define TXQEN(x)                        (x << 2)                /* Transmit Queue Enable */
    #define TX_EN                           0x2                     /* Transmit Queue Enable */
    #define TX_AV_EN                        0x1                     /* Transmit Queue Enable in AV mode */
    #define TX_DEN                          0x0                     /* Transmit Queue Not Enabled */
    #define TSF                             (1 << 1)                /* Transmit Store and Forward */
    #define FTQ                             (1 << 0)                /* Flush Transmit Queue */
    #define TQS(x)                          (x << 16)               /* Transmit Queue Size */

/* The Queue i Underflow Counter register */
#define MTL_TXQi_UNDERFLOW(i)           (0x0D04 + ((i) * 0x0040))   /* i = {1...TXQCNT-1} */
    #define UFCNTOVF                       (1 << 11)                /* Overflow Bit for Underflow Packet Counter. */
    #define UFFRMCNT_MASK                  (0x7FF << 0)             /* Underflow Packet Counter */

/* The Queue i Transmit Debug register */
#define MTL_TXQi_DEBUG_STATUS(i)   (0x0D08 + ((i) * 0x0040))   /* i = {1...TXQCNT-1} */
    #define STXSTSF_MASK              (0x7 << 20)                     /* Number of Status Words in Tx Status FIFO of Queue */
    #define STXSTSF(x)                (((x) & STXSTSF_MASK) >> 20)    /* Number of Status Words in Tx Status FIFO of Queue */
    #define PTXQ_MASK                 (0x7 << 16)                     /* Number of Packets in the Transmit Queue*/
    #define PTXQ(x)                   (((x) & PTXQ_MASK) >> 16)       /* Number of Packets in the Transmit Queue */
    #define TXSTSFSTS                 (1 << 5)                        /* MTL Tx Status FIFO Full Status */
    #define TXQSTS                    (1 << 4)                        /* MTL Tx Queue Not Empty Status */
    #define TWCSTS                    (1 << 3)                        /* MTL Tx Queue Write Controller Status */
    #define TRCSTS_MASK               (0x3 << 1)                      /* MTL Tx Queue Read Controller Status*/
    #define TRCSTS(x)                 (((x) & TRCSTS_MASK) >> 1)      /* MTL Tx Queue Read Controller Status */
    #define TXQPAUSED                 (1 << 0)                        /* Transmit Queue in Pause */

/* The Queue i Transmit ETS Control register */
#define MTL_TXQi_ETS_CONTROL(i)         (0x0D10 + ((i) * 0x0040))   /* i = {1...TXQCNT-1} */
    #define SLC_MASK                        (0x7 << 4)              /* Slot Count */
    #define CC                              (1 <<  3)               /* Credit Control */
    #define AVALG                           (1 <<  2)               /* AV Algorithm  */

/* The Queue i Transmit ETS Status register */
#define MTL_TXQi_ETS_STATUS(i)          (0x0D14 + ((i) * 0x0040))   /* i = {1...TXQCNT-1} */
    #define ABS_MASK                        (0xFFFFFF)                      /* Average Bits per Slot */

/* The Queue i Transmit Quantum or Weights register */
#define MTL_TXQi_QUANTUM_WEIGHT(i)      (0x0D18 + ((i) * 0x0040))   /* i = {1...TXQCNT-1} */
    #define ISCQW_MASK                       (0x1FFFFF)            * Quanturn or Weights */

/* The Queue i sendSlopeCredit register */
#define MTL_TXQi_SENDSLOPECREDIT(i)     (0x0D1C + ((i) * 0x0040))   /* i = {1...TXQCNT-1} */
    #define SSC_MASK                        (0x3FFF << 0)           /* sendSlopeCredit Value */

/* The Queue i Transmit hiCredit register */
#define MTL_TXQi_HICREDIT(i)            (0x0D20 + ((i) * 0x0040))   /* i = {1...TXQCNT-1} */
    #define HC_MASK                         (0x1FFFFFFF << 0)           /* HiCredit Value */

/* The Queue i Transmit loCredit register */
#define MTL_TXQi_LOCREDIT(i)            (0x0D24 + ((i) * 0x0040))  /* i = {1...TXQCNT-1} */
    #define LC_MASK                         (0x1FFFFFFF << 0)      /* LoCredit Value */

/* The Queue i Interrupt Control and Status register */
#define MTL_Qi_INTR_CTRL_STS(i)         (0x0D2C + ((i) * 0x0040))  /* i = {1...MAX(TXQCNT,RXQCNT)-1} */
    #define RXOIE                     (1 << 24)                /* Receive Queue Overflow Interrupt Enable */
    #define RXOVFIS                   (1 << 16)                /* Receive Queue Overflow Interrupt Status */
    #define ABPSIE                    (1 <<  9)                /* Average Bits Per Slot Interrupt Enable */
    #define TXUIE                     (1 <<  8)                /* Transmit Queue Underflow Interrupt Enable */
    #define ABPSIS                    (1 <<  1)                /* Average Bits Per Slot Interrupt Status */
    #define TXUNFIS                   (1 <<  0)                /* Transmit Queue Underflow Interrupt Status */

/* The Queue i Receive Operation Mode register */
#define MTL_RXQi_OPERATION_MODE(i)      (0x0D30 + ((i) * 0x0040))  /* i = {1...RXQCNT-1} */
    #define RQS_MASK            (0x7F << 20)            /* Receive Queue Size Mask */
    #define RQS(x)              (x << 20)               /* Receive Queue Size */
    #define RFD_MASK            (0x3F << 14)            /* Threshold for Deactivating Flow Control Mask */
    #define RFD(x)              (x << 14)               /* Threshold for Deactivating Flow Control */
    #define RFA_MASK            (0x3F << 8)             /* Threshold for Activating Flow Control Mask */
    #define RFA(x)              (x << 8)                /* Threshold for Activating Flow Control  */
    #define EHFC                (1 << 7)                /* Enable Hardware Flow Control */
    #define DIS_TCP_EF          (1 << 6)                /* Disable Dropping of TCP/IP Checksum Error Packets */
    #define RSF                 (1 << 5)                /* Receive Queue Store and Forward */
    #define FEP                 (1 << 4)                /* Forward Error Packets */
    #define FUP                 (1 << 3)                /* Forward Undersized Good Packets */
    #define RTC_MASK            (0x3 << 0)              /* Receive Queue Threshold Control */

/* The Queue i Missed Packet and Overflow Counter register */
#define MTL_RXQi_MISSED_PKT_OVF_CNTL(i) (0x0D34 + ((i) * 0x0040))  /* i = {1...RXQCNT-1} */
    #define MISCNTOVF                       (1 << 27)         /* Missed Packet Counter Overflow Bit */
    #define MISPKTCNT_MASK                  (0x7FF << 16)     /* Missed Packet Counter */
    #define OVFCNTOVF                       (1 << 11)         /* Overflow Counter Overflow Bit */
    #define OVFPKTCNT_MASK                  (0x7FF << 0)      /* Overflow Packet Counter */

/* The Queue i Receive Debug register */
#define MTL_RXQi_DEBUG_STATUS(i)        (0x0D38 + ((i) * 0x0040))   /* i = {1...RXQCNT-1} */
    #define PRXQ_MASK                       (0x3FFF << 16)        /* Number of Packets in Receive Queue */
    #define RXQSTS_MASK                     (0x3 << 4)            /* MTL Rx Queue Fill-Level Status */
    #define RRCSTS_MASK                     (0x3 << 1)            /* MTL Rx Queue Read Controller State */
    #define RWCSTS                          (1 << 0)              /* MTL Rx Queue Write Controller Active Status */

/* The Queue i Receive Control register */
#define MTL_RXQi_CTRL(i)                (0x0D3C + ((i) * 0x0040))   /* i = {1...RXQCNT-1} */
    #define FRM_ARBIT                       (1 << 3)              /* Receive Queue Packet Arbitration */
    #define WEGT_MASK                       (0x7 << 0)            /* Receive Queue Weight */

/* -------------------------------------------------------------------------
 * DMA registers
 * -------------------------------------------------------------------------
 */
#define DMA_MODE                    0x1000      /* The Bus Mode register */
    #define INTM_MASK                   (0x3 << 16)            /* Interrupt Mode */
    #define PR_MASK                     (0x7 << 12)            /* Priority Ratio */
    #define TXPR                        (1 << 11)              /* Transmit Priority */
    #define DSPW                        (1 << 8)               /* Descriptor Posted Write */
    #define TAA_MASK                    (0x7 << 2)             /* Transmit Arbitration Algorithm */
    #define DA                          (1 << 1)               /* DMA Tx or Rx Arbitration Scheme */
    #define SWR                         (1 << 0)               /* Software Reset */

#define DMA_SYSBUS_MODE             0x1004       /* The System Bus mode register */
    #define EN_LPI                      (1 << 31)                /* Enable Low Power Interface (LPI) */
    #define LPI_XIT_PKT                 (1 << 30)                /* Unlock on Magic Packet or Remote Wake-Up Packet */
    #define WR_OSR_LMT_MASK             (0xF << 24)              /* AXI maximum Write Outstanding Request Limit */
    #define RD_OSR_LMT                  (1 << 16)                /* AXI maximum Read Outstanding Request Limit */
    #define RB                          (1 << 15)                /* Rebuild INCRx Burst */
    #define MB                          (1 << 14)                /* Mixed Burst */
    #define ONEKBBE                     (1 << 13)                /* 1KB Boundary Crossing Enable to the EQOS-AXI Master */
    #define AAL                         (1 << 12)                /* Address-Aligned Beats */
    #define EAME                        (1 << 11)                /* Enhanced Address Mode Enable */
    #define AALE                        (1 << 10)                /* Automatic AXI LPI enable */
    #define BLEN256                     (1 <<  7)                /* AXI Burst Length 256 */
    #define BLEN128                     (1 <<  6)                /* AXI Burst Length 128 */
    #define BLEN64                      (1 <<  5)                /* AXI Burst Length 64 */
    #define BLEN32                      (1 <<  4)                /* AXI Burst Length 32 */
    #define BLEN16                      (1 <<  3)                /* AXI Burst Length 16 */
    #define BLEN8                       (1 <<  2)                /* AXI Burst Length 8 */
    #define BLEN4                       (1 <<  1)                /* AXI Burst Length 4 */
    #define FB                          (1 <<  0)                /* Fixed Burst Length */

    #define RD_OSR_LIMIT_MASK           (0xF << 16)
    #define RD_OSR_LIMIT(x)             (((x) << 16) & RD_OSR_LIMIT_MASK)
    #define RD_OSR_LIMIT_DEFAULT        3

    #define WR_OSR_LIMIT_MASK           (0xF << 24)
    #define WR_OSR_LIMIT(x)             (((x) << 24) & WR_OSR_LIMIT_MASK )
    #define WR_OSR_LIMIT_DEFAULT        3

    #define BURST_MASK                  (0x3F << 1)
    #define BURST(x)                    (((x) << 1) & BURST_MASK)
    #define BURST_DEFAULT               (0xF << 1)

/* The application reads this Interrupt Status register */
#define DMA_INTR_STS              0x1008
    #define DMA_MACIS                 (1 << 17)                /* MAC Interrupt Status */
    #define MTLIS                     (1 << 16)                /* MTL Interrupt Status */
    #define DCiIS(i)                  ((1 << (i)))             /* i = {0...MAX(RXDMACNT,TXDMACNT)-1} */
    #define DC7IS                     (1 <<  7)                /* DMA Channel 7 Interrupt Status */
    #define DC6IS                     (1 <<  6)                /* DMA Channel 6 Interrupt Status */
    #define DC5IS                     (1 <<  5)                /* DMA Channel 5 Interrupt Status */
    #define DC4IS                     (1 <<  4)                /* DMA Channel 4 Interrupt Status */
    #define DC3IS                     (1 <<  3)                /* DMA Channel 3 Interrupt Status */
    #define DC2IS                     (1 <<  2)                /* DMA Channel 2 Interrupt Status */
    #define DC1IS                     (1 <<  1)                /* DMA Channel 1 Interrupt Status */
    #define DC0IS                     (1 <<  0)                /* DMA Channel 0 Interrupt Status */

#define DMA_DEBUG_STS0            0x100C
#define DMA_DEBUG_STS1            0x1010
#define DMA_DEBUG_STS2            0x1014

/* DMA channel states */
#define DMA_TX_CH_STOPPED         0
#define DMA_TX_CH_SUSPENDED       6
#define DMA_GET_TX_STATE_CH0(status0) ((status0 & 0xF000) >> 12)

/* This register is used to control the AXI4 Cache Coherency Signals for read transactions by all the Transmit DMA channels */
#define AXI4_TX_AR_ACE_CTRL       0x1020
    #define THD_MASK                  (0x3 << 20)            /* Transmit DMA First Packet Buffer or TSO Header Domain Control. */
    #define THC_MASK                  (0xF << 16)            /* Transmit DMA First Packet Buffer or TSO Header Cache Control. */
    #define TED_MASK                  (0x3 << 12)            /* Transmit DMA Extended Packet Buffer or TSO Payload Domain Control. */
    #define TEC_MASK                  (0xF << 8)             /* Transmit DMA Extended Packet Buffer or TSO Payload Cache Control. */
    #define TDRD_MASK                 (0x3 << 4)             /* Transmit DMA Read Descriptor Domain Control. */
    #define TDRC_MASK                 (0xF << 0)             /* Transmit DMA Read Descriptor Cache Control. */

/* This register is used to control the AXI4 Cache Coherency Signals for write transactions by all the Receive DMA channels */
#define AXI4_RX_AW_ACE_CTRL       0x1024
    #define RDD_MASK                  (0x3 << 28)            /* Receive DMA Buffer Domain Control */
    #define RHD_MASK                  (0x3 << 20)            /* Receive DMA Header Domain Control */
    #define RHC_MASK                  (0xF << 16)            /* Receive DMA Header Cache Control */
    #define RPD_MASK                  (0x3 << 12)            /* Receive DMA Payload Domain Control */
    #define RPC_MASK                  (0xF << 8)             /* Receive DMA Payload Cache Control */
    #define PDWD_MASK                 (0x3 << 4)             /* Receive DMA Write Descriptor Domain Control */
    #define RDWC_MASK                 (0xF << 0)             /* Receive DMA Write Descriptor Cache Control */

/* This register is used to control the AXI4 Cache Coherency Signals for Descriptor write and read transactions */
#define AXI4_TXRX_AWAR_ACE_CTRL   0x1028
    #define WRP_MASK                (0x7 << 20)            /* DMA Write Protection control */
    #define RDP_MASK                (0x7 << 16)            /* DMA Read Protection control */
    #define RDRD_MASK               (0x3 << 12)            /* Receive DMA Read Descriptor Domain control */
    #define RDRC_MASK               (0xF << 8)             /* Receive DMA Read Descriptor Cache control */
    #define TDWD_MASK               (0x3 << 4)             /* Transmit DMA Write Descriptor Domain control */
    #define TDWC_MASK               (0xF << 0)             /* Transmit DMA Write Descriptor Cache control */

/* The AXI LPI entry interval register */
#define AXI_LPI_ENTRY_INTERVAL   0x1040
    #define LPIEI_MASK              (0xF << 0)             /* LPI Entry Interval */

/* The Channel i Control register */
#define DMA_CHi_CTRL(i)           (0x1100 + ((i) * 0x0080))   /* i = {0...TXDMACNT-1} */
    #define SPH                       (1 << 24)                   /* Split Headers */
    #define DSL_MASK                  (0x7 << 18)                 /* Descriptor Sip Length */
    #define PBLX8                     (1 << 16)                   /* 8xPBL mode */
    #define MSS_MASK                  (0x3FFF << 0)               /* Maximum Segment Size */
    #define DSL_SHIFT(x)              (x << 18)                   /* Descriptor Skip Length for unchained descriptors.*/


/* The Channel i Transmit Control register */
#define DMA_CHi_TX_CTRL(i)        (0x1104  + ((i) * 0x0080))  /* i = {0...TXDMACNT-1} */
    #define TQOS_MASK                 (0xF << 24)                 /* Transmit QOS */
    #define TXPBL(x)                  (x << 16)                   /* Transmit Programmable BUrst Length */
    #define TXPBL_MASK                (0x3F << 16)                /* Transmit Programmable BUrst Length */
    #define IPBL                      (1 << 15)                   /* Ignore PBL Requirement */
    #define TSE                       (1 << 12)                   /* TCP Segmentation Enable */
    #define OSF                       (1 << 4)                    /* Operate on Second Packet */
    #define TCW_MASK                  (0x7 << 1)                  /* Transmit Channel Weight */
    #define ST                        (1 << 0)                    /* Start to Stop Transmission Command */

/* The Channel i Receive Control register */
#define DMA_CHi_RX_CTRL(i)        (0x1108  + ((i) * 0x0080))  /* i = {0...RXDMACNT-1} */
    #define RPF                       (1 << 31)                   /* DMA Rx Channel0 Packet Flush */
    #define RQOS_MASK                 (0xF << 24)                 /* Rx AXI4 QOS */
    #define RXPBL(x)                  (x << 16)                   /* Receive Programmable Burst Length */
    #define RXPBL_MASK                (0x3F << 16)                /* Receive Programmable Burst Length */
    #define RBSZ(x)                   (x << 1)                    /* Receive Programmable Burst Length */
    #define RBSZ_MASK                 (0x3FFF << 1)               /* Receive Buffer size */
    #define SR                        (1 << 0)                    /* Start or Stop Receive */

/* The Channel i Tx Descriptor List Address register */
#define DMA_CHi_TXDESC_LIST_HADDR(i)   (0x1110  + ((i) * 0x0080))   /* i = {0...TXDMACNT-1} */

/* The Channel i Tx Descriptor List Address register */
#define DMA_CHi_TXDESC_LIST_ADDR(i)   (0x1114  + ((i) * 0x0080))   /* i = {0...TXDMACNT-1} */
    #define TDESLA_MASK                   (0x3FFFFFFF << 2)             /* Start of Transmit List */

/* The Channel i Rx Descriptor List Address register. */
#define DMA_CHi_RXDESC_LIST_HADDR(i)   (0x1118  + ((i) * 0x0080))   /* i = {0...RXDMACNT-1} */

/* The Channel i Rx Descriptor List Address register. */
#define DMA_CHi_RXDESC_LIST_ADDR(i)   (0x111C  + ((i) * 0x0080))   /* i = {0...RXDMACNT-1} */
    #define RDESLA_MASK                   (0x3FFFFFFF << 2)             /* Start of Receive List */

/* The Channel i Tx Descriptor Tail Pointer register */
#define DMA_CHi_TXDESC_TAIL_PTR(i)    (0x1120  + ((i) * 0x0080))   /* i = {0...TXDMACNT-1} */
    #define TDTP_MASK                     (0x3FFFFFFF << 2)             /* Transmit Descriptor Tail Pointer */

/* The Channel i Rx Descriptor Tail Pointer register */
#define DMA_CHi_RXDESC_TAIL_PTR(i)    (0x1128  + ((i) * 0x0080))   /* i = {0...RXDMACNT-1} */
    #define RDTP_MASK                     (0x3FFFFFFF << 2)             /* Receive Descriptor Tail Pointer */

/* The Channel i Tx Descriptor Ring Length register */
#define DMA_CHi_TXDESC_RING_LEN(i)    (0x112C  + ((i) * 0x0080))  /* i = {0...TXDMACNT-1} */
    #define TDRL_MASK                     (0x3FF << 0)                  /* Transmit Descriptor Ring Length */

/* The Channel i Rx Descriptor Ring Length register */
#define DMA_CHi_RXDESC_RING_LEN(i)    (0x1130  + ((i) * 0x0080))  /* i = {0...RXDMACNT-1} */
    #define RDRL_MASK                     (0x3FF << 0)                  /* Receive Descriptor Ring Length */

/* The Channel i Interrupt Enable register */
#define DMA_CHi_INTR_EN(i)            (0x1134  + ((i) * 0x0080))   /* i = {0...TXDMACNT-1} */
    #define NIE                           (1 << 15)                /* Normal Interrupt Summary Enable */
    #define AIE                           (1 << 14)                /* Abnormal Interrupt Summary Enable */
    #define CDEE                          (1 << 13)                /* Content Descriptor Error Enable */
    #define FBEE                          (1 << 12)                /* Fatal Bus Error Enable */
    #define ERIE                          (1 << 11)                /* Early Receive Interrupt Enable */
    #define ETIE                          (1 << 10)                /* Early Transmit Interrupt Enable */
    #define RWTE                          (1 <<  9)                /* Receive Watchdog Timeout Enable */
    #define RSE                           (1 <<  8)                /* Receive Stopped Enable */
    #define RBUE                          (1 <<  7)                /* Receive Buffer Unavailable Enable */
    #define RIE                           (1 <<  6)                /* Receive Interrupt Enable */
    #define TBUE                          (1 <<  2)                /* Transmit Buffer Unavailable Enable */
    #define TXSE                          (1 <<  1)                /* Transmit Stopped Enable */
    #define TIE                           (1 <<  0)                /* Transmit Interrupt Enable */

/* The Channel i Rx Interrupt Watchdog Timer register */
#define DMA_CHi_RX_INTR_WDG_TIMER(i)  (0x1138  + ((i) * 0x0080)) /* i = {0...RXDMACNT-1} */
    #define RWTU_MASK                     (0x3 << 16)            /* Receive Interrupt Watchdog Tier Count Units  */
    #define RWT_MASK                      (0xFF << 0)            /* Receive Interrupt Watchdog Tier Count */

/* The Channel i Slot Function Control and Status register */
#define DMA_CHi_SLOT_FUNC_CTRL_STS(i) (0x113C  + ((i) * 0x0080))  /* i = {0...TXDMACNT-1} */
    #define RSN_MASK                      (0xF << 16)             /* Reference Slot Number */
    #define SIV_MASK                      (0xFFF << 4)            /* Slot Interval Value */
    #define ASC                           (1 << 1)                /* Advance Slot Check */
    #define ESC                           (1 << 0)                /* Enable Slot Comparison */

/* The Channel i Current Application Transmit Descriptor register */
#define DMA_CHi_CURR_APP_TXDESC(i)    (0x1144  + ((i) * 0x0080))  /* i = {0...TXDMACNT-1} */

/* The Channel i Current Application Receive Descriptor register */
#define DMA_CHi_CURRENT_APP_RXDESC(i)                (0x114C  + ((i) * 0x0080))  /* i = {0...RXDMACNT-1} */

/* The Channel i Current Application Transmit Buffer Address register */
#define DMA_CHi_CURRENT_APP_TXBUF(i)                (0x1154  + ((i) * 0x0080))  /* i = {0...TXDMACNT-1} */

/* The Channel i Current Application Receive Buffer Address register */
#define DMA_CHi_CURRENT_APP_RXBUF(i)                (0x115C  + ((i) * 0x0080))  /* i = {0...RXDMACNT-1} */

/* The Channel i Status register */
#define DMA_CHi_STATUS(i)            (0x1160  + ((i) * 0x0080))  /* i = {0...TXDMACNT-1} */
    #define REB                          (0x7 << 19)              /* Rx DMA Error Bits */
    #define RX_ERR_DESCR                 (1 << 20)
    #define RX_ERR_READ                  (1 << 19)
    #define TEB                          (0x7 << 16)              /* Tx DMA Error Bits */
    #define TX_ERR_DESCR                 (1 << 17)
    #define TX_ERR_READ                  (1 << 16)
    #define NIS                          (1 << 15)                /* Normal Interrupt Summary */
    #define AIS                          (1 << 14)                /* Abnormal Interrupt Summary */
    #define CDE                          (1 << 13)                /* Content Descriptor Error */
    #define FBE                          (1 << 12)                /* Fatal Bus Error */
    #define ERI                          (1 << 11)                /* Early Receive Interrupt */
    #define ETI                          (1 << 10)                /* Early Transmit Interrupt */
    #define DMA_RWT                      (1 <<  9)                /* Receive Watchdog Timeout */
    #define RPS                          (1 <<  8)                /* Receive Process Stopped */
    #define RBU                          (1 <<  7)                /* Receive Buffer Unavailable */
    #define RI                           (1 <<  6)                /* Receive Interrupt */
    #define TBU                          (1 <<  2)                /* Transmit Buffer Unavailable */
    #define TPS                          (1 <<  1)                /* Transmit Process Stopped */
    #define TI                           (1 <<  0)                /* Transmit Interrupt */

/* The Channel i Missed Frame Count register. */
#define DMA_CHi_MISS_FRAME_CNT(i)    (0x116C  + ((i) * 0x0080))   /* i = {0...TXDMACNT-1} */
    #define MFCO                         (1 << 15)                /* Overflow status of the MFC Counter */
    #define MFC_MASK                     (0x7FF << 0)             /* Dropped Packet Counters */

/* CCNT Hardware registers */
#define REG_CCNT_SR00    0x1100
#define REG_CCNT_SR01    0x1104
#define REG_CCNT_DR00    0x1300
#define REG_CCNT_DR01    0x1304

#define EMAC_FILTER_LIMIT   64

/** GMAC version 5 -- safety feature **/
#define MAC_DPP_FSM_INT_STATUS		0x0140
#define MAC_AXI_SLV_DPE_ADDR_STATUS	0x0144
#define MAC_FSM_CONTROL			0x0148
#define FSM_PRTYEN		(1 << 1)
#define FSM_TMOUTEN		(1 << 0)

#define MTL_RXP_CONTROL_STATUS		0x0ca0
#define RXP_RXPI		(1 << 31)
#define RXP_NPE			0xFF0000
#define RXP_NVE			0xFF
#define MTL_RXP_IACC_CTRL_STATUS	0x0cb0
#define RXP_IACC_STARTBUSY	(1 << 31)
#define RXP_IACC_RXPEIEC	(0x3 << 21)
#define RXP_IACC_RXPEIEE	(1 << 20)
#define RXP_IACC_WRRDN		(1 << 16)
#define RXP_IACC_ADDR		0xFFFF
#define MTL_RXP_IACC_DATA		0x0cb4
#define MTL_ECC_CONTROL			0x0cc0
#define ECC_TSOEE		(1 << 4)
#define ECC_MRXPEE		(1 << 3)
#define ECC_MESTEE		(1 << 2)
#define ECC_MRXEE		(1 << 1)
#define ECC_MTXEE		(1 << 0)

#define MTL_SAFETY_INT_STATUS		0x0cc4
#define MTL_SAFETY_MCSIS	(1 << 31)
#define MTL_SAFETY_MEUIS	(1 << 1)
#define MTL_SAFETY_MECIS	(1 << 0)
#define MTL_ECC_INT_ENABLE		0x0cc8
#define MTL_ECC_RPCEIE		(1 << 12)
#define MTL_ECC_ECEIE		(1 << 8)
#define MTL_ECC_RXCEIE		(1 << 4)
#define MTL_ECC_TXCEIE		(1 << 0)
#define MTL_ECC_INT_STATUS		0x0ccc
#define MTL_DPP_CONTROL			0x0ce0
#define MTL_DPP_EPSI		(1 << 2)
#define MTL_DPP_OPE			(1 << 1)
#define MTL_DPP_EDPP		(1 << 0)

#define DMA_SAFETY_INT_STATUS		0x1080
#define DMA_SAFETY_MSUIS	(1 << 29)
#define DMA_SAFETY_MSCIS	(1 << 28)
#define DMA_SAFETY_DEUIS	(1 << 1)
#define DMA_SAFETY_DECIS	(1 << 0)
#define DMA_ECC_INT_ENABLE		0x1084
#define DMA_ECC_TCEIE		(1 << 0)
#define DMA_ECC_INT_STATUS		0x1088
/****/

/* Structure  */
typedef struct dwceqos_dev_s {
      struct device           dev;  /* Common device */
      struct ethercom         ecom;

      nic_config_t            cfg;
      nic_stats_t             stats;

      struct _iopkt_self      *iopkt;
      struct mii_data         bsd_mii;
      struct cache_ctrl       cachectl;

      uintptr_t               iobase;
      uintptr_t               mac_base;

      int                     iid;
      int                     dying;
      unsigned int            is_ptp_enabled;

      struct callout          mii_callout;
      struct _iopkt_inter     inter;
      const struct sigevent   *(*isrp)(void *, int);
      mdi_t                   *mdi;
      uint16_t                *phy_regs;
      void                    *sd_hook;
      uint32_t                flow;

      dwceqos_desc_t          *descs;

      /* Rx */
      uint32_t                rx_desc_num;
      uint32_t                rx_desc_head;
      dwceqos_desc_t          *rx_desc;
      dwceqos_desc_t          *rx_desc_tail;

      /* Tx */
      uint32_t                tx_desc_num;
      uint32_t                tx_desc_head;
      uint32_t                tx_desc_avail;
      dwceqos_desc_t          *tx_desc;
      dwceqos_desc_t          *tx_desc_tail;

      /* Transmission queued mbuf, len and sent bytes */
      struct mbuf             *tq_mbuf;
      uint32_t                tq_pkt_len;
      uint32_t                tq_pkt_xbytes;
} dwceqos_dev_t;

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/
/* bsd_media.c */
void bsd_mii_initmedia(dwceqos_dev_t *);
void bsd_mii_finimedia(dwceqos_dev_t *dwceqos);

/* devctl.c */
int dwceqos_ioctl (struct ifnet *, unsigned long, caddr_t);

/* event.c */
int dwceqos_process_interrupt (void *, struct nw_work_thread *);
const struct sigevent *dwceqos_isr (void *, int);
int dwceqos_enable_interrupt (void *);

/* mii.c */
void dwceqos_mdi_start_monitor(dwceqos_dev_t *dwceqos);
void dwceqos_mdi_stop_monitor(dwceqos_dev_t *dwceqos);
void dwceqos_init_phy(dwceqos_dev_t *dwceqos);
void dwceqos_fini_phy(dwceqos_dev_t *dwceqos);

/* transmit.c */
void dwceqos_start (struct ifnet *);
void dwceqos_reap_pkts (dwceqos_dev_t *dwceqos);

#ifdef __cplusplus
    };
#endif

#endif    /* __DWCEQOS_H__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/dwceqos/dwceqos.h $ $Rev: 905611 $")
#endif
