/*
 * $QNXLicenseC:
 * Copyright 2018, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable
 * license fees to QNX Software Systems before you may reproduce,
 * modify or distribute this software, or any work that includes
 * all or part of this software.   Free development licenses are
 * available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email
 * licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review
 * this entire file for other proprietary rights or license notices,
 * as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <devctl.h>
#include <sys/iomsg.h>
#include <hw/dcmd_gpio_siul2.h>

#define RESMGR_PATH         "/dev/gpio"

int main(int argc, char *argv[]) {
    int fd;
    int gpio_pin = -1;
    bool gpio_set_output = false;
    bool gpio_set_input = false;
    bool gpio_read_pin = false;
    bool gpio_write_pin = false;
    int output_val = -1;
    int option;
    int ret;
    bool missing_params = false;
    char exit_str[500];

    while ((option = getopt(argc, argv, "iorw:p:")) != -1) {
        switch (option) {
            case 'i':
                gpio_set_input = true;
                break;
            case 'o':
                gpio_set_output = true;
                break;
            case 'r':
                gpio_read_pin = true;
                break;
            case 'w':
                gpio_write_pin = true;
                output_val = strtoul(optarg, &optarg, 0);
                break;
            case 'p':
                gpio_pin = strtoul(optarg, &optarg, 0);
                break;
            default:
            return -1;
        }
    }

    strcpy(exit_str, "");

    if (gpio_set_output && gpio_set_input) {
        strcat(exit_str, "cannot set GPIO as an output and an input, exiting...\n");
        missing_params = true;
    }

    if (gpio_read_pin && gpio_write_pin) {
        strcat(exit_str, "cannot write and read GPIO, exiting...\n");
        missing_params = true;
    }

    if (gpio_pin == -1) {
        strcat(exit_str, "gpio_pin (-p option) not specified\n");
        missing_params = true;
    }

    if (gpio_write_pin && output_val == -1) {
        strcat(exit_str, "write (-w option) not specified\n");
        missing_params = true;
    }

    if (missing_params) {
        fprintf(stderr, "Missing parameters, %sexiting...\n", exit_str);
        return -1;
    }

    fd = open(RESMGR_PATH, O_RDONLY);

    if (fd < 0) {
        printf("Unable to open %s - Reason: %s\n", RESMGR_PATH, strerror(errno));
        return -1;
    }

    if (gpio_set_output) {
        gpio_devctl_output_t gpio_output;
        gpio_output.pin_num = gpio_pin;

        ret = devctl(fd, DCMD_GPIO_SET_OUTPUT, &gpio_output, sizeof(gpio_output), NULL);
        if (ret > 0) {
            printf("DCMD_GPIO_SET_OUTPUT returned an error - %d\n", ret);
            return -1;
        }
    }

    if (gpio_set_input) {
        gpio_devctl_input_t gpio_input;
        gpio_input.pin_num = gpio_pin;

        ret = devctl(fd, DCMD_GPIO_SET_INPUT, &gpio_input, sizeof(gpio_input), NULL);
        if (ret > 0) {
            printf("DCMD_GPIO_SET_INPUT returned an error - %d\n", ret);
            return -1;
        }
    }

    if (gpio_write_pin) {
        gpio_devctl_write_t gpio_write;
        gpio_write.pin_num = gpio_pin;
        gpio_write.data = output_val;

        ret = devctl(fd, DCMD_GPIO_WRITE, &gpio_write, sizeof(gpio_write), NULL);
        if (ret > 0) {
            printf("DCMD_GPIO_WRITE returned an error - %d\n", ret);
            return -1;
        }
    }

    if (gpio_read_pin) {
        gpio_devctl_read_t gpio_read;
        gpio_read.pin_num = gpio_pin;

        ret = devctl(fd, DCMD_GPIO_READ, &gpio_read, sizeof(gpio_read), NULL);
        if (ret > 0) {
            printf("DCMD_GPIO_READ returned an error - %d\n", ret);
            return -1;
        }
        printf("Value of GPIO[%d]: %d\n", gpio_pin, gpio_read.data);
    }

    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/support/gpio-siul2/test/main.c $ $Rev: 859876 $")
#endif
