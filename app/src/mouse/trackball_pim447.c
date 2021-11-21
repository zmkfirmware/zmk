/*
 * Copyright (c) 2021 Cedric VINCENT
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/sensor.h>
#include <logging/log.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/trackball_pim447.h>

LOG_MODULE_REGISTER(PIM447, CONFIG_SENSOR_LOG_LEVEL);

#define MOVE_FACTOR    DT_PROP(DT_INST(0, pimoroni_trackball_pim447), move_factor)
#define MOVE_X_INVERT  DT_PROP(DT_INST(0, pimoroni_trackball_pim447), invert_move_x)
#define MOVE_Y_INVERT  DT_PROP(DT_INST(0, pimoroni_trackball_pim447), invert_move_y)
#define MOVE_X_FACTOR  (MOVE_FACTOR * (MOVE_X_INVERT ? -1 : 1))
#define MOVE_Y_FACTOR  (MOVE_FACTOR * (MOVE_Y_INVERT ? -1 : 1))

#define SCROLL_DIVISOR    DT_PROP(DT_INST(0, pimoroni_trackball_pim447), scroll_divisor)
#define SCROLL_X_INVERT   DT_PROP(DT_INST(0, pimoroni_trackball_pim447), invert_scroll_x)
#define SCROLL_Y_INVERT   DT_PROP(DT_INST(0, pimoroni_trackball_pim447), invert_scroll_y)
#define SCROLL_X_DIVISOR  (SCROLL_DIVISOR * (SCROLL_X_INVERT ? -1 : 1))
#define SCROLL_Y_DIVISOR  (SCROLL_DIVISOR * (SCROLL_Y_INVERT ?  1 : -1))

#define BUTTON    DT_PROP(DT_INST(0, pimoroni_trackball_pim447), button)
#define SWAP_AXES DT_PROP(DT_INST(0, pimoroni_trackball_pim447), swap_axes)

static int mode = DT_PROP(DT_INST(0, pimoroni_trackball_pim447), mode);

void zmk_trackball_pim447_set_mode(int new_mode)
{
    switch (new_mode) {
        case PIM447_MOVE:
        case PIM447_SCROLL:
            mode = new_mode;
            break;

       case PIM447_TOGGLE:
            mode = mode == PIM447_MOVE
                   ? PIM447_SCROLL
                   : PIM447_MOVE;
            break;

       default:
            break;
    }
}

/*
 * It feels more natural and more confortable to convert the speed
 * reported by the PIM447 trackball.
 */
static int16_t convert_speed(int32_t value)
{
    bool negative = (value < 0);

    if (negative) {
        value = -value;
    }

    switch (value) {
        case 0:  value = 0;   break;
        case 1:  value = 1;   break;
        case 2:  value = 4;   break;
        case 3:  value = 8;   break;
        case 4:  value = 18;  break;
        case 5:  value = 32;  break;
        case 6:  value = 50;  break;
        case 7:  value = 72;  break;
        case 8:  value = 98;  break;
        default: value = 127; break;
    }

    if (negative) {
        value = -value;
    }

    return value;
}

static void thread_code(void *p1, void *p2, void *p3)
{
    const struct device *dev;
    int result;

    /* PIM447 trackball initialization. */

    const char *label = DT_LABEL(DT_INST(0, pimoroni_trackball_pim447));
    dev = device_get_binding(label);
    if (dev == NULL) {
        LOG_ERR("Cannot get TRACKBALL_PIM447 device");
        return;
    }

    /* Event loop. */

    bool button_press_sent   = false;
    bool button_release_sent = false;

    while (true) {
        struct sensor_value pos_dx, pos_dy, pos_dz;
        bool send_report = false;
        int clear = PIM447_NONE;

        result = sensor_sample_fetch(dev);
        if (result < 0) {
            LOG_ERR("Failed to fetch TRACKBALL_PIM447 sample");
            return;
        }

        result = sensor_channel_get(dev, SENSOR_CHAN_POS_DX, &pos_dx);
        if (result < 0) {
            LOG_ERR("Failed to get TRACKBALL_PIM447 pos_dx channel value");
            return;
        }

        result = sensor_channel_get(dev, SENSOR_CHAN_POS_DY, &pos_dy);
        if (result < 0) {
            LOG_ERR("Failed to get TRACKBALL_PIM447 pos_dy channel value");
            return;
        }

        result = sensor_channel_get(dev, SENSOR_CHAN_POS_DZ, &pos_dz);
        if (result < 0) {
            LOG_ERR("Failed to get TRACKBALL_PIM447 pos_dz channel value");
            return;
        }

        if (pos_dx.val1 != 0 || pos_dy.val1 != 0) {
            if (SWAP_AXES) {
                int32_t tmp = pos_dx.val1;
                pos_dx.val1 = pos_dy.val1;
                pos_dy.val1 = tmp;
            }

            switch(mode) {
                default:
                case PIM447_MOVE: {
                    int dx = convert_speed(pos_dx.val1) * MOVE_X_FACTOR;
                    int dy = convert_speed(pos_dy.val1) * MOVE_Y_FACTOR;
                    zmk_hid_mouse_movement_set(dx, dy);
                    send_report = true;
                    clear = PIM447_MOVE;
                    break;
                }

                case PIM447_SCROLL: {
                    int dx = pos_dx.val1 / SCROLL_X_DIVISOR;
                    int dy = pos_dy.val1 / SCROLL_Y_DIVISOR;
                    zmk_hid_mouse_scroll_set(dx, dy);
                    send_report = true;
                    clear = PIM447_SCROLL;
                    break;
                }
            }
        }

        if (pos_dz.val1 == 0x80 && button_press_sent == false) {
            zmk_hid_mouse_button_press(BUTTON);
            button_press_sent   = true;
            button_release_sent = false;
            send_report = true;
        } else if (pos_dz.val1 == 0x01 && button_release_sent == false) {
            zmk_hid_mouse_button_release(BUTTON);
            button_press_sent   = false;
            button_release_sent = true;
            send_report = true;
        }

        if (send_report) {
            zmk_endpoints_send_mouse_report();

            switch (clear) {
                case PIM447_MOVE: zmk_hid_mouse_movement_set(0, 0); break;
                case PIM447_SCROLL: zmk_hid_mouse_scroll_set(0, 0); break;
                default: break;
            }
        }

        k_sleep(K_MSEC(10));
    }
}

#define STACK_SIZE 1024

static K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);
static struct k_thread thread;

int zmk_trackball_pim447_init()
{
    k_thread_create(&thread, thread_stack, STACK_SIZE, thread_code,
                    NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
    return 0;
}

SYS_INIT(zmk_trackball_pim447_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
