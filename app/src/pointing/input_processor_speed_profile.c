/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_speed_profile

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <drivers/input_processor.h>
#include <drivers/input_processor_speed_profile.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define SPEED_PERCENT_DENOMINATOR 100

struct speed_profile_config {
    uint16_t type;
    size_t codes_len;
    const uint16_t *codes;
    uint8_t level_count;
    uint8_t default_level;
    const uint16_t *percentages;
    bool persist;
};

struct speed_profile_data {
    const struct device *dev;
    atomic_t selected_level;
#if IS_ENABLED(CONFIG_SETTINGS)
    struct k_work_delayable save_work;
#endif
};

static bool speed_profile_handles_event(const struct speed_profile_config *config,
                                        const struct input_event *event) {
    if (event->type != config->type) {
        return false;
    }

    for (size_t i = 0; i < config->codes_len; i++) {
        if (event->code == config->codes[i]) {
            return true;
        }
    }

    return false;
}

static int speed_profile_handle_event(const struct device *dev, struct input_event *event,
                                      uint32_t param1, uint32_t param2,
                                      struct zmk_input_processor_state *state) {
    const struct speed_profile_config *config = dev->config;
    struct speed_profile_data *data = dev->data;

    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    if (!speed_profile_handles_event(config, event)) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    uint8_t level = (uint8_t)atomic_get(&data->selected_level);
    uint16_t percentage = config->percentages[level - 1U];
    int32_t input_value = event->value;
    int64_t numerator = (int64_t)event->value * percentage;

    if (state != NULL && state->remainder != NULL) {
        numerator += *state->remainder;
    }

    int64_t scaled = numerator / SPEED_PERCENT_DENOMINATOR;
    bool saturated = scaled < INT16_MIN || scaled > INT16_MAX;
    event->value = (int32_t)CLAMP(scaled, (int64_t)INT16_MIN, (int64_t)INT16_MAX);

    if (state != NULL && state->remainder != NULL) {
        *state->remainder =
            saturated ? 0 : (int16_t)(numerator - (scaled * SPEED_PERCENT_DENOMINATOR));
    }

    LOG_DBG("Input speed scaled %d at level %u (%u%%) to %d with remainder %d", input_value, level,
            percentage, event->value,
            (state != NULL && state->remainder != NULL) ? *state->remainder : 0);

    return ZMK_INPUT_PROC_CONTINUE;
}

static int speed_profile_get_level(const struct device *dev, uint8_t *level) {
    struct speed_profile_data *data = dev->data;

    if (level == NULL) {
        return -EINVAL;
    }

    *level = (uint8_t)atomic_get(&data->selected_level);
    return 0;
}

static int speed_profile_get_level_count(const struct device *dev, uint8_t *count) {
    const struct speed_profile_config *config = dev->config;

    if (count == NULL) {
        return -EINVAL;
    }

    *count = config->level_count;
    return 0;
}

#if IS_ENABLED(CONFIG_SETTINGS)
static void speed_profile_save_work_handler(struct k_work *work) {
    struct k_work_delayable *delayable = k_work_delayable_from_work(work);
    struct speed_profile_data *data = CONTAINER_OF(delayable, struct speed_profile_data, save_work);
    const struct speed_profile_config *config = data->dev->config;
    uint8_t level = (uint8_t)atomic_get(&data->selected_level);
    char path[SETTINGS_MAX_NAME_LEN + 1];

    if (!config->persist) {
        return;
    }

    int len = snprintf(path, sizeof(path), "input_speed/%s", data->dev->name);
    if (len < 0 || (size_t)len >= sizeof(path)) {
        LOG_ERR("Speed profile settings path is too long for %s", data->dev->name);
        return;
    }

    int err = settings_save_one(path, &level, sizeof(level));
    if (err != 0) {
        LOG_ERR("Failed saving input speed level for %s (%d)", data->dev->name, err);
    } else {
        LOG_DBG("Input speed saved level %u for %s", level, data->dev->name);
    }
}
#endif

