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


#include "proto.h"

static volatile unsigned done = 0;
static gpio_dev_t *Dev;
extern gpio_dev_t * siul2_hw_init(gpio_dev_t *dev);
void siul2_hw_fini(gpio_dev_t *dev);


int
gpio_slogf(const char *fmt, ...)
{
    int         status;
    va_list     arg;

    va_start(arg, fmt);
    status = vfprintf(stderr, fmt, arg);
    status += fprintf(stderr, "\n");
    va_end(arg);
    return status;
}

static void
gpio_exit(int signo)
{
    atomic_set(&done, 1);
    dispatch_unblock(Dev->ctp);
}

static int
gpio_main(gpio_dev_t *dev)
{
    resmgr_connect_funcs_t  connect_funcs;
    resmgr_io_funcs_t   io_funcs;
    resmgr_attr_t       rattr;
    iofunc_funcs_t      ocb_funcs = { _IOFUNC_NFUNCS, gpio_ocb_calloc,
                            gpio_ocb_free
                        };
    iofunc_mount_t      mount = { 0, 0, 0, 0, &ocb_funcs };
    dispatch_context_t  *ctp;
    char                devname[PATH_MAX+1];
    struct sigaction    sa;
    if (!dev)
        return -1;

    Dev = dev;

    if (NULL == (dev->dpp = dispatch_create())) {
        perror("dispatch_create() failed");
        return -1;
    }

    /* set up i/o handler functions */
    memset(&rattr, 0, sizeof(rattr));
    rattr.nparts_max = GPIO_RESMGR_NPARTS_MIN;
    rattr.msg_max_size = GPIO_RESMGR_MSGSIZE_MIN;

    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                     _RESMGR_IO_NFUNCS, &io_funcs);
    io_funcs.devctl = gpio_devctl;

    iofunc_attr_init(&dev->hdr, S_IFCHR | 0666, NULL, NULL);
    dev->hdr.mount = &mount;

    /* register device name */
    snprintf(devname, PATH_MAX, "/dev/gpio");

    if (-1 == (dev->id =
               resmgr_attach(dev->dpp, &rattr, devname, _FTYPE_ANY, 0,
                             &connect_funcs, &io_funcs, dev)))
    {
        perror("resmgr_attach() failed");
        return -1;
    }

    if (NULL == (dev->ctp = dispatch_context_alloc(dev->dpp))) {
        perror("dispatch_context_alloc() failed");
        return -1;
    }

    /* register exit handler */
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);
    sa.sa_handler = gpio_exit;
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    /* background the process */
    procmgr_daemon(0, PROCMGR_DAEMON_NOCLOSE|PROCMGR_DAEMON_NODEVNULL);

    /* wait for messages */
    while (!done) {
        if ((ctp = dispatch_block(dev->ctp))) {
            dev->ctp = ctp;
            dispatch_handler(dev->ctp);
        } else if (errno != EFAULT) {
            atomic_set(&done, 1);
        }
    }

    /* Received SIGTERM: clean up */
    resmgr_detach(dev->dpp, dev->id, _RESMGR_DETACH_ALL);
    return 0;
}

int
main(int argc, char *argv[])
{
    int                 status;
    gpio_dev_t           dev;

    memset(&dev, 0, sizeof(dev));
    siul2_hw_init(&dev);
    status = gpio_main(&dev);
    siul2_hw_fini(&dev);

  return status;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/support/gpio-siul2/resmgr/main.c $ $Rev: 859876 $")
#endif

