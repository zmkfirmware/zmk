/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/sensor.h>
#include <devicetree.h>
#include <init.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/sensors.h>
#include <zmk/event_manager.h>
#include <zmk/events/sensor_event.h>

#if ZMK_KEYMAP_HAS_SENSORS

struct sensors_data_item {
    uint8_t sensor_number;
    const struct device *dev;
    struct sensor_trigger trigger;
};

#define _SENSOR_ITEM(node)                                                                         \
    {.dev = NULL, .trigger = {.type = SENSOR_TRIG_DELTA, .chan = SENSOR_CHAN_ROTATION}},
#define SENSOR_ITEM(idx, _)                                                                        \
    COND_CODE_1(DT_NODE_HAS_STATUS(ZMK_KEYMAP_SENSORS_BY_IDX(idx), okay),                          \
                (_SENSOR_ITEM(ZMK_KEYMAP_SENSORS_BY_IDX(idx))), ())

static struct sensors_data_item sensors[] = {UTIL_LISTIFY(ZMK_KEYMAP_SENSORS_LEN, SENSOR_ITEM, 0)};

static void zmk_sensors_trigger_handler(const struct device *dev, struct sensor_trigger *trigger) {
    int err;
    struct sensors_data_item *item = CONTAINER_OF(trigger, struct sensors_data_item, trigger);

    LOG_DBG("sensor %d", item->sensor_number);

    err = sensor_sample_fetch(dev);
    if (err) {
        LOG_WRN("Failed to fetch sample from device %d", err);
        return;
    }

    struct sensor_value value;
    err = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &value);
    if (err) {
        LOG_WRN("Failed to get sensor rotation value: %d", err);
        return;
    }

    ZMK_EVENT_RAISE(new_zmk_sensor_event((struct zmk_sensor_event){
        .sensor_number = item->sensor_number, .value = value, .timestamp = k_uptime_get()}));
}

static void zmk_sensors_init_item(const char *node, uint8_t i, uint8_t abs_i) {
    LOG_DBG("Init %s at index %d with sensor_number %d", node, i, abs_i);

    sensors[i].dev = device_get_binding(node);
    sensors[i].sensor_number = abs_i;

    if (!sensors[i].dev) {
        LOG_WRN("Failed to find device for %s", node);
        return;
    }

    sensor_trigger_set(sensors[i].dev, &sensors[i].trigger, zmk_sensors_trigger_handler);
}

#define _SENSOR_INIT(node) zmk_sensors_init_item(DT_LABEL(node), local_index++, absolute_index++);
#define SENSOR_INIT(idx, _i)                                                                       \
    COND_CODE_1(DT_NODE_HAS_STATUS(ZMK_KEYMAP_SENSORS_BY_IDX(idx), okay),                          \
                (_SENSOR_INIT(ZMK_KEYMAP_SENSORS_BY_IDX(idx))), (absolute_index++;))

static int zmk_sensors_init(const struct device *_arg) {
    int local_index = 0;
    int absolute_index = 0;

    UTIL_LISTIFY(ZMK_KEYMAP_SENSORS_LEN, SENSOR_INIT, 0)
    return 0;
}

SYS_INIT(zmk_sensors_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* ZMK_KEYMAP_HAS_SENSORS */