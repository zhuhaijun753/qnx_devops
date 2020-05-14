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

#ifdef DEFN
	#define EXT
	#define INIT1(a) = { a }
#else
	#define EXT extern
	#define INIT1(a)
#endif

#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <termios.h>
#include <devctl.h>
#include <sys/dcmd_chr.h>
#include <sys/iomsg.h>
#include <atomic.h>
#include <hw/inout.h>
#include <hw/linflexd.h>
#include <sys/io-char.h>
#include <sys/hwinfo.h>
#include <drvr/hwinfo.h>
#include <pthread.h>
#include <sys/rsrcdbmgr.h>
#include <sys/dispatch.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>

// RX and TX FIFOs are 4 bytes each
#define FIFO_SIZE 4

/*
 * LINFLEXD IP has a 4 byte buffer for TX and a 4 byte buffer for RX
 * The buffer can be configured in "buffer mode" or FIFO mode.
 * Note that depending on which mode is used several bits take on different
 * meanings such as the LINFLEXD_UARTSR register's LINFLEXD_UARTSR_DRFRFE and
 * LINFLEXD_UARTSR_DTFTFF bits.
 */

enum linflexd_buffer_mode {
    UART_MODE_BUFFER,
    UART_MODE_FIFO,
    UART_MODE_LAST,    /* End of enum, not a mode */
};

typedef struct dev_serlinflexd {
    TTYDEV        tty;
    uintptr_t     base;        /* Base address (virtual) of LinFlexD */
    unsigned      intr;        /* LinFlexD IRQ # from command line or HWINFO section */
    int           iid;         /* Interrupt ID from InterruptAttach */
    unsigned      buffer_mode; /* linflexd_uart_mode */
    unsigned      clk;
    unsigned      div;
    unsigned      fifo;
    unsigned      fcr;
    unsigned      cr2;
    unsigned      bir;
    unsigned      is_tx;       /* transmit status flag */
} DEV_LINFLEXD;

typedef struct ttyinit_linflexd {
    TTYINIT       tty;
    unsigned      intr;        /* Interrupts */
    unsigned      buffer_mode;
} TTYINIT_LINFLEXD;

EXT TTYCTRL       ttyctrl;

#include "proto.h"

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devc/serlinflexd/externs.h $ $Rev: 886103 $")
#endif
