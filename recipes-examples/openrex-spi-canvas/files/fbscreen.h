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

#ifndef __FBSCREEN_H__
#define __FBSCREEN_H__

#include <stdint.h>
#include <linux/fb.h>

#ifndef CANVAS_RGBCOLOR
#   define CANVAS_RGBCOLOR(r, g, b)        ((uint32_t)( (((uint8_t)(r)) << 16) | (((uint8_t)(g)) << 8) | (((uint8_t)(b))) ))
#endif

#ifndef CANVAS_RGBCOLOR_RED
#   define CANVAS_RGBCOLOR_RED(color)      (((color) >> 16) & 0xFF)
#endif

#ifndef CANVAS_RGBCOLOR_GREEN
#   define CANVAS_RGBCOLOR_GREEN(color)    (((color) >> 8) & 0xFF)
#endif

#ifndef CANVAS_RGBCOLOR_BLUE
#   define CANVAS_RGBCOLOR_BLUE(color)     ((color) & 0xFF)
#endif


/* framebuffers group */
struct fbscreen {
    /* famebuffer data */
    int32_t fb_fd;
    uint8_t *fb_mem;
    uint32_t fb_mem_size;
    /* screen data */
    struct fb_var_screeninfo var_info;
    struct fb_fix_screeninfo fix_info;
    /* drawing data */
    uint8_t *drawing_mem;
    uint32_t drawing_mem_size;
    uint32_t drawing_idx;
    /* precalculated data for mem swap */
    uint32_t drawing_yoffsets[2];
    uint8_t* drawing_addrs[2];
};

/* fbscreen circle */
struct fbscreen_circle {
    int32_t xpos;
    int32_t ypos;
    uint32_t color;
    uint32_t in_centre;
    uint32_t radius;
};

/* fbscreen rectangle */
struct fbscreen_rectangle {
    int32_t xpos;
    int32_t ypos;
    uint32_t color;
    uint32_t in_centre;
    uint32_t width;
    uint32_t height;
};

int32_t fbscreen_init(
    struct fbscreen *fbscreen,
    const char *fb_path,
    const uint8_t color_depth
);

int32_t fbscreen_deinit(
    struct fbscreen *fbscreen
);

int32_t fbscreen_set_pixel(
    const struct fbscreen *fbscreen,
    const int32_t xpos,
    const int32_t ypos,
    const uint32_t color
);

int32_t fbscreen_get_pixel(
    const struct fbscreen *fbscreen,
    const int32_t xpos,
    const int32_t ypos,
    uint32_t *color
);

int32_t fbscreen_clear_screen(
    const struct fbscreen *fbscreen,
    const uint32_t color
);

int32_t fbscreen_draw_rectangle(
    const struct fbscreen *fbscreen,
    const struct fbscreen_rectangle *rectangle
);

int32_t fbscreen_draw_circle(
    const struct fbscreen *fbscreen,
    const struct fbscreen_circle *circle
);

int32_t fbscreen_flush_drawing
(
    struct fbscreen *fbscreen
);

#endif
