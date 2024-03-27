/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/init.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/matrix.h>
#include <zmk/split/bluetooth/uuid.h>
#include <zmk/split/service.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
#include <zmk/events/hid_indicators_changed.h>
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

#include <zmk/events/sensor_event.h>
#include <zmk/sensors.h>

#if ZMK_KEYMAP_HAS_SENSORS
static struct sensor_event last_sensor_event;

static ssize_t split_svc_sensor_state(struct bt_conn *conn, const struct bt_gatt_attr *attrs,
                                      void *buf, uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attrs, buf, len, offset, &last_sensor_event,
                             sizeof(last_sensor_event));
}

static void split_svc_sensor_state_ccc(const struct bt_gatt_attr *attr, uint16_t value) {
    LOG_DBG("value %d", value);
}
#endif /* ZMK_KEYMAP_HAS_SENSORS */

static uint8_t num_of_positions = ZMK_KEYMAP_LEN;
static uint8_t position_state[ZMK_SPLIT_POS_STATE_LEN];

static struct zmk_split_run_behavior_payload behavior_run_payload;

static ssize_t split_svc_pos_state(struct bt_conn *conn, const struct bt_gatt_attr *attrs,
                                   void *buf, uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attrs, buf, len, offset, &position_state,
                             sizeof(position_state));
}

static ssize_t split_svc_run_behavior(struct bt_conn *conn, const struct bt_gatt_attr *attrs,
                                      const void *buf, uint16_t len, uint16_t offset,
                                      uint8_t flags) {
    struct zmk_split_run_behavior_payload *payload = attrs->user_data;
    uint16_t end_addr = offset + len;

    LOG_DBG("offset %d len %d", offset, len);

    if (end_addr > sizeof(struct zmk_split_run_behavior_payload)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(payload + offset, buf, len);

    // We run if:
    // 1: We've gotten all the position/state/param data.
    // 2: We have a null terminated string for the behavior device label.
    const size_t behavior_dev_offset =
        offsetof(struct zmk_split_run_behavior_payload, behavior_dev);
    if ((end_addr > sizeof(struct zmk_split_run_behavior_data)) &&
        payload->behavior_dev[end_addr - behavior_dev_offset - 1] == '\0') {
        struct zmk_behavior_binding binding = {
            .param1 = payload->data.param1,
            .param2 = payload->data.param2,
            .behavior_dev = payload->behavior_dev,
        };
        LOG_DBG("%s with params %d %d: pressed? %d", binding.behavior_dev, binding.param1,
                binding.param2, payload->data.state);
        struct zmk_behavior_binding_event event = {.position = payload->data.position,
                                                   .timestamp = k_uptime_get()};
        int err;
        if (payload->data.state > 0) {
            err = behavior_keymap_binding_pressed(&binding, event);
        } else {
            err = behavior_keymap_binding_released(&binding, event);
        }

        if (err) {
            LOG_ERR("Failed to invoke behavior %s: %d", binding.behavior_dev, err);
        }
    }

    return len;
}

static ssize_t split_svc_num_of_positions(struct bt_conn *conn, const struct bt_gatt_attr *attrs,
                                          void *buf, uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attrs, buf, len, offset, attrs->user_data, sizeof(uint8_t));
}

static void split_svc_pos_state_ccc(const struct bt_gatt_attr *attr, uint16_t value) {
    LOG_DBG("value %d", value);
}

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

static zmk_hid_indicators_t hid_indicators = 0;

static void split_svc_update_indicators_callback(struct k_work *work) {
    LOG_DBG("Raising HID indicators changed event: %x", hid_indicators);
    raise_zmk_hid_indicators_changed(
        (struct zmk_hid_indicators_changed){.indicators = hid_indicators});
}

static K_WORK_DEFINE(split_svc_update_indicators_work, split_svc_update_indicators_callback);

static ssize_t split_svc_update_indicators(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                           const void *buf, uint16_t len, uint16_t offset,
                                           uint8_t flags) {
    if (offset + len > sizeof(zmk_hid_indicators_t)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy((uint8_t *)&hid_indicators + offset, buf, len);

    k_work_submit(&split_svc_update_indicators_work);

    return len;
}

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

BT_GATT_SERVICE_DEFINE(
    split_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID)),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_POSITION_STATE_UUID),
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_ENCRYPT,
                           split_svc_pos_state, NULL, &position_state),
    BT_GATT_CCC(split_svc_pos_state_ccc, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_RUN_BEHAVIOR_UUID),
                           BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_WRITE_ENCRYPT, NULL,
                           split_svc_run_behavior, &behavior_run_payload),
    BT_GATT_DESCRIPTOR(BT_UUID_NUM_OF_DIGITALS, BT_GATT_PERM_READ, split_svc_num_of_positions, NULL,
                       &num_of_positions),
#if ZMK_KEYMAP_HAS_SENSORS
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_SENSOR_STATE_UUID),
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_ENCRYPT,
                           split_svc_sensor_state, NULL, &last_sensor_event),
    BT_GATT_CCC(split_svc_sensor_state_ccc, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
#endif /* ZMK_KEYMAP_HAS_SENSORS */
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(ZMK_SPLIT_BT_UPDATE_HID_INDICATORS_UUID),
                           BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_WRITE_ENCRYPT, NULL,
                           split_svc_update_indicators, NULL),
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
);

void send_position_state_impl(uint8_t *state, int len) {
    memcpy(position_state, state, MIN(len, sizeof(position_state)));
    int err = bt_gatt_notify(NULL, &split_svc.attrs[1], state, len);
    if (err) {
        LOG_DBG("Error notifying %d", err);
    }
}

#if ZMK_KEYMAP_HAS_SENSORS
void send_sensor_state_impl(struct sensor_event *event, int len) {
    memcpy(&last_sensor_event, event, MIN(len, sizeof(last_sensor_event)));
    int err = bt_gatt_notify(NULL, &split_svc.attrs[8], event, len);
    if (err) {
        LOG_DBG("Error notifying %d", err);
    }
}
#endif /* ZMK_KEYMAP_HAS_SENSORS */
