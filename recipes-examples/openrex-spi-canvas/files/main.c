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

#include <linux/kd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "config.h"
#include "canvas_common.h"
#include "spidevice.h"
#include "fbscreen.h"
#include "canvascmd.h"


/* app action to CLI */
enum app_action{
    app_action_daemon = 0,
    app_action_help,
    app_action_info,
    app_action_demo
};

/* application settings */
#define PATH_SIZE 256
struct app_settings {
    char fb_path[PATH_SIZE + 1];
    uint32_t baudrate;
    char tty_path[PATH_SIZE + 1];
    char spidev_path[PATH_SIZE + 1];
    enum app_action action;
};

/* parse CLI params */
int32_t parse_opt(
    int argc,
    char **argv,
    struct option *long_options,
    struct app_settings *app_options
)
{
    int long_index = 0;
    int opt;

    assert(!(app_options == NULL));
    if (app_options == NULL) return -1;

    while (
        (opt = getopt_long(argc, argv,"f:t:s:b:hix", long_options, &long_index )) != -1
    )
    {
        switch (opt)
        {
            case 'f':
                strncpy(app_options->fb_path, optarg, PATH_SIZE);
            break;
            case 't':
                strncpy(app_options->tty_path, optarg, PATH_SIZE);
            break;
            case 's':
                strncpy(app_options->spidev_path, optarg, PATH_SIZE);
            break;
            case 'b':
                app_options->baudrate = atoi(optarg);
            break;
            case 'h':
                app_options->action = app_action_help;
            break;
            case 'i':
                app_options->action = app_action_info;
            break;
            case 'x':
                app_options->action = app_action_demo;
            break;
        }
    }

    return 0;
}

/* disable graphic terminal */
int32_t disable_tty(
    const char *tty_path
)
{
    /* switch tty to graphic mode
     * - get rid of blinking cursor
     * - disable sleep mode */
    int32_t tty_fd;
    int32_t result;

    tty_fd = open(tty_path, O_RDWR);
    assert(!(tty_fd < 0));
    if (tty_fd < 0)
        return tty_fd;
    result = ioctl(tty_fd, KDSETMODE, KD_GRAPHICS);
    assert(!(result < 0));
    close(tty_fd);
    return result < 0 ? -1 : 0;
}

/* daemon main loop */
int32_t run_daemon(
    struct fbscreen *fbscreen,
    struct spidevice *spidevice,
    struct canvascmd *commands
)
{
    uint8_t cmd_code;
    int32_t result;

    canvas_dbg("main loop started \n");
    for (volatile int32_t loop = 1; loop;)
    {
        result = spidevice_read(spidevice, &cmd_code, sizeof(cmd_code));
        assert(!(result < 0));
        if (result < 0) return -1;
        canvas_dbg("command code: 0x%x \n", cmd_code);
        for (int i = 0; commands[i].cmd_code; i++ )
        {
            canvas_dbg("current code: 0x%x \n", commands[i].cmd_code);
            if (commands[i].cmd_code == cmd_code)
            {
                canvas_dbg("executing\n");
                result = commands[i].cmd_exec(fbscreen, spidevice);
                assert(!(result < 0));
                if (result < 0) return -1;
                break;
            }
        }
    }
    return 0;
}

