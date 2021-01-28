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

#include "config.h"
#include "canvas_common.h"
#include "spidevice.h"
#include "fbscreen.h"
#include "canvascmd.h"

int32_t canvascmd_get_dimension(
    struct fbscreen *fbscreen,
    struct spidevice *spidevice
)
{
    int32_t result;
    uint8_t ack = CANVAS_ACK_DIMENSION;
    struct ack_dimension dimension = { 0 };

    dimension.width = fbscreen->var_info.xres;
    dimension.height = fbscreen->var_info.yres;
    dimension.pixel_bits = fbscreen->var_info.bits_per_pixel;

    canvas_dbg("cmd dimension: 0x%x\n", sizeof(dimension));
    canvas_dbg("width: 0x%x\n", dimension.width);
    canvas_dbg("height: 0x%x\n", dimension.height);

    if (0 > (result = spidevice_write(
        spidevice, (uint8_t*)&ack, sizeof(ack)
    )))
    {
        return result;
    }

    if (0 > (result = spidevice_write(
        spidevice, (uint8_t*)&dimension, sizeof(dimension)
    )))
    {
        return result;
    }
    return 0;
}

int32_t canvascmd_clear_screen(
    struct fbscreen *fbscreen,
    struct spidevice *spidevice
)
{
    int32_t result;
    struct cmd_clearscreen cmd_screen;

    if (0 > (result = spidevice_read(
        spidevice, (uint8_t*)&cmd_screen, sizeof(cmd_screen)
    )))
    {
        return result;
    }

    canvas_dbg("cmd clear screen: 0x%x\n", sizeof(cmd_screen));
    canvas_dbg("color: 0x%x\n", cmd_screen.color);

    if (0 > (result = fbscreen_clear_screen(
        fbscreen, cmd_screen.color
    )))
    {
        return result;
    }
    return 0;
}

int32_t canvascmd_draw_circle(
    struct fbscreen *fbscreen,
    struct spidevice *spidevice
)
{
    int32_t result;
    struct cmd_circle cmd_circle = {0};
    struct fbscreen_circle fb_circle = {0};

    if (0 > (result = spidevice_read(
        spidevice, (uint8_t*)&cmd_circle, sizeof(cmd_circle)
    )))
    {
        return result;
    }

    canvas_dbg("drawing circle: 0x%x\n", sizeof(cmd_circle));
    canvas_dbg("xpos: 0x%x\n", cmd_circle.xpos);
    canvas_dbg("ypos: 0x%x\n", cmd_circle.ypos);
    canvas_dbg("color: 0x%x\n", cmd_circle.color);
    canvas_dbg("in centre: 0x%x\n", cmd_circle.in_centre);
    canvas_dbg("radius: 0x%x\n", cmd_circle.radius);

    /* copy circle between 'command' and 'drawing' domain */
    fb_circle.xpos = cmd_circle.xpos;
    fb_circle.ypos = cmd_circle.ypos;
    fb_circle.color = cmd_circle.color;
    fb_circle.in_centre = cmd_circle.in_centre;
    fb_circle.radius = cmd_circle.radius;

    if (0 > (result = fbscreen_draw_circle(
        fbscreen, &fb_circle
    )))
    {
        return result;
    }
    return 0;
}

int32_t canvascmd_draw_rectangle(
    struct fbscreen *fbscreen,
    struct spidevice *spidevice
)
{
    int32_t result;
    struct cmd_rectangle cmd_rectangle = {0};
    struct fbscreen_rectangle fb_rectangle = {0};

    if (0 > (result = spidevice_read(
        spidevice, (uint8_t*)&cmd_rectangle, sizeof(cmd_rectangle)
    )))
    {
        return result;
    }

    canvas_dbg("cmd rectangle: 0x%x\n", sizeof(cmd_rectangle));
    canvas_dbg("xpos: 0x%x\n", cmd_rectangle.xpos);
    canvas_dbg("ypos: 0x%x\n", cmd_rectangle.ypos);
    canvas_dbg("color: 0x%x\n", cmd_rectangle.color);
    canvas_dbg("in centre: 0x%x\n", cmd_rectangle.in_centre);
    canvas_dbg("width: 0x%x\n", cmd_rectangle.width);
    canvas_dbg("height: 0x%x\n", cmd_rectangle.height);

    /* copy rectangle between 'command' and 'drawing' domain */
    fb_rectangle.xpos = cmd_rectangle.xpos;
    fb_rectangle.ypos = cmd_rectangle.ypos;
    fb_rectangle.color = cmd_rectangle.color;
    fb_rectangle.in_centre = cmd_rectangle.in_centre;
    fb_rectangle.width = cmd_rectangle.width;
    fb_rectangle.height = cmd_rectangle.height;

    if (0 > (result = fbscreen_draw_rectangle(
        fbscreen, &fb_rectangle
    )))
    {
        return result;
    }

    return 0;
}

int32_t canvascmd_get_color(
    struct fbscreen *fbscreen,
    struct spidevice *spidevice
)
{
    int32_t result;
    uint8_t ack = CANVAS_ACK_DIMENSION;
    struct cmd_getcolor cmd_getcolor;
    struct ack_getcolor ack_getcolor;

    /* read requested xpos, ypos */
    if (0 > (result = spidevice_read(
        spidevice, (uint8_t*)&cmd_getcolor, sizeof(cmd_getcolor)
    )))
    {
        return result;
    }

    canvas_dbg("cmd getcolor: 0x%x\n", sizeof(cmd_getcolor));
    canvas_dbg("xpos: 0x%x\n", cmd_getcolor.xpos);
    canvas_dbg("ypos: 0x%x\n", cmd_getcolor.ypos);

    /* get color from framebuffer */
    fbscreen_get_pixel(
        fbscreen, cmd_getcolor.xpos, cmd_getcolor.ypos, &ack_getcolor.color
    );

    canvas_dbg("ack getcolor: 0x%x\n", sizeof(ack_getcolor));
    canvas_dbg("color: 0x%x\n", ack_getcolor.color);

    /* send acknowledge */
    if (0 > (result = spidevice_write(
        spidevice, (uint8_t*)&ack, sizeof(ack)
    )))
    {
        return result;
    }

    /* send acknowledge data */
    if (0 > (result = spidevice_write(
        spidevice, (uint8_t*)&ack_getcolor, sizeof(ack_getcolor)
    )))
    {
        return result;
    }

    return 0;
}

int32_t canvascmd_flush_drawing(
    struct fbscreen *fbscreen,
    struct spidevice *spidevice
)
{
    canvas_dbg("flush drawing:\n");
    fbscreen_flush_drawing(fbscreen);
    return 0;
}

int32_t canvascmd_do_nothing(
    struct fbscreen *fbscreen,
    struct spidevice *spidevice
)
{
    canvas_dbg("cmd do nothing \n");
    return 0;
}
