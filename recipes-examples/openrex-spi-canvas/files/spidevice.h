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

#ifndef __SPIDEVICE_H__
#define __SPIDEVICE_H__ 1

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <linux/spi/spidev.h>
#include <sys/types.h>

struct spidevice {
    int32_t fd;
    uint32_t speed_hz;
};

int32_t spidevice_init(
    struct spidevice *spidevice,
    const char *dev_path,
    uint32_t speed_hz
);

int32_t spidevice_deinit(
    struct spidevice *spidevice
);

int32_t spidevice_transfer(
    const struct spidevice *spidevice,
    const struct spi_ioc_transfer *message
);

int32_t spidevice_write(
    const struct spidevice *spidevice,
    const uint8_t *buffer,
    const size_t size
);

int32_t spidevice_read(
    const struct spidevice *spidevice,
    uint8_t *buffer,
    const size_t size
);

#endif
