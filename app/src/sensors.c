/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/sensor.h>
#include <devicetree.h>
#include <init.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/sensors.h>
#include <zmk/event-manager.h>
#include <zmk/events/sensor-event.h>

struct sensors_data_item {
    u8_t sensor_number;
    struct device *dev;
    struct sensor_trigger trigger;
};

#define _SENSOR_ITEM(node) {.dev = NULL, .trigger = { .type = SENSOR_TRIG_DELTA, .chan = SENSOR_CHAN_ROTATION } },
#define SENSOR_ITEM(node, _) COND_CODE_1(DT_NODE_HAS_STATUS(node,okay), (_SENSOR_ITEM(node)),())

static struct sensors_data_item sensors[] = {
    UTIL_LISTIFY(ZMK_KEYMAP_SENSORS_LEN, SENSOR_ITEM, 0)
};

static void zmk_sensors_trigger_handler(struct device *dev, struct sensor_trigger *trigger)
{
    int err;
    struct sensor_value val;
    struct sensors_data_item * item = CONTAINER_OF(trigger, struct sensors_data_item, trigger);
    struct sensor_event *event;
    
    LOG_DBG("sensor %d", item->sensor_number);

    err = sensor_sample_fetch(dev);
    if (err) {
        LOG_WRN("Failed to fetch sample from device %d", err);
        return;
    }

    event = new_sensor_event();
    event->sensor_number = item->sensor_number;

    err = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &event->value);
    if (err) {
        k_free(event);
        LOG_WRN("Failed to get the value for the rotation channel: %d", err);
    }

    LOG_DBG("val1 %d val2 %d", val.val1, val.val2);

    ZMK_EVENT_RAISE(event);
}

static void zmk_sensors_init_item(const char *node, u8_t i, u8_t abs_i)
{
    LOG_DBG("Init %s at index %d with sensor_number %d", node, i, abs_i);

    sensors[i].dev = device_get_binding(node);
    sensors[i].sensor_number = abs_i;

    if (!sensors[i].dev) {
        LOG_ERR("Failed to find device for %s", node);
    }

    sensor_trigger_set(sensors[i].dev, &sensors[i].trigger, zmk_sensors_trigger_handler);
}

#define _SENSOR_INIT(node) zmk_sensors_init_item(DT_LABEL(node), local_index++, absolute_index++);
#define SENSOR_INIT(node, _i) COND_CODE_1(DT_NODE_HAS_STATUS(node,okay), (_SENSOR_INIT(node)),(absolute_index++;))

static int zmk_sensors_init(struct device *_arg)
{
    int local_index = 0;
    int absolute_index = 0;

    UTIL_LISTIFY(ZMK_KEYMAP_SENSORS_LEN, SENSOR_INIT, 0)
    return 0;
}

SYS_INIT(zmk_sensors_init,
        APPLICATION,
        CONFIG_APPLICATION_INIT_PRIORITY);