/* print framebuffer info */
int32_t print_info(
    struct fbscreen *fbscreen
)
{
    assert(!(NULL == fbscreen));
    if (NULL == fbscreen)
        return -1;

    printf("var.xres %d\n", fbscreen->var_info.xres);
    printf("var.yres %d\n", fbscreen->var_info.yres);
    printf("var.xres_virtual %d\n", fbscreen->var_info.xres_virtual);
    printf("var.yres_virtual %d\n", fbscreen->var_info.yres_virtual);
    printf("var.xoffset %d\n", fbscreen->var_info.xoffset);
    printf("var.yoffset %d\n", fbscreen->var_info.yoffset);
    printf("var.bits_per_pixel %d\n", fbscreen->var_info.bits_per_pixel);
    printf("var.grayscale %d\n", fbscreen->var_info.grayscale);
    printf("var.red %d %d %d\n", fbscreen->var_info.red.offset, fbscreen->var_info.red.length, fbscreen->var_info.red.msb_right);
    printf("var.green %d %d %d\n", fbscreen->var_info.green.offset, fbscreen->var_info.green.length, fbscreen->var_info.green.msb_right);
    printf("var.blue %d %d %d\n", fbscreen->var_info.blue.offset, fbscreen->var_info.blue.length, fbscreen->var_info.blue.msb_right);
    printf("var.transp %d %d %d\n", fbscreen->var_info.transp.offset, fbscreen->var_info.transp.length, fbscreen->var_info.transp.msb_right);
    printf("var.nonstd %d\n", fbscreen->var_info.nonstd);
    printf("var.activate %d\n", fbscreen->var_info.activate);
    printf("var.height %d\n", fbscreen->var_info.height);
    printf("var.width %d\n", fbscreen->var_info.width);
    printf("var.accel_flags %d\n", fbscreen->var_info.accel_flags);
    printf("var.pixclock %d\n", fbscreen->var_info.pixclock);
    printf("var.left_margin %d\n", fbscreen->var_info.left_margin);
    printf("var.right_margin %d\n", fbscreen->var_info.right_margin);
    printf("var.upper_margin %d\n", fbscreen->var_info.upper_margin);
    printf("var.lower_margin %d\n", fbscreen->var_info.lower_margin);
    printf("var.hsync_len %d\n", fbscreen->var_info.hsync_len);
    printf("var.vsync_len %d\n", fbscreen->var_info.vsync_len);
    printf("var.sync %d\n", fbscreen->var_info.sync);
    printf("var.vmode %d\n", fbscreen->var_info.vmode);
    printf("var.rotate %d\n", fbscreen->var_info.rotate);
    printf("var.colorspace %d\n", fbscreen->var_info.colorspace);

    printf("fix.smem_start %llu\n", fbscreen->fix_info.smem_start);
    printf("fix.smem_len %d\n", fbscreen->fix_info.smem_len);
    printf("fix.type %d\n", fbscreen->fix_info.type);
    printf("fix.type_aux %d\n", fbscreen->fix_info.type_aux);
    printf("fix.visual %d\n", fbscreen->fix_info.visual);
    printf("fix.xpanstep %d\n", fbscreen->fix_info.xpanstep);
    printf("fix.ypanstep %d\n", fbscreen->fix_info.ypanstep);
    printf("fix.ywrapstep %d\n", fbscreen->fix_info.ywrapstep);
    printf("fix.line_length %d\n", fbscreen->fix_info.line_length);
    printf("fix.mmio_start %llu\n", fbscreen->fix_info.mmio_start);
    printf("fix.mmio_len %d\n", fbscreen->fix_info.mmio_len);
    printf("fix.accel %d\n", fbscreen->fix_info.accel);
    printf("fix.capabilities %d\n", fbscreen->fix_info.capabilities);

    return 0;
}

/* run test demo */
int32_t run_demo(
    struct fbscreen *fbscreen
)
{
    fbscreen_clear_screen(fbscreen, CANVAS_COLOR_GREEN);
    fbscreen_flush_drawing(fbscreen);
    fbscreen_clear_screen(fbscreen, CANVAS_COLOR_GREEN);
    // rectangle
    struct fbscreen_rectangle rectangle1 = {
        .xpos = 0,
        .ypos = 60, //fbscreen->var_info.yres/2,
        .color = CANVAS_COLOR_RED,
        .width = 300,
        .height = 300,
        .in_centre = 0,
    };
    struct fbscreen_rectangle rectangle2 = {
        .xpos = 0,
        .ypos = 0, //fbscreen->var_info.yres/2,
        .color = CANVAS_COLOR_BLUE,
        .width = 300,
        .height = 300,
        .in_centre = 0,
    };
    struct fbscreen_rectangle rectangle3 = {
        .xpos = fbscreen->var_info.xres - 100,
        .ypos = 0, //fbscreen->var_info.yres/2,
        .color = CANVAS_COLOR_WHITE,
        .width = 100,
        .height = 100,
        .in_centre = 0,
    };
    struct fbscreen_rectangle rectangle4 = {
        .xpos = fbscreen->var_info.xres - 100,
        .ypos = fbscreen->var_info.yres - 100, //fbscreen->var_info.yres/2,
        .color = CANVAS_COLOR_BLACK,
        .width = 100,
        .height = 100,
        .in_centre = 0,
    };

    /* press key to launch demo */
    getchar();
    for (int i = 0; 1; i++)
    {
        
        fbscreen_draw_rectangle(fbscreen, &rectangle1);
        fbscreen_draw_rectangle(fbscreen, &rectangle2);
        if (i & 0x1)
        {
            fbscreen_draw_rectangle(fbscreen, &rectangle3);
            rectangle3.xpos -= 10;
            rectangle3.ypos += 10;
        }
        else
        {
            fbscreen_draw_rectangle(fbscreen, &rectangle4);
            rectangle4.xpos -= 10;
            rectangle4.ypos -= 10;
        }
        rectangle1.xpos += 1;
        rectangle1.ypos += 1;
        // rectangle1.color += 0x1;
        rectangle2.xpos += 1;
        rectangle2.ypos += 1;
        // rectangle2.color += 0x1;
        // usleep(1);
        fbscreen_flush_drawing(fbscreen);
    }
}

