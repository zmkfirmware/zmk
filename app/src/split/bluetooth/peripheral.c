/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <init.h>

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <settings/settings.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci_err.h>

#if IS_ENABLED(CONFIG_SETTINGS)

#include <settings/settings.h>

#endif

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/ble.h>
#include <zmk/split/bluetooth/uuid.h>

static const struct bt_data zmk_ble_ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_SOME, 0x0f, 0x18 /* Battery Service */
                  ),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, ZMK_SPLIT_BT_SERVICE_UUID)};

static bool is_connected = false;

static int start_advertising() {
    return bt_le_adv_start(BT_LE_ADV_CONN, zmk_ble_ad, ARRAY_SIZE(zmk_ble_ad), NULL, 0);
};

static void connected(struct bt_conn *conn, uint8_t err) {
    is_connected = (err == 0);

    ZMK_EVENT_RAISE(new_zmk_split_peripheral_status_changed(
        (struct zmk_split_peripheral_status_changed){.connected = is_connected}));
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Disconnected from %s (reason 0x%02x)", log_strdup(addr), reason);

    is_connected = false;

    ZMK_EVENT_RAISE(new_zmk_split_peripheral_status_changed(
        (struct zmk_split_peripheral_status_changed){.connected = is_connected}));
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        LOG_DBG("Security changed: %s level %u", log_strdup(addr), level);
    } else {
        LOG_ERR("Security failed: %s level %u err %d", log_strdup(addr), level, err);
    }
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
                             uint16_t timeout) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("%s: interval %d latency %d timeout %d", log_strdup(addr), interval, latency, timeout);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
    .le_param_updated = le_param_updated,
};

bool zmk_split_bt_peripheral_is_connected() { return is_connected; }

static int zmk_peripheral_ble_init(const struct device *_arg) {
    int err = bt_enable(NULL);

    if (err) {
        LOG_ERR("BLUETOOTH FAILED (%d)", err);
        return err;
    }

#if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();

    settings_load_subtree("ble");
    settings_load_subtree("bt");
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE_CLEAR_BONDS_ON_START)
    LOG_WRN("Clearing all existing BLE bond information from the keyboard");

    bt_unpair(BT_ID_DEFAULT, NULL);
#endif

    bt_conn_cb_register(&conn_callbacks);

    start_advertising();

    return 0;
}

SYS_INIT(zmk_peripheral_ble_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
