/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/sensors.h>
#include <zmk/event_manager.h>
#include <zmk/events/sensor_event.h>

#if ZMK_KEYMAP_HAS_SENSORS

struct sensors_item_cfg {
    const struct zmk_sensor_config *config;
    const struct device *dev;
    struct sensor_trigger trigger;
    uint8_t sensor_index;
};

#define _SENSOR_ITEM(idx, node)                                                                    \
    {                                                                                              \
        .dev = DEVICE_DT_GET_OR_NULL(node),                                                        \
        .trigger = {.type = SENSOR_TRIG_DATA_READY, .chan = SENSOR_CHAN_ROTATION},                 \
        .config = &configs[idx], .sensor_index = idx                                               \
    }
#define SENSOR_ITEM(idx, _i) _SENSOR_ITEM(idx, ZMK_KEYMAP_SENSORS_BY_IDX(idx))

#define PLUS_ONE(n) +1
#define ZMK_KEYMAP_SENSORS_CHILD_COUNT (0 DT_FOREACH_CHILD(ZMK_KEYMAP_SENSORS_NODE, PLUS_ONE))
#define SENSOR_CHILD_ITEM(node)                                                                    \
    {                                                                                              \
        .triggers_per_rotation =                                                                   \
            DT_PROP_OR(node, triggers_per_rotation,                                                \
                       DT_PROP_OR(ZMK_KEYMAP_SENSORS_NODE, triggers_per_rotation,                  \
                                  CONFIG_ZMK_KEYMAP_SENSORS_DEFAULT_TRIGGERS_PER_ROTATION))        \
    }
#define SENSOR_CHILD_DEFAULTS(idx, arg)                                                            \
    {                                                                                              \
        .triggers_per_rotation =                                                                   \
            DT_PROP_OR(ZMK_KEYMAP_SENSORS_NODE, triggers_per_rotation,                             \
                       CONFIG_ZMK_KEYMAP_SENSORS_DEFAULT_TRIGGERS_PER_ROTATION)                    \
    }

static struct zmk_sensor_config configs[] = {
#if ZMK_KEYMAP_SENSORS_CHILD_COUNT > 0
    DT_FOREACH_CHILD_SEP(ZMK_KEYMAP_SENSORS_NODE, SENSOR_CHILD_ITEM, (, ))
#else
    LISTIFY(ZMK_KEYMAP_SENSORS_LEN, SENSOR_CHILD_DEFAULTS, (, ), 0)
#endif
};

static struct sensors_item_cfg sensors[] = {LISTIFY(ZMK_KEYMAP_SENSORS_LEN, SENSOR_ITEM, (, ), 0)};

static ATOMIC_DEFINE(pending_sensors, ZMK_KEYMAP_SENSORS_LEN);

const struct zmk_sensor_config *zmk_sensors_get_config_at_index(uint8_t sensor_index) {
    if (sensor_index > ARRAY_SIZE(configs)) {
        return NULL;
    }

    return &configs[sensor_index];
}

static void trigger_sensor_data_for_position(uint32_t sensor_index) {
    int err;
    const struct sensors_item_cfg *item = &sensors[sensor_index];

    err = sensor_sample_fetch(item->dev);
    if (err) {
        LOG_WRN("Failed to fetch sample from device %d", err);
        return;
    }

    struct sensor_value value;
    err = sensor_channel_get(item->dev, item->trigger.chan, &value);

    if (err) {
        LOG_WRN("Failed to get channel data from device %d", err);
        return;
    }

    raise_zmk_sensor_event(
        (struct zmk_sensor_event){.sensor_index = item->sensor_index,
                                  .channel_data_size = 1,
                                  .channel_data = {(struct zmk_sensor_channel_data){
                                      .value = value, .channel = item->trigger.chan}},
                                  .timestamp = k_uptime_get()});
}

static void run_sensors_data_trigger(struct k_work *work) {
    for (int i = 0; i < ARRAY_SIZE(sensors); i++) {
        if (atomic_test_and_clear_bit(pending_sensors, i)) {
            trigger_sensor_data_for_position(i);
        }
    }
}

K_WORK_DEFINE(sensor_data_work, run_sensors_data_trigger);

static void zmk_sensors_trigger_handler(const struct device *dev,
                                        const struct sensor_trigger *trigger) {
    const struct sensors_item_cfg *test_item =
        CONTAINER_OF(trigger, struct sensors_item_cfg, trigger);
    int sensor_index = test_item - sensors;

    if (sensor_index < 0 || sensor_index >= ARRAY_SIZE(sensors)) {
        LOG_ERR("Invalid sensor item triggered our callback (%d)", sensor_index);
        return;
    }

    if (k_is_in_isr()) {
        atomic_set_bit(pending_sensors, sensor_index);
        k_work_submit(&sensor_data_work);
    } else {
        trigger_sensor_data_for_position(sensor_index);
    }
}

static void zmk_sensors_init_item(uint8_t i) {
    LOG_DBG("Init sensor at index %d", i);

    if (!sensors[i].dev) {
        LOG_DBG("No local device for %d", i);
        return;
    }

    int err = sensor_trigger_set(sensors[i].dev, &sensors[i].trigger, zmk_sensors_trigger_handler);
    if (err) {
        LOG_WRN("Failed to set sensor trigger (%d)", err);
    }
}

#define SENSOR_INIT(idx, _t) zmk_sensors_init_item(idx);

static int zmk_sensors_init(void) {
    LISTIFY(ZMK_KEYMAP_SENSORS_LEN, SENSOR_INIT, (), 0)

    return 0;
}

SYS_INIT(zmk_sensors_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* ZMK_KEYMAP_HAS_SENSORS */
