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

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <linux/spi/spidev.h>
#include <sys/types.h>
#include <string.h>

#include "spidevice.h"

int32_t spidevice_init(
    struct spidevice *spidevice,
    const char *dev_path,
    const uint32_t speed_hz
)
{
    assert(!(NULL == spidevice || NULL == dev_path));
    if (NULL == spidevice || NULL == dev_path)
        return -1;
    if ((spidevice->fd = open(dev_path, O_RDWR)) < 0)
        return -1;
    spidevice->speed_hz = speed_hz;
    return 0;
}

int32_t spidevice_deinit(
    struct spidevice *spidevice
)
{
    assert(!(NULL == spidevice));
    if (NULL == spidevice)
        return -1;
    if (spidevice->fd >= 0)
        close(spidevice->fd);
    return 0;
}

int32_t spidevice_transfer(
    const struct spidevice *spidevice,
    const struct spi_ioc_transfer *message
)
{
    // check params
    assert(!(NULL == spidevice || NULL == message || message->bits_per_word != 8));
    if (NULL == spidevice || NULL == message || message->bits_per_word != 8)
        return -1;

    assert(!(message->rx_buf == 0 && message->tx_buf == 0));
    if (message->rx_buf == 0 && message->tx_buf == 0)
        return -1;

    // clone message
    struct spi_ioc_transfer new_message = *message;
    int32_t result;
    uint8_t tx_dummy = 0xFF, rx_dummy;

    // update baudrate and length
    new_message.speed_hz = spidevice->speed_hz;
    new_message.len = 1;

    // use dummy rx/tx buffer
    if (0 == message->tx_buf)
    {
        new_message.tx_buf = &tx_dummy;
    }
    if (0 == message->rx_buf)
    {
        new_message.rx_buf = &rx_dummy;
    }

    // SPP peripheral on SLAVE requires 
    // deassert CS for each WORD
    for (int i = 0; i < message->len; i++)
    {
        if (0 != message->tx_buf)
        {
            new_message.tx_buf = message->tx_buf + i;
        }
        if (0 != message->rx_buf)
        {
            new_message.rx_buf = message->rx_buf + i;
        }
        // invoke transmit by SPIDEV module
        if ((result = ioctl(spidevice->fd, SPI_IOC_MESSAGE(1), &new_message)) < 0)
            return result;
    }
    return 0;
}

int32_t spidevice_write(
    const struct spidevice *spidevice,
    const uint8_t *buffer,
    const size_t size
)
{
    assert(!(NULL == spidevice || NULL == buffer || 0 == size));
    if (NULL == spidevice || NULL == buffer || 0 == size)
        return -1;

    struct spi_ioc_transfer message = {
        .tx_buf = (__u64)buffer,
        .rx_buf = 0,
        .len = size,
        .bits_per_word = 8,
    };

    return spidevice_transfer(spidevice, &message);
}

int32_t spidevice_read(
    const struct spidevice *spidevice,
    uint8_t *buffer,
    const size_t size
)
{
    assert(!(NULL == spidevice || NULL == buffer || 0 == size));
    if (NULL == spidevice || NULL == buffer || 0 == size)
        return -1;

    struct spi_ioc_transfer message = {
        .tx_buf = 0,
        .rx_buf = (__u64)buffer,
        .len = size,
        .bits_per_word = 8,
    };

    return spidevice_transfer(spidevice, &message);
}
