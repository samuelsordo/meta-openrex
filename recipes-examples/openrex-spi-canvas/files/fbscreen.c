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
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <sys/types.h>

#include "config.h"
#include "fbscreen.h"
#include "math.h"
#include "string.h"

/* transform generic R|G|B to display R|G|B according color info */
#define FBSCREEN_COLOR2PIX(off, len, val) ((((1 << (len)) - 1) & (val)) << (off))
#define FBSCREEN_PIX2COLOR(off, len, val) (((val) >> (off)) & ((1 << (len)) - 1))


/* https://www.kernel.org/doc/Documentation/fb/fbuffer.txt
 * https://www.kernel.org/doc/Documentation/fb/api.txt */

int32_t fbscreen_init(
    struct fbscreen *fbscreen,
    const char *fb_path,
    const uint8_t color_depth
)
{
    int32_t result = -1;

    assert(!((NULL == fbscreen) || (NULL == fb_path)));
    if ((NULL == fbscreen) || (NULL == fb_path))
        return -1;

    /* open framebuffer */
    fbscreen->fb_fd = open(fb_path, O_RDWR);
    assert(!(fbscreen->fb_fd < 0));
    if (fbscreen->fb_fd < 0) return -1;

    /* get fixed info */
    result = ioctl(fbscreen->fb_fd, FBIOGET_FSCREENINFO, &fbscreen->fix_info);
    assert(!(result < 0));
    if (result < 0) return -1;

    /* get variable info */
    result = ioctl(fbscreen->fb_fd, FBIOGET_VSCREENINFO, &fbscreen->var_info);
    assert(!(result < 0));
    if (result < 0) return -1;

    assert(!((color_depth != 16) && (color_depth != 24) && (color_depth != 32)));
    if ((color_depth != 16) && (color_depth != 24) && (color_depth != 32))
        return -1;

    /* try to double 'virtual_yres' according visible 'yres' */
    fbscreen->var_info.yres_virtual = fbscreen->var_info.yres * 2;
    fbscreen->var_info.xres_virtual = fbscreen->var_info.xres;
    fbscreen->var_info.xoffset = 0;
    fbscreen->var_info.yoffset = 0;
    fbscreen->var_info.bits_per_pixel = color_depth;
    // fbscreen->var_info.activate = FB_ACTIVATE_VBL;
    result = ioctl(fbscreen->fb_fd, FBIOPUT_VSCREENINFO, &fbscreen->var_info);
    assert(!(result < 0));
    if (result < 0) return -1;

    /* expected size of fb (virtual_yres * virtual_xres * depth) */
    fbscreen->fb_mem_size = fbscreen->var_info.yres_virtual * fbscreen->var_info.xres * (fbscreen->var_info.bits_per_pixel >> 3);
    /* map file into memory */
    fbscreen->fb_mem = mmap(
        NULL, fbscreen->fb_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbscreen->fb_fd, 0
    );
    assert(!(fbscreen->fb_mem <= 0));
    if (fbscreen->fb_mem <= 0)
    {
        fbscreen->fb_mem = NULL;
        return -1;
    }

    /* clear whole fb - set to black */
    memset(fbscreen->fb_mem, 0, fbscreen->fb_mem_size);

    /* prepare offsets, addresses */
    fbscreen->drawing_mem_size = fbscreen->var_info.yres * fbscreen->var_info.xres * (fbscreen->var_info.bits_per_pixel >> 3);
    fbscreen->drawing_yoffsets[0] = 0;
    fbscreen->drawing_yoffsets[1] = fbscreen->var_info.yres;
    fbscreen->drawing_addrs[0] = fbscreen->fb_mem;
    fbscreen->drawing_addrs[1] = fbscreen->fb_mem + fbscreen->drawing_mem_size;
    fbscreen->drawing_idx = 1;
    fbscreen->drawing_mem = fbscreen->drawing_addrs[fbscreen->drawing_idx];

    return 0;
}

int32_t fbscreen_deinit(
    struct fbscreen *fbscreen
)
{
    if (NULL == fbscreen)
        return -1;

    munmap(fbscreen->fb_mem, fbscreen->fb_mem_size);
    close(fbscreen->fb_fd);

    return 0;
}

