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

/*
 * The purpose of this module is to control the access to shared variables
 * to shared mutexes that are used to implement
 * synchronization control for multiple instances of the dma library.
 */

#include "fsl_edma3.h"

// local variables
static int fd;
static fsl_edma3_shmem_t * shmem_ptr;

////////////////////////////////////////////////////////////////////////////////
//                              PUBLIC FUNCTIONS                              //
////////////////////////////////////////////////////////////////////////////////

// This function opens and maps shared memory for the fsl_edma3 library

int fsl_edma3_sync_init(void)
{

    //seed the shared memory if need
    fsl_edma3_seed_shared_mem();

    fd = shm_open("/FSL_EDMA3_MUTEX",O_RDWR , 0666);
    if (fd == -1) {
        goto fail1;
    }

    //map it to our control structure
    shmem_ptr = mmap(    0,
                         sizeof(fsl_edma3_shmem_t),
                         PROT_READ|PROT_WRITE,
                         MAP_SHARED,
                         fd,
                         0          );
    if (shmem_ptr == MAP_FAILED) {
        goto fail2;
    }

    return 0;
fail2:
    close(fd);
fail1:
    return -1;
}


void fsl_edma3_sync_fini(void)
{
    munmap(shmem_ptr,sizeof(fsl_edma3_shmem_t));
    close(fd);
}

// Shared-memory variable access functions

pthread_mutex_t * fsl_edma3_sync_libinit_mutex_get()
{
    return &(shmem_ptr->libinit_mutex);
}


int fsl_edma3_sync_is_first_process()
{
    if (shmem_ptr->process_cnt == 1)
        return 1;
    else
        return 0;
}

int fsl_edma3_sync_is_last_process()
{
    if (shmem_ptr->process_cnt == 0)
        return 1;
    else
        return 0;
}

void fsl_edma3_sync_process_cnt_incr()
{
    shmem_ptr->process_cnt++;
}

void fsl_edma3_sync_process_cnt_decr()
{
    shmem_ptr->process_cnt--;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/lib/dma/fsl-edma3/sync.c $ $Rev: 862698 $")
#endif

