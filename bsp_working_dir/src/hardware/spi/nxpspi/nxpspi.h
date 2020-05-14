/*
 * $QNXLicenseC:
 * Copyright 2017, QNX Software Systems.
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


#ifndef _NXP_SPI_H_INCLUDED
#define _NXP_SPI_H_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <hw/spi-master.h>

#define NXP_SPI_PRIORITY                21
#define NXP_SPI_EVENT                   1
#define NXP_DMA_PULSE_PRIORITY          21
#define NXP_DMA_PULSE_CODE              1
#define NXP_SPI_BASE                    0x40057000
#define NXP_SPI_SIZE                    0x140
#define NXP_SPI_CLK                     133000000

#define NXP_SPI_IRQ                     93
#define NXP_SPI_FIFO_SIZE               5
#define NXP_SPI_CHANNEL_MAX             4

#define NXP_SPI_MCR                     0x00        // Module config register
 #define NXP_SPI_MCR_HALT               1           // Stop transfer
 #define NXP_SPI_MCR_CLR_RXF            (1 << 10)   // clear RX FIFO/buffer
 #define NXP_SPI_MCR_CLR_TXF            (1 << 11)   // clear TX FIFO/buffer
 #define NXP_SPI_MCR_MDIS               (1 << 14)   // Module Disable
 #define NXP_SPI_MCR_PCSIS              (1 << 16)   // Peripheral Chip Select x Inactive State
 #define NXP_SPI_MCR_PCSIS_POS          16
 #define NXP_SPI_MCR_PCSIS_BITS         (0xff << 16)
 #define NXP_SPI_MCR_ROOE               (1 << 24)   // Receive FIFO Overflow Overwrite Enable
 #define NXP_SPI_MCR_FRZ                (1 << 27)   // Freeze
 #define NXP_SPI_MCR_CONT_SCKE          (1 << 30)   // continous SCK enable
 #define NXP_SPI_MCR_MSTR               (1 << 31)   // master mode select

#define NXP_SPI_TCR                     0x08        // Transfer count register
 #define NXP_SPI_TCR_SPI_TCNT           0xFFFF0000  // SPI transfer counter
 #define NXP_SPI_TCR_SPI_TCNT_POS       16

#define NXP_SPI_CTAR0                   0x0C        // Clock and Transfer Attributes Register 0
 #define NXP_SPI_CTAR0_BR_POS           0           // baud rate scaler value 2
 #define NXP_SPI_CTAR0_DT               0x000000E0  // Delay after transfer scaler value 0xE
 #define NXP_SPI_CTAR0_ASC_4            (1 << 8 )   // after SCK delay scaler value 4
 #define NXP_SPI_CTAR0_ASC_8            (2 << 8 )   // after SCK delay scaler value 8
 #define NXP_SPI_CTAR0_ASC_16           (3 << 8 )   // after SCK delay scaler value 16
 #define NXP_SPI_CTAR0_ASC_32           (4 << 8 )   // after SCK delay scaler value 32
 #define NXP_SPI_CTAR0_ASC_64           (5 << 8 )   // after SCK delay scaler value 64
 #define NXP_SPI_CTAR0_CSSCK_2          (0 << 12)   // PCS to SCK delay scaler value 2
 #define NXP_SPI_CTAR0_CSSCK_4          (1 << 12)   // PCS to SCK delay scaler value 4
 #define NXP_SPI_CTAR0_CSSCK_8          (2 << 12)   // PCS to SCK delay scaler value 8
 #define NXP_SPI_CTAR0_CSSCK_16         (3 << 12)   // PCS to SCK delay scaler value 16
 #define NXP_SPI_CTAR0_CSSCK_32         (4 << 12)   // PCS to SCK delay scaler value 32
 #define NXP_SPI_CTAR0_CSSCK_64         (5 << 12)   // PCS to SCK delay scaler value 64
 #define NXP_SPI_CTAR0_PDT              0x00040000  // Delay after transfer prescaler value 3
 #define NXP_SPI_CTAR0_PBR              0x00010000  // Baud rate prescaler value of 3
 #define NXP_SPI_CTAR0_PBR_POS          16
 #define NXP_SPI_CTAR0_PASC_1           (0 << 20)   // After SCK delay prescaler value 1, default value
 #define NXP_SPI_CTAR0_PASC_3           (1 << 20)   // After SCK delay prescaler value 3
 #define NXP_SPI_CTAR0_PASC_5           (2 << 20)   // After SCK delay prescaler value 5
 #define NXP_SPI_CTAR0_PASC_7           (3 << 20)   // After SCK delay prescaler value 7
 #define NXP_SPI_CTAR0_PCSSCK_1         (0 << 22)   // PCS to SCK delay prescaler value 1
 #define NXP_SPI_CTAR0_PCSSCK_3         (1 << 22)   // PCS to SCK delay prescaler value 3
 #define NXP_SPI_CTAR0_PCSSCK_5         (2 << 22)   // PCS to SCK delay prescaler value 5
 #define NXP_SPI_CTAR0_PCSSCK_7         (3 << 22)   // PCS to SCK delay prescaler value 7
 #define NXP_SPI_CTAR0_LSBFE            (1 << 24)   // Data transfered LSB First
 #define NXP_SPI_CTAR0_CPHA             (1 << 25)   // Clock phase 1
 #define NXP_SPI_CTAR0_CPOL             (1 << 26)   // Clock polarity when inactive is high
 #define NXP_SPI_CTAR0_CPOL_POS         26
 #define NXP_SPI_CTAR0_FMSZ(n)          (((n)-1) << 27)  //   Frame size max 16 bits
 #define NXP_SPI_CTAR0_DBR              (1 << 31)   // Double baud rate

#define NXP_SPI_SR                      0x02C       // Status register
 #define NXP_SPI_SR_POPNXTPTR           0x0000000F  // Pop Next Pointer
 #define NXP_SPI_SR_RXCTR               0x000000F0  // RX FIFO counter
 #define NXP_SPI_SR_RXCTR_POS           4
 #define NXP_SPI_SR_TXNXTPTR            0x00000F00  // Transmit Next Pointer
 #define NXP_SPI_SR_TXCTR               0x0000F000  // TX FIFO Counter

 // write 1 to clear the following status bits
 #define NXP_SPI_SR_CMDFFF              (1 << 16)   // Command FIFO Fill Flag
 #define NXP_SPI_SR_RFDF                (1 << 17)   // Receive FIFO Drain Flag, RX FIFO is not empty.
 #define NXP_SPI_SR_TFIWF               (1 << 18)   // Tranmit FIFO Invalid Write Flag
 #define NXP_SPI_SR_RFOF                (1 << 19)   // Receive FIFO Overflow Flag
 #define NXP_SPI_SR_TFFF                (1 << 25)   // Transmit FIFO Fill Flag
 #define NXP_SPI_SR_TFUF                (1 << 27)   // Transmit FIFO Underflow Flag
 #define NXP_SPI_SR_EOQF                (1 << 28)   // End of Queue Flag, is set
 #define NXP_SPI_SR_TCF                 (1 << 31)   // Transfer Complete Flag

#define NXP_SPI_RSER                    0x30        // DMA/Interrupt Request Select and Enable Register
 #define NXP_SPI_RSER_TFIWF_RE          (1 << 18)   // Transmit FIFO Invalid Write Request Enable
 #define NXP_SPI_RSER_RFOF_RE           (1 << 19)   // Receive FIFO Overflow Request Enable
 #define NXP_SPI_RSER_CMDTCF_RE         (1 << 23)   // Command Transmission Complete Request Enable
 #define NXP_SPI_RSER_TCF_RE            (1 << 31)   // Transmission Complete Request Enable

#define NXP_SPI_PUSHR                   0x34        // PUSH TX FIFO Register In Master Mode
 #define NXP_SPI_PUSHR_TXDATA           0x0000FFFF  // Transmit data
 #define NXP_SPI_PUSHR_PCS              0x000F0000  // Peripheral Chip Select
 #define NXP_SPI_PUSHR_PCS_POS          16
 #define NXP_SPI_PUSHR_CTCNT            (1 << 26)   // Clear Transfer Counter
 #define NXP_SPI_PUSHR_EOQ              (1 << 27)   // End Of Queue
 #define NXP_SPI_PUSHR_CONT             (1 << 31)   // Continuous Peripheral Chip Select Enable

#define NXP_SPI_POPR                    0x38        // POP RX FIFO register
 #define NXP_SPI_POPR_RXDATA_MASK       0xFFFFFFFF  // Received Data

#define NXP_SPI_TXFRn                   0x3C        // Transmit FIFO Registers 0 to 4, RO
 #define NXP_SPI_TXFRn_TXCMD            0xFFFF0000  // Transmit Command
 #define NXP_SPI_TXFRn_TXDATA           0x0000FFFF  // Transmit Data

#define NXP_SPI_RXFRn                   0x7C        // Receive FIFO Registers 0 to 4, RO
 #define NXP_SPI_RXFRn_RXDATA_MASK      0xFFFFFFFF  // Receive Data

// The following is for the QPSI
// The Sampling Register allows configuration of how the incoming data from the external
// serial flash devices are sampled in the QuadSPI module.
#define NXP_QSPI_SMPR                   0x108   // sampling register

// The QSPI_RBSR register contains information related to the receive data buffer.
#define NXP_QSPI_RBSR                   0x10C   // RX buffer status register
 #define NXP_QSPI_RBSR_RDBFL_MASK       0x00003F00  // RX Buffer Fill Level, indicates how many entries of 4 bytes are still available in the RX Buffer
    #define NXP_QSPI_RBSR_RDBFL_POS     8
 #define NXP_QSPI_RBSR_RDCTR_MASK       0xFFFF0000  // Read counter
    #define NXP_QSPI_RBSR_RDCTR_POS     16

// The QSPI_RBCT register contains control data related to the receive data buffer.
#define NXP_QSPI_RBCT                   0x110   // RX buffer control register
 #define NXP_QSPI_RBCT_WMRK             0x0000001F  // RX Buffer Watermark
 #define NXP_QSPI_RBCT_RXBRD            (1 << 8)    // RX Buffer content is read using the IP Bus registers QSPI_RBDR0 to QSPI_RBDR31

// The QSPI_TBSR register contains information related to the transmit data buffer.
#define NXP_QSPI_TBSR                   0x150   // TX buffer status register
 #define NXP_QSPI_TBSR_TRBFL            0x00003F00  // TX Buffer Fill Level
    #define NXP_QSPI_TBSR_TRBFL_POS     8
 #define NXP_QSPI_TBSR_TRCTR            0xFFFF0000  // Transmit counter
    #define NXP_QSPI_TBSR_TRCTR_POS     16

// The QSPI_TBDR register provides access to the circular TX Buffer of depth 128 bytes.
#define NXP_QSPI_TBDR                   0x154   // TX buffer data register

// This register contains control information for transmit data buffer.
#define NXP_QSPI_TBCT                   0x158   // TX buffer control register
 #define NXP_QSPI_TBCT_WMRK             0x00000001F // Determines the watermark for the TX Buffer

// The QSPI_SR register provides all available status information about SFM command
// execution and arbitration, the RX Buffer, TX Buffer, and the AHB Buffer.
#define NXP_QSPI_SR                     0x15C   // Status register
 #define NXP_QSPI_SR_RXWE               (1 << 16) // RX Buffer Watermark Exceeded
 #define NXP_QSPI_SR_RXFULL             (1 << 19) // RX Buffer full
 #define NXP_QSPI_SR_TXEDA              (1 << 24) // Tx Buffer Enough Data Available
 #define NXP_QSPI_SR_TXWA               (1 << 25) // Tx Buffer watermark available
 #define NXP_QSPI_SR_TXFULL             (1 << 27) // Tx Buffer full

// The QSPI_FR register provides all available flags about SFM command execution and
// arbitration which may serve as source for the generation of interrupt service requests.
// write 1 to clear any of the bits set
#define NXP_QSPI_FR                     0x160   // Flag register
 #define NXP_QSPI_FR_RBDF               (1 << 16) // RX Buffer Drain Flag, w1c to trigger RX buffer pop event
 #define NXP_QSPI_FR_RBOF               (1 << 17) // RX Buffer overflow flag
 #define NXP_QSPI_FR_TBUF               (1 << 26) // TX buffer underrun flag
 #define NXP_QSPI_FR_TBFF               (1 << 27) // TX buffer fil flag

// The QuadSPI_RSER register provides enables and selectors for the interrupts in the
// QuadSPI module.
#define NXP_QSPI_RSER                   0x164       // Interrupt and DMA request select and enable register
 #define NXP_QSPI_RSER_TFIE             1           // Transaction Finished Interrupt Enable
 #define NXP_QSPI_RSER_RBDIE            (1 << 16)   // RX Buffer Drain Interrupt Enable
 #define NXP_QSPI_RSER_RBOIE            (1 << 17)   // RX Buffer Overflow Interrupt Enable
 #define NXP_QSPI_RSER_TBUIE            (1 << 26)   // TX Buffer Underrun Interrupt Enable
 #define NXP_QSPI_RSER_TBFIE            (1 << 27)   // TX Buffer Fill Interrupt Enable

// The QuadSPI_RBDR registers provide access to the individual entries in the RX Buffer.
#define NXP_QSPI_RBDR0                  0x200   // RX buffer data register 0
#define NXP_QSPI_RBDR1                  0x204   // RX buffer data register 1
// .
// .
#define NXP_QSPI_RBDR31                 0x27C   // Rx buffer data register 31
#define CSPI_CONTROLREG_DRCTL_MASK      0x00030000  // reserved

#define CSPI_CONTROLREG_DRCTL_POS       16
#define CSPI_CONTROLREG_DRCTL_EDGE      1
#define CSPI_CONTROLREG_DRCTL_LEVEL     2
#define CSPI_CONTROLREG_CSEL_MASK       0x000C0000  //channel 3, SS3, not used
#define CSPI_CONTROLREG_CSEL_POS        18
#define CSPI_CONTROLREG_BCNT_MASK       0xFFF00000  // SPI burst 2^7 words, 128 words
#define CSPI_CONTROLREG_BCNT_POS        20
#define CSPI_CONREG_PREDIVIDR_POS       12

#define CSPI_CONREG_POSTDIVIDR_POS      8

// CONFIGREG BIT Definitions
#define CSPI_CONFIGREG_PHA_MASK         0x0000000f  // phase 1 operation
#define CSPI_CONFIGREG_PHA_POS          0
#define CSPI_CONFIGREG_POL_MASK         0x000000f0  // active low polarity
#define CSPI_CONFIGREG_POL_POS          4
#define CSPI_CONFIGREG_SSCTL_MASK       0x00000f00  // chip select operation
#define CSPI_CONFIGREG_SSCTL_POS        8
#define CSPI_CONFIGREG_SSPOL_MASK       0x0000f000  // CS active high
#define CSPI_CONFIGREG_SSPOL_POS        12
#define CSPI_CONFIGREG_DATACTL_MASK     0x000f0000  // data line are low for inactive
#define CSPI_CONFIGREG_DATACTL_POS      16
#define CSPI_CONFIGREG_CLKCTL_MASK      0x00f00000  // clocks are high when inactive
#define CSPI_CONFIGREG_CLKCTL_POS       20

typedef struct {
    SPIDEV          spi;    /* has to be the first element */
    unsigned        pbase;
    uintptr_t       vbase;
    uintptr_t       iomuxc_vbase;
    int             irq;
    int             iid;
    int             chid, coid;
    int             irq_count;
    uint32_t        clock;
    uint32_t        csdelay;
    uint8_t         *pbuf;
    int             xlen, tlen, rlen, txfifo;
    int             dlen;
    int             dtime;  /* usec per burst, for time out use */
    int             emulate_8b;
    struct sigevent spievent;
} nxp_spi_t;

extern void *nxp_init(void *hdl, char *options);
extern void nxp_dinit(void *hdl);
extern void *nxp_xfer(void *hdl, uint32_t device, uint8_t *buf, int *len);
extern int nxp_setcfg(void *hdl, uint16_t device, spi_cfg_t *cfg);

extern int nxp_devinfo(void *hdl, uint32_t device, spi_devinfo_t *info);
extern int nxp_drvinfo(void *hdl, spi_drvinfo_t *info);

extern int nxp_attach_intr(nxp_spi_t *nxp);
extern int nxp_wait(nxp_spi_t *dev, int len);

extern uint32_t nxp_cfg(void *hdl, spi_cfg_t *cfg);

#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/spi/nxpspi/nxpspi.h $ $Rev: 847655 $")
#endif
