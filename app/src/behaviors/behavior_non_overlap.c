/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_non_overlap

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct active_non_overlap {
    struct active_non_overlap *previous;
    struct active_non_overlap *next;
    bool is_pressed;
    uint32_t position;
    struct zmk_behavior_binding binding;
};

struct behavior_non_overlap_config {
    const char *behavior_dev;
};

struct behavior_non_overlap_data {
    struct active_non_overlap *head;
    struct active_non_overlap *tail;
    struct active_non_overlap *actives;
    const size_t keep_active_size;
};

/*
 * Non-overlap key presses are kept in a static array of `struct active_non_overlap`.
 * The array size defaults to 10 if `keep-active-size` is not specified.
 * The array size limits the number of key presses that non-overlap behavior can remember.
 * Each instance of non-overlap behavior has its own array.
 *
 * Non-overlap behavior must preserve the order of key presses.
 * Linked list is implemented to allow efficient queue operations.
 * When the number of key presses reaches its limit,
 * previous key presses will be deleted to accommodate new key presses in a FIFO manner.
 */

static inline struct active_non_overlap *find_empty_slot(struct behavior_non_overlap_data *data) {
    const size_t keep_active_size = data->keep_active_size;
    struct active_non_overlap *actives = data->actives;

    for (int i = 0; i < keep_active_size; i++) {
        struct active_non_overlap *active = &actives[i];
        if (!active->is_pressed) {
            return active;
        }
    }

    return NULL;
}

static inline bool matches_params(const struct zmk_behavior_binding *this,
                                  const struct zmk_behavior_binding *that) {
    return this->param1 == that->param1 && this->param2 == that->param2;
}

static inline bool matches_active(struct zmk_behavior_binding *binding, uint32_t position,
                                  const struct active_non_overlap *active) {
    return position == active->position && matches_params(binding, &active->binding);
}

static inline struct active_non_overlap *find_active(struct zmk_behavior_binding *binding,
                                                     uint32_t position,
                                                     struct active_non_overlap *tail) {
    for (struct active_non_overlap *active = tail; active != NULL; active = active->previous) {
        if (matches_active(binding, position, active)) {
            return active;
        }
    }

    return NULL;
}

static inline void release_binding(struct active_non_overlap *active, int64_t timestamp) {
    struct zmk_behavior_binding_event event = {
        .position = active->position,
        .timestamp = timestamp,
    };

    behavior_keymap_binding_released(&active->binding, event);
}

static inline void press_binding(struct active_non_overlap *active, int64_t timestamp) {
    struct zmk_behavior_binding_event event = {
        .position = active->position,
        .timestamp = timestamp,
    };

    behavior_keymap_binding_pressed(&active->binding, event);
}

static int behavior_non_overlap_init(const struct device *dev) { return 0; };

