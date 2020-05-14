/*
 * $QNXLicenseC:
 * Copyright 2016-2019, QNX Software Systems.
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

#include <stdio.h>
#include <stdarg.h>
#include <sys/slog2.h>
#include <sys/slogcodes.h>
#include "fsl_edma3.h"

static slog2_buffer_set_config_t buffer_config;
static slog2_buffer_t buffer_handle;
static const char *buffer_set_name = "fsl_edma3_lib";
static const char *buffer_name = "fsl_edma3_lib_buffer";

int fsl_slog2_init(void)
{
  buffer_config.buffer_set_name = (char *)buffer_set_name;
  buffer_config.num_buffers = 1;
  buffer_config.verbosity_level = SLOG2_ERROR;
  buffer_config.buffer_config[0].buffer_name = (char *)buffer_name;
  buffer_config.buffer_config[0].num_pages = 8;

  if ( slog2_register (&buffer_config, &buffer_handle, 0) == -1 ) {
      fprintf (stderr, "In %s, unable to register slog2 buffer!\n", __FUNCTION__);
      return -1;
  }

  return 0;
}

int
fsl_edma3_slogf(const char *fmt, ...)
{
    int         status;
    va_list     arg;

    va_start (arg, fmt);
    status = vslog2f (buffer_handle, 0, SLOG2_ERROR, fmt, arg);
    va_end(arg);
    return status;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/lib/dma/fsl-edma3/slogf.c $ $Rev: 876350 $")
#endif

