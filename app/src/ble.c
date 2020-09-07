/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <init.h>

#include <math.h>

#include <settings/settings.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>


#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keys.h>
#include <zmk/split/bluetooth/uuid.h>

static struct bt_conn *auth_passkey_entry_conn;
static u8_t passkey_entries[6] = {0, 0, 0, 0, 0, 0};
static u8_t passkey_digit = 0;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL)
#define ZMK_ADV_PARAMS BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
                                        BT_LE_ADV_OPT_USE_NAME | \
                                        BT_LE_ADV_OPT_ONE_TIME, \
                                        BT_GAP_ADV_FAST_INT_MIN_2, \
                                        BT_GAP_ADV_FAST_INT_MAX_2, NULL)
#else
#define ZMK_ADV_PARAMS BT_LE_ADV_CONN_NAME
#endif

static void connected(struct bt_conn *conn, u8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err)
    {
        LOG_WRN("Failed to connect to %s (%u)", log_strdup(addr), err);
        return;
    }

    LOG_DBG("Connected %s", log_strdup(addr));

    bt_conn_le_param_update(conn, BT_LE_CONN_PARAM(0x0006, 0x000c, 30, 400));

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL)
    bt_conn_le_phy_update(conn, BT_CONN_LE_PHY_PARAM_2M);
#endif

    if (bt_conn_set_security(conn, BT_SECURITY_L2))
    {
        LOG_ERR("Failed to set security");
    }
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Disconnected from %s (reason 0x%02x)", log_strdup(addr), reason);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err)
    {
        LOG_DBG("Security changed: %s level %u", log_strdup(addr), level);
    }
    else
    {
        LOG_ERR("Security failed: %s level %u err %d", log_strdup(addr), level,
               err);
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Passkey for %s: %06u", log_strdup(addr), passkey);
}

#ifdef CONFIG_ZMK_BLE_PASSKEY_ENTRY

static void auth_passkey_entry(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Passkey entry requested for %s", log_strdup(addr));
    auth_passkey_entry_conn = bt_conn_ref(conn);
}

#endif

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (auth_passkey_entry_conn)
    {
        bt_conn_unref(auth_passkey_entry_conn);
        auth_passkey_entry_conn = NULL;
    }

    passkey_digit = 0;

    LOG_DBG("Pairing cancelled: %s", log_strdup(addr));
}

static struct bt_conn_auth_cb zmk_ble_auth_cb_display = {
// .passkey_display = auth_passkey_display,

#ifdef CONFIG_ZMK_BLE_PASSKEY_ENTRY
    .passkey_entry = auth_passkey_entry,
#endif
    .cancel = auth_cancel,
};

static const struct bt_data zmk_ble_ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_SOME,
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL)
                  0x12, 0x18,  /* HID Service */
#endif
                  0x0f, 0x18 /* Battery Service */
    ),
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL)
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                  ZMK_SPLIT_BT_SERVICE_UUID)
#endif
};

static void zmk_ble_ready(int err)
{
    LOG_DBG("ready? %d", err);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    err = bt_le_adv_start(ZMK_ADV_PARAMS, zmk_ble_ad, ARRAY_SIZE(zmk_ble_ad), NULL, 0);
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }
}

static int zmk_ble_init(struct device *_arg)
{
    int err = bt_enable(NULL);

    if (err)
    {
        LOG_ERR("BLUETOOTH FAILED (%d)", err);
        return err;
    }

    if (IS_ENABLED(CONFIG_BT_SETTINGS))
    {
        settings_load();
    }

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&zmk_ble_auth_cb_display);

    zmk_ble_ready(0);

    return 0;
}

int zmk_ble_unpair_all()
{
    LOG_DBG("");
    return bt_unpair(BT_ID_DEFAULT, NULL);
};

bool zmk_ble_handle_key_user(struct zmk_key_event *key_event)
{
    zmk_key key = key_event->key;

    if (!auth_passkey_entry_conn)
    {
        return true;
    }

    if (key < NUM_1 || key > NUM_0)
    {
        return true;
    }

    u32_t val = (key == NUM_0) ? 0 : (key - NUM_1 + 1);

    passkey_entries[passkey_digit++] = val;

    if (passkey_digit == 6)
    {
        u32_t passkey = 0;
        for (int i = 5; i >= 0; i--)
        {
            passkey = (passkey * 10) + val;
        }
        bt_conn_auth_passkey_entry(auth_passkey_entry_conn, passkey);
        bt_conn_unref(auth_passkey_entry_conn);
        auth_passkey_entry_conn = NULL;
    }

    return false;
}

SYS_INIT(zmk_ble_init,
        APPLICATION,
        CONFIG_ZMK_BLE_INIT_PRIORITY);
