/**
 *  Copyright 2016 
 *  Marian Cingel - cingel.marian@gmail.com
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 *  specific language governing permissions and limitations
 *  under the License.
 */

#include <stdio.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// #define EXAMPLE_SPEED 100000
// #define EXAMPLE_SPIDEV_PATH "/dev/spidev2.0"
#define BUFFER_SIZE 4 /* this value must be exactly the same as spi_slave on MCU */


/* application options structure */
#define PATH_SIZE 256
struct app_options {
    uint32_t baudrate;
    char spidev_path[PATH_SIZE + 1];
    int print_help;
};

int32_t parse_opt(
    int argc,
    char **argv,
    struct option *long_options,
    struct app_options *app_options
)
{
    int long_index = 0;
    int opt;

    assert(!(app_options == NULL));
    if (app_options == NULL) return -1;

    while (
        (opt = getopt_long(argc, argv,"s:b:h", long_options, &long_index )) != -1
    )
    {
        switch (opt)
        {
            case 's':
                strncpy(app_options->spidev_path, optarg, PATH_SIZE);
            break;
            case 'b':
                app_options->baudrate = atoi(optarg);
            break;
            case 'h':
                app_options->print_help = 1;
            break;
        }
    }

    return 0;
}


int32_t transfer_message(int fd, struct spi_ioc_transfer *message_p)
{
    int32_t result;
    uint8_t *rx_sign_p;
    /* call io control - spidev kernel module will 
     * transfer then 'message' */
    if ((result = ioctl(fd, SPI_IOC_MESSAGE(1), message_p) < 0))
    {
        fprintf(stderr, "cannot call ioctl, status is: %d \n", result);
        return -1;
    }
    rx_sign_p = (uint8_t*)message_p->rx_buf;
    /* not a dummy char - put to stdin */
    if (*rx_sign_p != 0xFF)
    {
        write(STDIN_FILENO, rx_sign_p, sizeof(*rx_sign_p));
    }
    return 0;
}


int32_t main_loop(const char *dev_path, uint32_t speed)
{
    uint8_t tx_sign, rx_sign;
    struct spi_ioc_transfer message = {0};
    int32_t no_error = 1, result = 0, fd;

    /* open spi device */
    if ((fd = open(dev_path, O_RDWR)) < 0)
    {
        fprintf(stderr, "cannot initialize spi device '%s', error %d\n", dev_path, fd);
        return -1;
    }

    /* LPC SSP peripheral requires assert/deassert
     * CS for each word so we have to transfer
     * each character separately */
    message.tx_buf = &tx_sign;
    message.rx_buf = &rx_sign;
    message.len = sizeof(tx_sign);
    message.speed_hz = speed;
    message.bits_per_word = 8;

    /* note: CSPI mode should be set for CPHA0 CPOL0 by default */
    while (read(STDIN_FILENO, (void*)&tx_sign, sizeof(tx_sign)) > 0)
    {
        result = transfer_message(fd, &message);
        assert(!(0 > result));
        if (0 > result) break;

        /* if transfered char is newline, flush the queue on slave 
         * by sending 0xFF of BUFFER_SIZE count */
        if (tx_sign == '\n')
        {
            int32_t has_error = 0;
            tx_sign = 0xFF;
            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                result = transfer_message(fd, &message);
                assert(!(0 > result));
                if (0 > result)
                {
                    has_error = 1;
                    break;
                }
            }
            if (has_error) break;
        }
    }

    close(fd);
    return result;
}

int32_t print_help(void)
{
    printf("openrex_spi_slave -s /dev/spidevice2.0 -b 100000 \n");
    printf("-s = path to spidev device that is connected to LPC \n");
    printf("-b = baudrate speed \n");
    return 0;
}

/* options to parse */
static struct option long_options[] = {
    { "framebuffer", required_argument, 0, 'f' },
    { "tty", required_argument, 0, 't' },
    { "spidev", required_argument, 0, 's' },
    { "baudrate", required_argument, 0, 'b' },
    { "help", required_argument, 0, '-h' },
    { 0 },
};

int main(int argc, char *argv[])
{
    struct app_options options = {0};
    int result = 0;

    /* parse command line options */
    parse_opt(argc, argv, &long_options, &options);

    if (options.print_help)
    {
        print_help();
    }
    else
    {
        result = main_loop(options.spidev_path, options.baudrate);
    }

    return result;
}