int32_t print_help(void)
{
    printf("openrex_spi_canvas -f /dev/fb0 -s /dev/spidev2.0 -t /dev/tty1 -b 400000 \n");
    printf("-f = path to framebuffer device \n");
    printf("-t = path to graphic TTY device that need to be disabled \n");
    printf("-s = path to spidev device that is connected to LPC \n");
    printf("-b = baudrate speed \n");
    printf("-i print info \n");
    printf("-x run test demo \n");
    return 0;
}

/* options to parse */
static struct option long_options[] = {
    { "framebuffer", required_argument, 0, 'f' },
    { "tty", required_argument, 0, 't' },
    { "spidev", required_argument, 0, 's' },
    { "baudrate", required_argument, 0, 'b' },
    { "help", no_argument, 0, 'h' },
    { "info", no_argument, 0, 'i' },
    { "demo", no_argument, 0, 'x' },
    { 0 },
};

/* supported commands */
struct canvascmd commands[] = {
    { CANVAS_CMD_CLEAR, canvascmd_clear_screen },
    { CANVAS_CMD_GETDIMENSION, canvascmd_get_dimension },
    { CANVAS_CMD_RECTANGLE, canvascmd_draw_rectangle },
    { CANVAS_CMD_CIRCLE, canvascmd_draw_circle },
    { CANVAS_CMD_FLUSH_DRAWING, canvascmd_flush_drawing },
    { CANVAS_CMD_DUMMY, canvascmd_do_nothing },
// other commands ...
// and NULL terminated list of commands
    {0},
};


struct fbscreen fbscreen = {0};
struct spidevice spidevice = {0};


int main(int argc, char **argv)
{
    struct app_settings settings = {0};
    int32_t result = 0;

    /* parse command line settings */
    parse_opt(argc, argv, (void*)&long_options, &settings);

    if ((app_action_help == settings.action) || (argc == 1))
    {
        print_help();
    }
    else
    {
        /* optional - disable graphics tty */
        if (NULL != settings.tty_path)
        {
            result = disable_tty(settings.tty_path);
            if (0 > result)
            {
                fprintf(stderr, "cannot disable tty '%s', error %d\n", settings.tty_path, result);
                return -1;
            }
        }

        /* initialize single framebuffer */
        result = fbscreen_init(&fbscreen, settings.fb_path, 16);
        if (0 > result)
        {
            fprintf(stderr, "cannot initialize framebuffer '%s', error %d\n", settings.fb_path, result);
            goto error1;
        }

        /* initialize spi device */
        result = spidevice_init(&spidevice, settings.spidev_path, settings.baudrate);
        if (0 > result)
        {
            fprintf(stderr, "cannot initialize spi device '%s', error %d\n", settings.spidev_path, result);
            goto error2;
        }

        /* perform action according CLI */
        if (app_action_info == settings.action)
        {
            print_info(&fbscreen);
        }
        else if (app_action_demo == settings.action)
        {
            run_demo(&fbscreen);
        }
        else
        {
            run_daemon(&fbscreen, &spidevice, commands);
        }

        error2:
            spidevice_deinit(&spidevice);
        error1:
            fbscreen_deinit(&fbscreen);
    }

    return result;
}