int32_t fbscreen_set_pixel(
    const struct fbscreen *fbscreen,
    const int32_t xpos,
    const int32_t ypos,
    const uint32_t color
)
{
    uint8_t *color_addr;
    const struct fb_var_screeninfo *var_info = NULL;
    const struct fb_fix_screeninfo *fix_info = NULL;

    /* assert params */
    assert(!(NULL == fbscreen));
    if (NULL == fbscreen) return -1;

    /* get var/fix info */
    var_info = &fbscreen->var_info;
    fix_info = &fbscreen->fix_info;

    if (
        (xpos < 0) || (ypos < 0) || (xpos >= var_info->xres) || (ypos >= var_info->yres)
    ) return -2;

    /* TODO: fix integer range */
    uint32_t pixel_position = (ypos * var_info->xres * (var_info->bits_per_pixel >> 3)) + (xpos * (var_info->bits_per_pixel >> 3));

    /* calculate pixel base address */
    color_addr = fbscreen->drawing_mem + pixel_position;

    if (var_info->bits_per_pixel == 16)
    {
        uint16_t tmp_pixel = \
            FBSCREEN_COLOR2PIX(var_info->red.offset, var_info->red.length, CANVAS_RGBCOLOR_RED(color)) | \
            FBSCREEN_COLOR2PIX(var_info->green.offset, var_info->green.length, CANVAS_RGBCOLOR_GREEN(color)) | \
            FBSCREEN_COLOR2PIX(var_info->blue.offset, var_info->blue.length, CANVAS_RGBCOLOR_BLUE(color));
        *((uint16_t*)color_addr) = tmp_pixel;
    }
    else if (var_info->bits_per_pixel == 24)
    {
        uint32_t tmp_pixel = \
            FBSCREEN_COLOR2PIX(var_info->red.offset, var_info->red.length, CANVAS_RGBCOLOR_RED(color)) | \
            FBSCREEN_COLOR2PIX(var_info->green.offset, var_info->green.length, CANVAS_RGBCOLOR_GREEN(color)) | \
            FBSCREEN_COLOR2PIX(var_info->blue.offset, var_info->blue.length, CANVAS_RGBCOLOR_BLUE(color));
        /* processor must use little endian mode, copy byte by byte */
        *(((uint8_t*)color_addr) + 0) = *(((uint8_t*)&tmp_pixel) + 0);
        *(((uint8_t*)color_addr) + 1) = *(((uint8_t*)&tmp_pixel) + 1);
        *(((uint8_t*)color_addr) + 2) = *(((uint8_t*)&tmp_pixel) + 2);
    }
    else if (var_info->bits_per_pixel == 32)
    {
        uint32_t tmp_pixel = \
            FBSCREEN_COLOR2PIX(var_info->red.offset, var_info->red.length, CANVAS_RGBCOLOR_RED(color)) | \
            FBSCREEN_COLOR2PIX(var_info->green.offset, var_info->green.length, CANVAS_RGBCOLOR_GREEN(color)) | \
            FBSCREEN_COLOR2PIX(var_info->blue.offset, var_info->blue.length, CANVAS_RGBCOLOR_BLUE(color));
        *((uint32_t*)color_addr) = tmp_pixel;
    }
    else
    {
        return -3;
    }

    return 0;
}

int32_t fbscreen_get_pixel(
    const struct fbscreen *fbscreen,
    const int32_t xpos,
    const int32_t ypos,
    uint32_t *color
)
{
    uint8_t *color_addr;
    const struct fb_var_screeninfo *var_info = NULL;
    const struct fb_fix_screeninfo *fix_info = NULL;

    /* assert params */
    assert(!(NULL == fbscreen));
    if (NULL == fbscreen) return -1;

    /* get var/fix info */
    var_info = &fbscreen->var_info;
    fix_info = &fbscreen->fix_info;

    if (
        (xpos < 0) || (ypos < 0) || (xpos >= var_info->xres) || (ypos >= var_info->yres)
    ) return -2;

    /* TODO: fix integer range */
    uint32_t pixel_position = (ypos * var_info->xres * (var_info->bits_per_pixel >> 3)) + (xpos * (var_info->bits_per_pixel >> 3));

    /* calculate pixel base address */
    color_addr = fbscreen->drawing_mem + pixel_position;

    if (var_info->bits_per_pixel == 16)
    {
        uint16_t tmp_pixel = *((uint16_t*)color_addr);
        *color = CANVAS_RGBCOLOR(
            FBSCREEN_PIX2COLOR(var_info->red.offset, var_info->red.length, tmp_pixel),
            FBSCREEN_PIX2COLOR(var_info->green.offset, var_info->green.length, tmp_pixel),
            FBSCREEN_PIX2COLOR(var_info->blue.offset, var_info->blue.length, tmp_pixel)
        );
    }
    else if (var_info->bits_per_pixel == 24)
    {
        /* not tested */
        uint32_t tmp_pixel = 0;
        /* processor must use little endian mode, copy byte by byte */
        *(((uint8_t*)&tmp_pixel) + 0) = *(((uint8_t*)color_addr) + 0);
        *(((uint8_t*)&tmp_pixel) + 1) = *(((uint8_t*)color_addr) + 1);
        *(((uint8_t*)&tmp_pixel) + 2) = *(((uint8_t*)color_addr) + 2);
        *color = CANVAS_RGBCOLOR(
            FBSCREEN_PIX2COLOR(var_info->red.offset, var_info->red.length, tmp_pixel),
            FBSCREEN_PIX2COLOR(var_info->green.offset, var_info->green.length, tmp_pixel),
            FBSCREEN_PIX2COLOR(var_info->blue.offset, var_info->blue.length, tmp_pixel)
        );
    }
    else if (var_info->bits_per_pixel == 32)
    {
        /* not tested */
        uint32_t tmp_pixel = 0;
        *color = CANVAS_RGBCOLOR(
            FBSCREEN_PIX2COLOR(var_info->red.offset, var_info->red.length, tmp_pixel),
            FBSCREEN_PIX2COLOR(var_info->green.offset, var_info->green.length, tmp_pixel),
            FBSCREEN_PIX2COLOR(var_info->blue.offset, var_info->blue.length, tmp_pixel)
        );
    }
    else
    {
        return -3;
    }

    return 0;
}