static int speed_profile_set_level(const struct device *dev, uint8_t level) {
    const struct speed_profile_config *config = dev->config;
    struct speed_profile_data *data = dev->data;

    if (level == 0U || level > config->level_count) {
        return -EINVAL;
    }

    uint8_t previous = (uint8_t)atomic_get(&data->selected_level);
    if (level == previous) {
        return 0;
    }

    atomic_set(&data->selected_level, level);
    LOG_INF("Input speed level %u/%u (%u%%)", level, config->level_count,
            config->percentages[level - 1U]);

#if IS_ENABLED(CONFIG_SETTINGS)
    if (config->persist) {
        int err = k_work_reschedule(&data->save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
        return MIN(err, 0);
    }
#endif

    return 0;
}

static int speed_profile_init(const struct device *dev) {
    const struct speed_profile_config *config = dev->config;
    struct speed_profile_data *data = dev->data;

    data->dev = dev;
    atomic_set(&data->selected_level, config->default_level);
#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init_delayable(&data->save_work, speed_profile_save_work_handler);
#endif

    return 0;
}

#if IS_ENABLED(CONFIG_SETTINGS)
#define SPEED_PROFILE_DEVICE(inst) DEVICE_DT_INST_GET(inst),

static const struct device *const speed_profile_devices[] = {
    DT_INST_FOREACH_STATUS_OKAY(SPEED_PROFILE_DEVICE)};

static int speed_profile_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                      void *cb_arg) {
    for (size_t i = 0; i < ARRAY_SIZE(speed_profile_devices); i++) {
        const struct device *dev = speed_profile_devices[i];
        const struct speed_profile_config *config = dev->config;
        struct speed_profile_data *data = dev->data;
        const char *next;

        if (!config->persist || !settings_name_steq(name, dev->name, &next) || next != NULL) {
            continue;
        }

        if (len != sizeof(uint8_t)) {
            LOG_WRN("Input speed ignored invalid setting size for %s", dev->name);
            atomic_set(&data->selected_level, config->default_level);
            return 0;
        }

        uint8_t level;
        int err = read_cb(cb_arg, &level, sizeof(level));
        if (err < 0) {
            return err;
        }
        if (err != (int)sizeof(level)) {
            LOG_WRN("Input speed ignored incomplete setting for %s", dev->name);
            atomic_set(&data->selected_level, config->default_level);
            return 0;
        }

        if (level == 0U || level > config->level_count) {
            LOG_WRN("Input speed ignored invalid stored level %u for %s", level, dev->name);
            atomic_set(&data->selected_level, config->default_level);
            return 0;
        }

        atomic_set(&data->selected_level, level);
        LOG_INF("Input speed restored level %u/%u (%u%%)", level, config->level_count,
                config->percentages[level - 1U]);
        return 0;
    }

    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(input_speed, "input_speed", NULL, speed_profile_settings_set, NULL,
                               NULL);
#endif

static const struct zmk_input_processor_speed_profile_driver_api speed_profile_driver_api = {
    .input_processor =
        {
            .handle_event = speed_profile_handle_event,
        },
    .get_level = speed_profile_get_level,
    .get_level_count = speed_profile_get_level_count,
    .set_level = speed_profile_set_level,
};

#define SPEED_PERCENTAGE_ASSERT(node_id, prop, idx)                                                \
    BUILD_ASSERT(DT_PROP_BY_IDX(node_id, prop, idx) > 0,                                           \
                 "speed-percentages values must be greater than zero");                            \
    BUILD_ASSERT(DT_PROP_BY_IDX(node_id, prop, idx) <= UINT16_MAX,                                 \
                 "speed-percentages values must fit in 16 bits");

#define SPEED_PERCENTAGE_INIT(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx),

#define SPEED_PROFILE_DEFINE(inst)                                                                 \
    BUILD_ASSERT(DT_INST_PROP_LEN(inst, codes) > 0, "codes must contain at least one input code"); \
    BUILD_ASSERT(DT_INST_PROP_LEN(inst, speed_percentages) > 0,                                    \
                 "speed-percentages must contain at least one level");                             \
    BUILD_ASSERT(DT_INST_PROP_LEN(inst, speed_percentages) <= UINT8_MAX,                           \
                 "speed-percentages must contain at most 255 levels");                             \
    BUILD_ASSERT(!DT_INST_PROP(inst, persist_speed_level) || IS_ENABLED(CONFIG_SETTINGS),          \
                 "persist-speed-level requires CONFIG_SETTINGS");                                  \
    BUILD_ASSERT(DT_INST_PROP(inst, default_speed_level) > 0,                                      \
                 "default-speed-level must be one or greater");                                    \
    BUILD_ASSERT(DT_INST_PROP(inst, default_speed_level) <=                                        \
                     DT_INST_PROP_LEN(inst, speed_percentages),                                    \
                 "default-speed-level must select an available speed percentage");                 \
    DT_INST_FOREACH_PROP_ELEM(inst, speed_percentages, SPEED_PERCENTAGE_ASSERT)                    \
    static const uint16_t speed_profile_codes_##inst[] = DT_INST_PROP(inst, codes);                \
    static const uint16_t speed_profile_percentages_##inst[] = {                                   \
        DT_INST_FOREACH_PROP_ELEM(inst, speed_percentages, SPEED_PERCENTAGE_INIT)};                \
    static const struct speed_profile_config speed_profile_config_##inst = {                       \
        .type = DT_INST_PROP(inst, type),                                                          \
        .codes_len = ARRAY_SIZE(speed_profile_codes_##inst),                                       \
        .codes = speed_profile_codes_##inst,                                                       \
        .level_count = ARRAY_SIZE(speed_profile_percentages_##inst),                               \
        .default_level = DT_INST_PROP(inst, default_speed_level),                                  \
        .percentages = speed_profile_percentages_##inst,                                           \
        .persist = DT_INST_PROP(inst, persist_speed_level),                                        \
    };                                                                                             \
    static struct speed_profile_data speed_profile_data_##inst;                                    \
    DEVICE_DT_INST_DEFINE(inst, speed_profile_init, NULL, &speed_profile_data_##inst,              \
                          &speed_profile_config_##inst, POST_KERNEL,                               \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &speed_profile_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPEED_PROFILE_DEFINE)
