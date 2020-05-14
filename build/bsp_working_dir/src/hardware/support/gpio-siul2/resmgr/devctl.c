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
#include <hw/dcmd_gpio_siul2.h>

int
gpio_devctl(resmgr_context_t *ctp, io_devctl_t *msg, gpio_ocb_t *ocb)
{
    int        status;
    int        nbytes=0;
    gpio_devctl_t    *devmsg_ptr;
    uint8_t    pin;
    gpio_dev_t   *dev = ocb->hdr.attr;

    if ((status = iofunc_devctl_default(ctp, msg, &ocb->hdr)) != _RESMGR_DEFAULT)
        return(_RESMGR_ERRNO(status));
    status=0;

    // get a pointer to the structure
    devmsg_ptr = (gpio_devctl_t*)_DEVCTL_DATA(msg->i);
    switch (msg->i.dcmd)
    {
        case DCMD_SET_INPUT:
             pin=devmsg_ptr->set_input.pin_num;

             status = hw_set_input(dev->hdl, pin);

             if (status < 0)
                 return (EINVAL);
             break;

        case DCMD_SET_OUTPUT:
             pin=devmsg_ptr->set_output.pin_num;

             status = hw_set_output(dev->hdl, pin);

            if (status < 0)
                 return (EINVAL);
             break;

        case DCMD_READ:
             status = hw_cmd_read(dev->hdl, (void*)devmsg_ptr);
            if (status < 0)
                 return (EINVAL);
             msg->o.ret_val = 0;
             return(_RESMGR_PTR(ctp,  &msg->o,sizeof(msg->o) + sizeof(gpio_devctl_read_t)));
             break;

        case DCMD_WRITE:
             status = hw_cmd_write(dev->hdl, (void*)devmsg_ptr);
             if (status < 0)
                 return (EINVAL);
             msg->o.ret_val = 0;
             return(_RESMGR_PTR(ctp,  &msg->o, sizeof(msg->o) + sizeof(gpio_devctl_write_t)));
             break;

         default:
             return (ENOSYS);
        }

        _IO_SET_WRITE_NBYTES (ctp, nbytes);
        return (_RESMGR_NPARTS(0));
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/support/gpio-siul2/resmgr/devctl.c $ $Rev: 859876 $")
#endif

