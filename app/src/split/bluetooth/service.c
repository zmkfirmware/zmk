/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/types.h>
#include <sys/util.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#include <zmk/matrix.h>
#include <zmk/split/bluetooth/uuid.h>
#include <zmk/split/bluetooth/service.h>

static uint8_t num_of_positions = ZMK_KEYMAP_LEN;
static uint8_t position_state[16];

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
                       &num_of_positions), );

int zmk_split_bt_position_pressed(uint8_t position) {
    WRITE_BIT(position_state[position / 8], position % 8, true);
    return bt_gatt_notify(NULL, &split_svc.attrs[1], &position_state, sizeof(position_state));
}

int zmk_split_bt_position_released(uint8_t position) {
    WRITE_BIT(position_state[position / 8], position % 8, false);
    return bt_gatt_notify(NULL, &split_svc.attrs[1], &position_state, sizeof(position_state));
}