int32_t fbscreen_clear_screen(
    const struct fbscreen *fbscreen,
    const uint32_t color
)
{
    /* get var/fix info */
    const struct fb_var_screeninfo *var_info = &fbscreen->var_info;

    for (int32_t i = 0; i < var_info->xres; i++)
    {
        for (int32_t j = 0; j < var_info->yres; j++)
        {
            fbscreen_set_pixel(fbscreen, i, j, color);
        }
    }

    return 0;
}

int32_t fbscreen_draw_rectangle(
    const struct fbscreen *fbscreen,
    const struct fbscreen_rectangle *rectangle
)
{
    assert(!(NULL == fbscreen || NULL == rectangle));
    if (NULL == fbscreen || NULL == rectangle)
    {
        return -1;
    }

    int32_t xpos = rectangle->xpos, ypos = rectangle->ypos;
    if (rectangle->in_centre)
    {
        xpos -= rectangle->width/2;
        ypos -= rectangle->height/2;
    }

    for (int32_t i = 0; i < rectangle->width; i++)
    {
        for (int32_t j = 0; j < rectangle->height; j++)
        {
            fbscreen_set_pixel(
                fbscreen, xpos + i, ypos + j, rectangle->color
            );
        }
    }

    return 0;
}

int32_t fbscreen_draw_circle(
    const struct fbscreen *fbscreen,
    const struct fbscreen_circle *circle
)
{
    assert(!(NULL == fbscreen || NULL == circle));
    if (NULL == fbscreen || NULL == circle)
    {
        return -1;
    }

    int32_t xpos = circle->xpos, ypos = circle->ypos;
    if (!circle->in_centre)
    {
        xpos += circle->radius;
        ypos += circle->radius;
    }

    int32_t xlimit;
    int32_t radius_sqr = circle->radius * circle->radius;

    for (int32_t i = 0; i < circle->radius; i++)
    {
        xlimit = sqrt(radius_sqr - (i * i));
        for (int32_t j = 0; j < xlimit; j++)
        {
            fbscreen_set_pixel(
                fbscreen, xpos + i, ypos + j, circle->color
            );
            fbscreen_set_pixel(
                fbscreen, xpos - i, ypos + j, circle->color
            );
            fbscreen_set_pixel(
                fbscreen, xpos + i, ypos - j, circle->color
            );
            fbscreen_set_pixel(
                fbscreen, xpos - i, ypos - j, circle->color
            );
        }
    }

    return 0;
}

int32_t fbscreen_flush_drawing
(
    struct fbscreen *fbscreen
)
{
    int32_t result = 0;
    int32_t useless = 0;

    assert(!(NULL == fbscreen));
    if (NULL == fbscreen) return -1;

    /* wait for sync */
    result = ioctl(fbscreen->fb_fd, FBIO_WAITFORVSYNC, &useless);
    assert(!(result < 0));
    if (result < 0) return -1;

    /* call ioctl to change/swap starting position on y-axis */
    fbscreen->var_info.activate = FB_ACTIVATE_VBL;
    fbscreen->var_info.yoffset = fbscreen->drawing_yoffsets[fbscreen->drawing_idx];
    result = ioctl(fbscreen->fb_fd, FBIOPAN_DISPLAY, &fbscreen->var_info);
    assert(!(result < 0));
    if (result < 0) return -1;

    /* calculate which half of memory is 'drawing' one.
     * calculate address of 'drawing' memory */
    uint32_t active_idx = fbscreen->drawing_idx;
    fbscreen->drawing_idx = (fbscreen->drawing_idx + 1) & 0x1;
    fbscreen->drawing_mem = fbscreen->drawing_addrs[fbscreen->drawing_idx];

    /* wait for sync */
    result = ioctl(fbscreen->fb_fd, FBIO_WAITFORVSYNC, &useless);
    assert(!(result < 0));
    if (result < 0) return -1;

    /* copy data from 'active' to 'inactive' memory before
     * drawing API will modify 'inactive' memory.
     * Comment out this line to speedup a drawing in a cost of
     * different picture for each half of 'memory' */
    memcpy(
        fbscreen->drawing_addrs[fbscreen->drawing_idx], 
        fbscreen->drawing_addrs[active_idx],
        fbscreen->drawing_mem_size
    );

    return 0;
}
