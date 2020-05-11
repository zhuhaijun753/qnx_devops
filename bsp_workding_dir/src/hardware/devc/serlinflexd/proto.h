/*
 * $QNXLicenseC:
 * Copyright 2016 QNX Software Systems.
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

DEV_LINFLEXD *    create_device(TTYINIT_LINFLEXD *dip, unsigned unit);
void              ser_stty(DEV_LINFLEXD *dev);
void              ser_ctrl(DEV_LINFLEXD *dev, unsigned flags);
void              ser_attach_intr(DEV_LINFLEXD *dev);
void              ser_detach_intr(DEV_LINFLEXD *dev);
void *            query_default_device(TTYINIT_LINFLEXD *dip, void *link);
unsigned          options(int argc, char *argv[]);
void              clear_device(DEV_LINFLEXD *dev);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devc/serlinflexd/proto.h $ $Rev: 810496 $")
#endif
