/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_snap_tap

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/behavior.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_snap_tap_config {
    uint8_t index;
};

// Global state storage for snap tap keys
#define MAX_SNAP_TAP_KEYS 16
struct snap_tap_state {
    uint32_t keycode;
    uint32_t opposing_keycode;
    bool is_physically_pressed;
    bool is_logically_pressed;
    uint32_t position;
};

static struct snap_tap_state snap_tap_states[MAX_SNAP_TAP_KEYS];
static int snap_tap_count = 0;

// Find state for given keycode combination
static struct snap_tap_state *find_snap_tap_state(uint32_t keycode, uint32_t opposing_keycode) {
    for (int i = 0; i < snap_tap_count; i++) {
        if (snap_tap_states[i].keycode == keycode &&
            snap_tap_states[i].opposing_keycode == opposing_keycode) {
            return &snap_tap_states[i];
        }
    }
    return NULL;
}

// Find state for opposing key
static struct snap_tap_state *find_opposing_state(uint32_t my_keycode) {
    for (int i = 0; i < snap_tap_count; i++) {
        if (snap_tap_states[i].opposing_keycode == my_keycode) {
            return &snap_tap_states[i];
        }
    }
    return NULL;
}

// Create or get state for keycode pair
static struct snap_tap_state *get_or_create_state(uint32_t keycode, uint32_t opposing_keycode,
                                                  uint32_t position) {
    struct snap_tap_state *state = find_snap_tap_state(keycode, opposing_keycode);
    if (state) {
        return state;
    }

    if (snap_tap_count >= MAX_SNAP_TAP_KEYS) {
        LOG_ERR("Maximum snap tap keys exceeded");
        return NULL;
    }

    state = &snap_tap_states[snap_tap_count++];
    state->keycode = keycode;
    state->opposing_keycode = opposing_keycode;
    state->is_physically_pressed = false;
    state->is_logically_pressed = false;
    state->position = position;

    LOG_DBG("Created snap tap state for keycode 0x%02X opposing 0x%02X", keycode, opposing_keycode);
    return state;
}

static int behavior_snap_tap_init(const struct device *dev) { return 0; }

static int on_snap_tap_binding_pressed(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    uint32_t keycode = binding->param1;
    uint32_t opposing_keycode = binding->param2;

    LOG_DBG("Snap tap pressed: keycode 0x%02X, opposing: 0x%02X", keycode, opposing_keycode);

    struct snap_tap_state *state = get_or_create_state(keycode, opposing_keycode, event.position);
    if (!state) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    state->is_physically_pressed = true;

    // Find opposing state and suppress it if needed
    struct snap_tap_state *opposing_state = find_opposing_state(keycode);
    if (opposing_state && opposing_state->is_physically_pressed &&
        opposing_state->is_logically_pressed) {
        LOG_DBG("Suppressing opposing key: 0x%02X", opposing_keycode);
        opposing_state->is_logically_pressed = false;
        raise_zmk_keycode_state_changed_from_encoded(opposing_keycode, false, event.timestamp);
    }

    // Press our key logically
    state->is_logically_pressed = true;
    return raise_zmk_keycode_state_changed_from_encoded(keycode, true, event.timestamp);
}

static int on_snap_tap_binding_released(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    uint32_t keycode = binding->param1;
    uint32_t opposing_keycode = binding->param2;

    LOG_DBG("Snap tap released: keycode 0x%02X", keycode);

    struct snap_tap_state *state = find_snap_tap_state(keycode, opposing_keycode);
    if (!state) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    state->is_physically_pressed = false;

    // Always release our key logically
    if (state->is_logically_pressed) {
        state->is_logically_pressed = false;
        raise_zmk_keycode_state_changed_from_encoded(keycode, false, event.timestamp);
    }

    // Check if opposing key should be restored
    struct snap_tap_state *opposing_state = find_opposing_state(keycode);
    if (opposing_state && opposing_state->is_physically_pressed &&
        !opposing_state->is_logically_pressed) {
        LOG_DBG("Restoring opposing key: 0x%02X", opposing_keycode);
        opposing_state->is_logically_pressed = true;
        raise_zmk_keycode_state_changed_from_encoded(opposing_keycode, true, event.timestamp);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_snap_tap_driver_api = {
    .binding_pressed = on_snap_tap_binding_pressed,
    .binding_released = on_snap_tap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata param_values[] = {
    {
        .display_name = "Key",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_HID_USAGE,
    },
    {
        .display_name = "Opposing Key",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_HID_USAGE,
    },
};

static const struct behavior_parameter_metadata_set param_metadata_set[] = {{
    .param1_values = param_values,
    .param1_values_len = 1,
    .param2_values = param_values,
    .param2_values_len = 1,
}};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(param_metadata_set),
    .sets = param_metadata_set,
};

#endif

#define ST_INST(n)                                                                                 \
    static const struct behavior_snap_tap_config behavior_snap_tap_config_##n = {                  \
        .index = n,                                                                                \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_snap_tap_init, NULL, NULL, &behavior_snap_tap_config_##n,  \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_snap_tap_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ST_INST)

#endif