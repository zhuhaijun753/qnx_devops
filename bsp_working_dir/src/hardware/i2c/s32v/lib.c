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


#include "proto.h"

int
i2c_master_getfuncs(i2c_master_funcs_t *funcs, int tabsize)
{
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            init, s32v_init, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            fini, s32v_fini, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            send, s32v_send, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            recv, s32v_recv, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            set_slave_addr, s32v_set_slave_addr, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            set_bus_speed, s32v_set_bus_speed, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            version_info, s32v_version_info, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            driver_info, s32v_driver_info, tabsize);
    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/s32v/lib.c $ $Rev: 810496 $")
#endif