static int on_non_overlap_binding_pressed(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    LOG_DBG("position = %d, param1 = 0x%02X.", event.position, binding->param1);

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_non_overlap_data *data = dev->data;
    struct active_non_overlap *least_recent_active = data->head;
    struct active_non_overlap *most_recent_active = data->tail;
    struct active_non_overlap *new_active;

    if (most_recent_active == NULL) {
        LOG_DBG("First active.");
        new_active = data->actives;

        data->head = new_active;
    } else {
        LOG_DBG("New active. Release the previous active.");
        release_binding(most_recent_active, event.timestamp);

        new_active = find_empty_slot(data);
        if (new_active == NULL) {
            new_active = least_recent_active;
            most_recent_active->next = least_recent_active;

            data->head = least_recent_active->next;
            data->head->previous = NULL;
        }

        most_recent_active->next = new_active;
    }

    if (new_active == NULL) {
        LOG_ERR("New active is not available for some reason.");
        return ZMK_BEHAVIOR_OPAQUE;
    }

    new_active->is_pressed = true;
    new_active->position = event.position;
    new_active->binding.param1 = binding->param1;
    new_active->binding.param2 = binding->param2;
    press_binding(new_active, event.timestamp);

    if (new_active != most_recent_active) {
        new_active->previous = most_recent_active;
    }
    new_active->next = NULL;

    data->tail = new_active;

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_non_overlap_binding_released(struct zmk_behavior_binding *binding,
                                           struct zmk_behavior_binding_event event) {
    LOG_DBG("position = %d, param1 = 0x%02X.", event.position, binding->param1);

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_non_overlap_data *data = dev->data;
    struct active_non_overlap *least_recent_active = data->head;
    struct active_non_overlap *most_recent_active = data->tail;
    struct active_non_overlap *active = find_active(binding, event.position, most_recent_active);
    const int64_t timestamp = event.timestamp;

    if (active == NULL) {
        LOG_DBG("No existing active. Nothing to do here.");
        return ZMK_BEHAVIOR_OPAQUE;
    }

    active->is_pressed = false;

    if (active == most_recent_active) {
        LOG_DBG("This is the most recent active. Release it.");
        release_binding(most_recent_active, timestamp);

        most_recent_active = most_recent_active->previous;
        if (most_recent_active != NULL) {
            LOG_DBG("Previous active exists. Re-press it.");
            press_binding(most_recent_active, timestamp);

            most_recent_active->next = NULL;
        }

        data->tail = most_recent_active;
        return ZMK_BEHAVIOR_OPAQUE;
    }

    active->next->previous = active->previous;

    if (active == least_recent_active) {
        data->head = least_recent_active->next;
    } else {
        active->previous->next = active->next;
    }

    LOG_DBG("Matched active deleted.");
    return ZMK_BEHAVIOR_OPAQUE;
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static int non_overlap_parameter_metadata(const struct device *non_overlap,
                                          struct behavior_parameter_metadata *param_metadata) {
    const struct behavior_non_overlap_config *cfg = non_overlap->config;
    struct behavior_parameter_metadata child_metadata;

    int err = behavior_get_parameter_metadata(zmk_behavior_get_binding(cfg->behavior_dev),
                                              &child_metadata);
    if (err < 0) {
        LOG_WRN("Failed to get the non-overlap behavior parameter: %d", err);
        return err;
    }

    for (int s = 0; s < child_metadata.sets_len; s++) {
        const struct behavior_parameter_metadata_set *set = &child_metadata.sets[s];

        if (set->param2_values_len > 0) {
            return -ENOTSUP;
        }
    }

    *param_metadata = child_metadata;

    return 0;
}

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static struct behavior_driver_api behavior_non_overlap_driver_api = {
    .binding_pressed = on_non_overlap_binding_pressed,
    .binding_released = on_non_overlap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = non_overlap_parameter_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#define _SET_NULL(i, inst)                                                                         \
    {                                                                                              \
        .previous = NULL,                                                                          \
        .next = NULL,                                                                              \
        .is_pressed = false,                                                                       \
        .position = 0,                                                                             \
        .binding =                                                                                 \
            {                                                                                      \
                .behavior_dev = behavior_non_overlap_config_##inst.behavior_dev,                   \
                .param1 = 0,                                                                       \
                .param2 = 0,                                                                       \
            },                                                                                     \
    }

#define _KEEP_ACTIVE_SIZE(inst)                                                                    \
    COND_CODE_1(DT_INST_PROP(inst, no_active), (1), (DT_INST_PROP_OR(inst, keep_active_size, 10)))

#define _SET_ACTIVES(inst) {LISTIFY(_KEEP_ACTIVE_SIZE(inst), _SET_NULL, (, ), inst)}

#define KP_INST(inst)                                                                              \
    static const struct behavior_non_overlap_config behavior_non_overlap_config_##inst = {         \
        .behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE_BY_IDX(inst, bindings, 0)),                 \
    };                                                                                             \
    static struct active_non_overlap actives_##inst[] = _SET_ACTIVES(inst);                        \
    static struct behavior_non_overlap_data behavior_non_overlap_data_##inst = {                   \
        .head = NULL,                                                                              \
        .tail = NULL,                                                                              \
        .actives = actives_##inst,                                                                 \
        .keep_active_size = _KEEP_ACTIVE_SIZE(inst),                                               \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(                                                                       \
        inst, behavior_non_overlap_init, NULL, &behavior_non_overlap_data_##inst,                  \
        &behavior_non_overlap_config_##inst, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,     \
        &behavior_non_overlap_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
