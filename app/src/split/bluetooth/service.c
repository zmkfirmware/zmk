/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/types.h>
#include <sys/util.h>
#include <init.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/matrix.h>
#include <zmk/split/bluetooth/uuid.h>
#include <zmk/split/bluetooth/service.h>

#define POS_STATE_LEN 16

static uint8_t num_of_positions = ZMK_KEYMAP_LEN;
static uint8_t position_state[POS_STATE_LEN];

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
        LOG_DBG("%s with params %d %d: pressed? %d", log_strdup(binding.behavior_dev),
                binding.param1, binding.param2, payload->data.state);
        struct zmk_behavior_binding_event event = {.position = payload->data.position,
                                                   .timestamp = k_uptime_get()};
        int err;
        if (payload->data.state > 0) {
            err = behavior_keymap_binding_pressed(&binding, event);
        } else {
            err = behavior_keymap_binding_released(&binding, event);
        }

        if (err) {
            LOG_ERR("Failed to invoke behavior %s: %d", log_strdup(binding.behavior_dev), err);
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
                       &num_of_positions), );

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

int service_init(const struct device *_arg) {
    k_work_q_start(&service_work_q, service_q_stack, K_THREAD_STACK_SIZEOF(service_q_stack),
                   CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_PRIORITY);

    return 0;
}

SYS_INIT(service_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
