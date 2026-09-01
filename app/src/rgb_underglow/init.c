/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <math.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <zephyr/drivers/led_strip.h>
#include <drivers/ext_power.h>

#include <zmk/rgb_underglow/init.h>
#include <zmk/rgb_underglow/state.h>
#include <zmk/rgb_underglow/rgb_underglow_base.h>
#include <zmk/rgb_underglow/current_status.h>

#include <zmk/usb.h>
#include <zmk/workqueue.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *led_strip;
static struct rgb_underglow_state *state;

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
static const struct device *const ext_power = DEVICE_DT_GET(DT_INST(0, zmk_ext_power_generic));
#endif

K_WORK_DEFINE(underglow_tick_work, zmk_rgb_ug_tick);

static void zmk_rgb_ug_tick_handler(struct k_timer *timer) {
    if (!state->on) {
        return;
    }

    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &underglow_tick_work);
}

K_TIMER_DEFINE(underglow_tick, zmk_rgb_ug_tick_handler, NULL);

int zmk_rgb_ug_on(void) {
    if (!led_strip)
        return -ENODEV;

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
    if (ext_power != NULL) {
        int rc = ext_power_enable(ext_power);
        if (rc != 0) {
            LOG_ERR("Unable to enable EXT_POWER: %d", rc);
        }
    }
#endif

    state->on = true;
    state->animation_step = 0;
    k_timer_start(&underglow_tick, K_NO_WAIT, K_MSEC(50));

    return 0;
}

static void zmk_rgb_ug_off_handler(struct k_work *work) {
    struct led_rgb pixels[STRIP_NUM_PIXELS];
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        pixels[i] = (struct led_rgb){r : 0, g : 0, b : 0};
    }

    led_strip_update_rgb(led_strip, pixels, STRIP_NUM_PIXELS);
}

K_WORK_DEFINE(underglow_off_work, zmk_rgb_ug_off_handler);

int zmk_rgb_ug_off(void) {
    if (!led_strip)
        return -ENODEV;

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
    if (ext_power != NULL) {
        int rc = ext_power_disable(ext_power);
        if (rc != 0) {
            LOG_ERR("Unable to disable EXT_POWER: %d", rc);
        }
    }
#endif

    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &underglow_off_work);

    k_timer_stop(&underglow_tick);
    state->on = false;

    return 0;
}

static int zmk_rgb_ug_init(void) {
    led_strip = DEVICE_DT_GET(STRIP_CHOSEN);

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
    if (!device_is_ready(ext_power)) {
        LOG_ERR("External power device \"%s\" is not ready", ext_power->name);
        return -ENODEV;
    }
#endif

    zmk_rgb_ug_state_init();
    zmk_rgb_ug_tools_init(led_strip);
    zmk_rgb_underglow_init();

    state = zmk_rgb_ug_get_state();
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB)
    state->on = zmk_usb_is_powered();
#endif

    if (state->on) {
        k_timer_start(&underglow_tick, K_NO_WAIT, K_MSEC(50));
    }

    return 0;
}

SYS_INIT(zmk_rgb_ug_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
