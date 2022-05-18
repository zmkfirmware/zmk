/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <devicetree.h>
#include <pm/device.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/display.h>
#include <lvgl.h>

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/display/status_screen.h>
#include <zmk/power_domain.h>

#define ZMK_DISPLAY_NAME CONFIG_LVGL_DISPLAY_DEV_NAME

#if CONFIG_ZMK_EXT_POWER

#define DISPLAY_POWER_DOMAIN DT_LABEL(DT_PHANDLE(DT_CHOSEN(zephyr_display), power_domain))

#endif

static const struct device *display;
static bool initialized = false;
bool power_domain_enabled = true;
bool keyboard_active = true;

static lv_obj_t *screen;

__attribute__((weak)) lv_obj_t *zmk_display_status_screen() { return NULL; }

void display_tick_cb(struct k_work *work) { lv_task_handler(); }

#define TICK_MS 10

int zmk_display_init_power_domain_manager(const struct device *dev);

K_WORK_DEFINE(display_tick_work, display_tick_cb);

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED)

K_THREAD_STACK_DEFINE(display_work_stack_area, CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE);

static struct k_work_q display_work_q;

#endif

struct k_work_q *zmk_display_work_q() {
#if IS_ENABLED(CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED)
    return &display_work_q;
#else
    return &k_sys_work_q;
#endif
}

void display_timer_cb() {
    lv_tick_inc(TICK_MS);
    k_work_submit_to_queue(zmk_display_work_q(), &display_tick_work);
}

void blank_display_cb(struct k_work *work) { display_blanking_on(display); }

void unblank_display_cb(struct k_work *work) { display_blanking_off(display); }

K_TIMER_DEFINE(display_timer, display_timer_cb, NULL);
K_WORK_DEFINE(blank_display_work, blank_display_cb);
K_WORK_DEFINE(unblank_display_work, unblank_display_cb);

static void start_display_updates() {
    if (display == NULL) {
        return;
    }

    k_work_submit_to_queue(zmk_display_work_q(), &unblank_display_work);

    k_timer_start(&display_timer, K_MSEC(TICK_MS), K_MSEC(TICK_MS));
}

static void stop_display_updates() {
    if (display == NULL) {
        return;
    }

    k_work_submit_to_queue(zmk_display_work_q(), &blank_display_work);

    k_timer_stop(&display_timer);
}

int zmk_display_is_initialized() { return initialized; }

// This function must be called from `main()`, otherwise the firmware will
// crash in zmk_display_status_screen.
int zmk_display_init() {
    LOG_DBG("");

    display = device_get_binding(ZMK_DISPLAY_NAME);
    if (display == NULL) {
        LOG_ERR("Failed to find display device");
        return -EINVAL;
    }

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED)
    k_work_queue_start(&display_work_q, display_work_stack_area,
                       K_THREAD_STACK_SIZEOF(display_work_stack_area),
                       CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_PRIORITY, NULL);
#endif

    screen = zmk_display_status_screen();

    if (screen == NULL) {
        LOG_ERR("No status screen provided");
        return 0;
    }

    lv_scr_load(screen);

    if(power_domain_enabled == true) {
        LOG_DBG("Starting display updates");
        start_display_updates();
    }

    initialized = true;

    LOG_DBG("");
    return 0;
}

int display_event_handler(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    switch (ev->state) {
    case ZMK_ACTIVITY_ACTIVE:
        keyboard_active = true;

        if(power_domain_enabled) {
            LOG_DBG("Starting display updates, because keyboard is active and power domain enabled.");
            start_display_updates();
        } else {
            LOG_DBG("Not enabling display updates even though keyboard is active, because power domain is disabled.");
        }
        break;
    case ZMK_ACTIVITY_IDLE:
    case ZMK_ACTIVITY_SLEEP:
        keyboard_active = false;

        if(power_domain_enabled) {
            LOG_DBG("Stopping display updates, because keyboard is not active and power domain enabled.");
            stop_display_updates();
        } else {
            LOG_DBG("Not stopping display updates even though keyboard is idle, because power domain is disabled.");
        }
        break;
    default:
        LOG_WRN("Unhandled activity state: %d", ev->state);
        return -EINVAL;
    }
    return 0;
}

ZMK_LISTENER(display, display_event_handler);
ZMK_SUBSCRIPTION(display, zmk_activity_state_changed);

#if CONFIG_ZMK_EXT_POWER

static int zmk_display_pm_action(const struct device *dev, enum pm_device_action action) {
	LOG_DBG("In zmk_display_pm_action on dev %s with action: %d", dev->name, action);

    switch (action) {
        case PM_DEVICE_ACTION_RESUME:
            power_domain_enabled = true;

            if(initialized == true) {
        	    LOG_DBG("Power domain enabled - Enabling display updates");
                start_display_updates();
            }

            break;
        case PM_DEVICE_ACTION_TURN_OFF:
            power_domain_enabled = false;

            if(initialized == true) {
        	    LOG_DBG("Power domain disabled - Disabling display updates");
                stop_display_updates();
            }

            break;
        default:
            LOG_ERR("PM Action %d not implemented", action);
            return -ENOTSUP;
    }

    return 0;
}


int zmk_display_init_power_domain_manager(const struct device *dev) {

    LOG_DBG("");
    const struct device *power_domain = device_get_binding(DISPLAY_POWER_DOMAIN);
    if (power_domain == NULL) {

        LOG_ERR("Unable to retrieve display's power_domain device... is the power-domain property set?");
        return -1;
    }

    return zmk_power_domain_init_power_domain_manager_helper(dev, power_domain);
}

PM_DEVICE_DEFINE(pd_manager_display, zmk_display_pm_action);
DEVICE_DEFINE(pd_manager_display, "pd_manager_display", &zmk_display_init_power_domain_manager, PM_DEVICE_GET(pd_manager_display), NULL, NULL, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY, NULL);

#endif // CONFIG_ZMK_EXT_POWER
