/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/sensor.h>
#include <zephyr/types.h>
#include <sys/util.h>
#include <init.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#include <zmk/matrix.h>
#include <zmk/split/bluetooth/uuid.h>
#include <zmk/split/bluetooth/service.h>
#include <zmk/sensors.h>

#if ZMK_KEYMAP_HAS_SENSORS
struct sensor_event {
    uint8_t sensor_number;
    struct sensor_value value;
} sensor_event;

static ssize_t split_svc_sensor_state(struct bt_conn *conn, const struct bt_gatt_attr *attrs,
                                      void *buf, uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attrs, buf, len, offset, &sensor_event, sizeof(sensor_event));
}

static void split_svc_sensor_state_ccc(const struct bt_gatt_attr *attr, uint16_t value) {
    LOG_DBG("value %d", value);
}
#endif /* ZMK_KEYMAP_HAS_SENSORS */

#define POS_STATE_LEN 16

static uint8_t num_of_positions = ZMK_KEYMAP_LEN;
static uint8_t position_state[POS_STATE_LEN];

static ssize_t split_svc_pos_state(struct bt_conn *conn, const struct bt_gatt_attr *attrs,
                                   void *buf, uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attrs, buf, len, offset, &position_state,
                             sizeof(position_state));
}

static ssize_t split_svc_num_of_positions(struct bt_conn *conn, const struct bt_gatt_attr *attrs,
                                          void *buf, uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attrs, buf, len, offset, attrs->user_data, sizeof(uint8_t));
}

static void split_svc_pos_state_ccc(const struct bt_gatt_attr *attr, uint16_t value) {
    LOG_DBG("value %d", value);
}

BT_GATT_SERVICE_DEFINE(
    split_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID)),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_POSITION_STATE_UUID),
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_ENCRYPT,
                           split_svc_pos_state, NULL, &position_state),
    BT_GATT_CCC(split_svc_pos_state_ccc, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_DESCRIPTOR(BT_UUID_NUM_OF_DIGITALS, BT_GATT_PERM_READ, split_svc_num_of_positions, NULL,
                       &num_of_positions),
#if ZMK_KEYMAP_HAS_SENSORS
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_SENSOR_STATE_UUID),
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_ENCRYPT,
                           split_svc_sensor_state, NULL, &sensor_event),
    BT_GATT_CCC(split_svc_sensor_state_ccc, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
#endif /* ZMK_KEYMAP_HAS_SENSORS */
);

K_THREAD_STACK_DEFINE(service_q_stack, CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_STACK_SIZE);

struct k_work_q service_work_q;

K_MSGQ_DEFINE(position_state_msgq, sizeof(char[POS_STATE_LEN]),
              CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_POSITION_QUEUE_SIZE, 4);

void send_position_state_callback(struct k_work *work) {
    uint8_t state[POS_STATE_LEN];

    while (k_msgq_get(&position_state_msgq, &state, K_NO_WAIT) == 0) {
        int err = bt_gatt_notify(NULL, &split_svc.attrs[1], &state, sizeof(state));
        if (err) {
            LOG_DBG("Error notifying %d", err);
        }
    }
};

K_WORK_DEFINE(service_position_notify_work, send_position_state_callback);

int send_position_state() {
    int err = k_msgq_put(&position_state_msgq, position_state, K_MSEC(100));
    if (err) {
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Position state message queue full, popping first message and queueing again");
            uint8_t discarded_state[POS_STATE_LEN];
            k_msgq_get(&position_state_msgq, &discarded_state, K_NO_WAIT);
            return send_position_state();
        }
        default:
            LOG_WRN("Failed to queue position state to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&service_work_q, &service_position_notify_work);

    return 0;
}

int zmk_split_bt_position_pressed(uint8_t position) {
    WRITE_BIT(position_state[position / 8], position % 8, true);
    return send_position_state();
}

int zmk_split_bt_position_released(uint8_t position) {
    WRITE_BIT(position_state[position / 8], position % 8, false);
    return send_position_state();
}

#if ZMK_KEYMAP_HAS_SENSORS
K_MSGQ_DEFINE(sensor_state_msgq, sizeof(sensor_event),
              CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_POSITION_QUEUE_SIZE, 4);

void send_sensor_state_callback(struct k_work *work) {
    struct sensor_event ev;

    while (k_msgq_get(&sensor_state_msgq, &ev, K_NO_WAIT) == 0) {
        int err = bt_gatt_notify(NULL, &split_svc.attrs[5], &ev, sizeof(ev));
        if (err) {
            LOG_DBG("Error notifying %d", err);
        }
    }
};

K_WORK_DEFINE(service_sensor_notify_work, send_sensor_state_callback);

int send_sensor_state() {
    int err = k_msgq_put(&sensor_state_msgq, &sensor_event, K_MSEC(100));
    if (err) {
        // retry...
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Sensor state message queue full, popping first message and queueing again");
            struct sensor_event discarded_state;
            k_msgq_get(&sensor_state_msgq, &discarded_state, K_NO_WAIT);
            return send_sensor_state();
        }
        default:
            LOG_WRN("Failed to queue sensor state to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&service_work_q, &service_sensor_notify_work);
    return 0;
}

int zmk_split_bt_sensor_triggered(uint8_t sensor_number, struct sensor_value value) {
    sensor_event.sensor_number = sensor_number;
    sensor_event.value = value;
    return send_sensor_state();
}
#endif /* ZMK_KEYMAP_HAS_SENSORS */

int service_init(const struct device *_arg) {
    k_work_q_start(&service_work_q, service_q_stack, K_THREAD_STACK_SIZEOF(service_q_stack),
                   CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_PRIORITY);

    return 0;
}

SYS_INIT(service_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
