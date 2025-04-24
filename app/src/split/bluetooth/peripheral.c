/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>

#if IS_ENABLED(CONFIG_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/ble.h>
#include <zmk/split/bluetooth/uuid.h>

#define MAX_BONDS CONFIG_ZMK_SPLIT_PERIPHERAL_MAX_BONDS
#define ADV_ROTATION_INTERVAL CONFIG_ZMK_SPLIT_PERIPHERAL_ADV_ROTATION_INTERVAL

static bt_addr_le_t bonded_centrals[MAX_BONDS];
static int bond_count = 0;
static int current_bond_index = 0;
static bool is_connected = false;
static bool is_bonded = false;
static bool rotating = true;

static struct k_work_delayable advertising_work;

static const struct bt_data zmk_ble_ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_SOME, 0x0f, 0x18),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, ZMK_SPLIT_BT_SERVICE_UUID)};

static void collect_bonded(const struct bt_bond_info *info, void *user_data) {
    if (bond_count < MAX_BONDS) {
        bt_addr_le_copy(&bonded_centrals[bond_count], &info->addr);
        bond_count++;
    }
}

static int start_advertising_to(bt_addr_le_t *addr, bool low_duty) {
    struct bt_le_adv_param adv_param =
        low_duty ? *BT_LE_ADV_CONN_DIR_LOW_DUTY(addr) : *BT_LE_ADV_CONN_DIR(addr);
    return bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);
}

static void advertising_cb(struct k_work *work) {
    if (is_connected || !rotating || bond_count == 0)
        return;

    int err = start_advertising_to(&bonded_centrals[current_bond_index], false);

    if (err < 0) {
        LOG_ERR("Failed advertising to bonded central %d (err: %d)", current_bond_index, err);
    } else {
        LOG_DBG("Advertising to bonded central %d", current_bond_index);
    }

    current_bond_index = (current_bond_index + 1) % bond_count;
    k_work_schedule(&advertising_work, K_SECONDS(ADV_ROTATION_INTERVAL));
}

K_WORK_DELAYABLE_DEFINE(advertising_work, advertising_cb);

static void connected(struct bt_conn *conn, uint8_t err) {
    is_connected = (err == 0);
    rotating = false;

    raise_zmk_split_peripheral_status_changed(
        (struct zmk_split_peripheral_status_changed){.connected = is_connected});

    if (err == BT_HCI_ERR_ADV_TIMEOUT) {
        k_work_submit(&advertising_work.work);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    is_connected = false;
    rotating = true;

    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Disconnected from %s (reason 0x%02x)", addr, reason);

    raise_zmk_split_peripheral_status_changed(
        (struct zmk_split_peripheral_status_changed){.connected = is_connected});

    k_work_schedule(&advertising_work, K_NO_WAIT);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    if (!err) {
        LOG_DBG("Security changed: %s level %u", addr, level);
    } else {
        LOG_ERR("Security failed: %s level %u err %d", addr, level, err);
    }
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
                             uint16_t timeout) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("%s: interval %d latency %d timeout %d", addr, interval, latency, timeout);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
    .le_param_updated = le_param_updated,
};

static void auth_pairing_complete(struct bt_conn *conn, bool bonded) { is_bonded = bonded; }

static struct bt_conn_auth_info_cb zmk_peripheral_ble_auth_info_cb = {
    .pairing_complete = auth_pairing_complete,
};

bool zmk_split_bt_peripheral_is_connected(void) { return is_connected; }

bool zmk_split_bt_peripheral_is_bonded(void) { return is_bonded; }

static int zmk_peripheral_ble_complete_startup(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE_CLEAR_BONDS_ON_START)
    LOG_WRN("Clearing all existing BLE bond information from the keyboard");
    bt_unpair(BT_ID_DEFAULT, NULL);
#else
    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_info_cb_register(&zmk_peripheral_ble_auth_info_cb);

    bond_count = 0;
    bt_foreach_bond(BT_ID_DEFAULT, collect_bonded, NULL);

    if (bond_count > 0) {
        LOG_INF("Found %d bonded centrals, starting advertising rotation", bond_count);
        rotating = true;
        k_work_schedule(&advertising_work, K_NO_WAIT);
    } else {
        LOG_INF("No bonded centrals found, starting generic advertising");
        bt_le_adv_start(BT_LE_ADV_CONN, zmk_ble_ad, ARRAY_SIZE(zmk_ble_ad), NULL, 0);
    }
#endif
    return 0;
}

#if IS_ENABLED(CONFIG_SETTINGS)
static int peripheral_ble_handle_set(const char *name, size_t len, settings_read_cb read_cb,
                                     void *cb_arg) {
    return 0;
}

static struct settings_handler ble_peripheral_settings_handler = {
    .name = "ble_peripheral",
    .h_set = peripheral_ble_handle_set,
    .h_commit = zmk_peripheral_ble_complete_startup};
#endif

static int zmk_peripheral_ble_init(void) {
    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("BLUETOOTH FAILED (%d)", err);
        return err;
    }

#if IS_ENABLED(CONFIG_SETTINGS)
    settings_register(&ble_peripheral_settings_handler);
#else
    zmk_peripheral_ble_complete_startup();
#endif

    return 0;
}

SYS_INIT(zmk_peripheral_ble_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
