/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sensor_rotate_key_press

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <drivers/sensor.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ZMK_BHV_SENSOR_MAX_MODS 2

struct behavior_sensor_rotate_key_press_config {
    int modifier_key;
    int mod_timeout_ms;
};

struct active_mod_press {
    struct k_delayed_work work;
    const struct behavior_sensor_rotate_key_press_config *config;
    int64_t last_timestamp;
};

struct active_mod_press active_mod_presses[ZMK_BHV_SENSOR_MAX_MODS] = {};

void behavior_sensor_rotate_key_press_work_handler(struct k_work *item) {
    struct active_mod_press *c = CONTAINER_OF(item, struct active_mod_press, work);
    const struct behavior_sensor_rotate_key_press_config *cfg = c->config;

    /* timeout expired, release modifier */
    ZMK_EVENT_RAISE(
        zmk_keycode_state_changed_from_encoded(cfg->modifier_key, false, k_uptime_get()));
    c->config = NULL;
    c->last_timestamp = 0;
}

static int behavior_sensor_rotate_key_press_init(const struct device *dev) {
    static bool init_first_run = true;

    if (init_first_run) {
        init_first_run = false;
        for (int i = 0; i < ZMK_BHV_SENSOR_MAX_MODS; i++) {
            k_delayed_work_init(&active_mod_presses[i].work,
                                behavior_sensor_rotate_key_press_work_handler);
            active_mod_presses[i].config = NULL;
            active_mod_presses[i].last_timestamp = 0;
        }
    }
    return 0;
}

static int on_sensor_binding_triggered(struct zmk_behavior_binding *binding,
                                       const struct device *sensor, int64_t timestamp) {
    struct sensor_value value;
    int err;
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sensor_rotate_key_press_config *cfg = dev->config;

    uint32_t keycode;
    LOG_DBG("inc keycode 0x%02X dec keycode 0x%02X", binding->param1, binding->param2);

    err = sensor_channel_get(sensor, SENSOR_CHAN_ROTATION, &value);

    if (err) {
        LOG_WRN("Failed to ge sensor rotation value: %d", err);
        return err;
    }

    switch (value.val1) {
    case 1:
        keycode = binding->param1;
        break;
    case -1:
        keycode = binding->param2;
        break;
    default:
        return -ENOTSUP;
    }

    LOG_DBG("SEND %d", keycode);

    bool mod_timeout_enabled = cfg->mod_timeout_ms != -1;
    struct active_mod_press *c = NULL;

    if (mod_timeout_enabled) {
        // lookup slot
        for (int i = 0; i < ZMK_BHV_SENSOR_MAX_MODS; i++) {
            if (active_mod_presses[i].config == cfg) {
                c = &active_mod_presses[i];
                c->config = cfg;
            }
        }

        // nothing found, take a slot
        if (c == NULL) {
            for (int i = 0; i < ZMK_BHV_SENSOR_MAX_MODS; i++) {
                if (active_mod_presses[i].config == NULL) {
                    c = &active_mod_presses[i];
                    c->config = cfg;
                }
            }
        }

        if (c == NULL) {
            LOG_WRN("increase ZMK_BHV_SENSOR_MAX_MODS");
        }

        if ((timestamp - c->last_timestamp) < cfg->mod_timeout_ms) {
            /* another input, restart timeout */
            k_delayed_work_cancel(&c->work);
        } else {
            /* first time, activate modifier key */
            ZMK_EVENT_RAISE(
                zmk_keycode_state_changed_from_encoded(cfg->modifier_key, true, timestamp));

            // TODO: Better way to do this?
            k_msleep(5);
        }
    }

    ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_encoded(keycode, true, timestamp));

    if (mod_timeout_enabled) {
        c->last_timestamp = timestamp;
        k_delayed_work_submit(&c->work, K_MSEC(cfg->mod_timeout_ms));
    }

    // TODO: Better way to do this?
    k_msleep(5);

    return ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_encoded(keycode, false, timestamp));
}

static const struct behavior_driver_api behavior_sensor_rotate_key_press_driver_api = {
    .sensor_binding_triggered = on_sensor_binding_triggered,
};

#define KP_INST(n)                                                                                 \
    const struct behavior_sensor_rotate_key_press_config                                           \
        behavior_sensor_rotate_key_press_config_##n = {                                            \
            .modifier_key = DT_INST_PROP(n, modifier_key),                                         \
            .mod_timeout_ms = DT_INST_PROP(n, mod_timeout_ms),                                     \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, behavior_sensor_rotate_key_press_init, device_pm_control_nop, NULL,   \
                          &behavior_sensor_rotate_key_press_config_##n, APPLICATION,               \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                     \
                          &behavior_sensor_rotate_key_press_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
