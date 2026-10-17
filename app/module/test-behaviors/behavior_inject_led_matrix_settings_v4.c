/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_inject_led_matrix_settings_v4

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <drivers/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct led_matrix_settings_v4_injected {
    uint8_t on;
    uint8_t brightness;
    uint8_t selected_profile_id;
    uint8_t speed;
    uint8_t manual_hue_index;
    uint8_t selected_independent_profile_ids[4];
};

static int on_inject_led_matrix_settings_v4_binding_pressed(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    static const struct led_matrix_settings_v4_injected persisted = {
        .on = 1U,
        .brightness = 73U,
        .selected_profile_id = 0U,
        .speed = 2U,
        .manual_hue_index = 5U,
        .selected_independent_profile_ids = {1U, 0U, 3U, 2U},
    };

#if IS_ENABLED(CONFIG_SETTINGS_RUNTIME)
    int rc = settings_runtime_set("led_matrix/state", &persisted, sizeof(persisted));
    if (rc != 0) {
        LOG_ERR("LED matrix v4 settings inject failed: rc=%d", rc);
        return rc;
    }

    LOG_DBG("LED matrix v4 settings injected: bytes=%u ids=%u on=%d brt=%u profile=%u speed=%u "
            "hue=%u child0=%u child1=%u",
            (unsigned int)sizeof(persisted),
            (unsigned int)ARRAY_SIZE(persisted.selected_independent_profile_ids), persisted.on,
            persisted.brightness, persisted.selected_profile_id, persisted.speed,
            persisted.manual_hue_index, persisted.selected_independent_profile_ids[0],
            persisted.selected_independent_profile_ids[1]);
    return 0;
#else
    LOG_ERR("LED matrix v4 settings inject requires CONFIG_SETTINGS_RUNTIME");
    return -ENOTSUP;
#endif
}

static int on_inject_led_matrix_settings_v4_binding_released(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    return 0;
}

static const struct behavior_driver_api behavior_inject_led_matrix_settings_v4_driver_api = {
    .binding_pressed = on_inject_led_matrix_settings_v4_binding_pressed,
    .binding_released = on_inject_led_matrix_settings_v4_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_inject_led_matrix_settings_v4_driver_api);

#endif
