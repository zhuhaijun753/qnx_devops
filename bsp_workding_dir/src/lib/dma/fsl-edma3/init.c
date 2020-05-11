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
 * $authority
 */

#include "fsl_edma3.h"


////////////////////////////////////////////////////////////////////////////////
//                            PRIVATE FUNCTIONS                               //
////////////////////////////////////////////////////////////////////////////////


static void shared_mem_init( fsl_edma3_shmem_t * shmem_ptr )
{
    pthread_mutexattr_t mutex_attr;

    // Initialized multi-process mutexes
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr,PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shmem_ptr->libinit_mutex),&mutex_attr);

    // init other state variables
    shmem_ptr->process_cnt = 0;
}

static fsl_edma3_shmem_t * shared_mem_create()
{
    int fd;
    int status;
    fsl_edma3_shmem_t * shmem_ptr;

    fd = shm_open("/FSL_EDMA3_MUTEX",O_RDWR | O_CREAT | O_EXCL, 0666);

    if (fd >= 0) {

        //size the newly allocated memory
        status = ftruncate(fd,sizeof(fsl_edma3_shmem_t));
        if (status == -1) {
            perror("shared_mem_create() ftruncate failed\n");
            goto fail2;
        }

        //map it to our control structure
        shmem_ptr = mmap(    0,
                             sizeof(fsl_edma3_shmem_t),
                             PROT_READ|PROT_WRITE,
                             MAP_SHARED,
                             fd,
                             0          );
        if (shmem_ptr == MAP_FAILED) {
            perror("shared_mem_create() Couldn't mmap shared memory\n");
            goto fail2;
        }
    } else {

        // Couldn't create shared memory because it either already exists,
        // or for some other reason... it doesn't matter at this point, because
        // fsl_edma3_sync_init() will try and open the shared memory object if it exists.
        goto fail1;
    }

    return shmem_ptr;

fail2:
    close(fd);

fail1:
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//                            PUBLIC FUNCTIONS                                //
////////////////////////////////////////////////////////////////////////////////

void fsl_edma3_seed_shared_mem(void)
{
    rsrc_alloc_t    ralloc;
    fsl_edma3_shmem_t    *shmem_ptr;

    shmem_ptr = shared_mem_create();
    if ( shmem_ptr ) {
        // initialize shared memory on creation
        shared_mem_init( shmem_ptr );

        // seed the resource db manager
        memset(&ralloc, 0, sizeof(ralloc));
        ralloc.start    = EDMA3_MIN_PRIORITY;
        ralloc.end      = EDMA3_MAX_PRIORITY;
        ralloc.flags    = RSRCDBMGR_DMA_CHANNEL|RSRCDBMGR_FLAG_NOREMOVE;
        if ( rsrcdbmgr_create(&ralloc, 1) != EOK ) {
            perror("FSL_EDMA3 seed_shared_mem() Unable to seed dma channels\n");
        }
    }
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/lib/dma/fsl-edma3/init.c $ $Rev: 862698 $")
#endif

