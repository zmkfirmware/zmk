/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <init.h>

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

#include <zmk/keys.h>
#include <zmk/split/bluetooth/uuid.h>

static struct bt_conn *auth_passkey_entry_conn;
static u8_t passkey_entries[6] = {0, 0, 0, 0, 0, 0};
static u8_t passkey_digit = 0;

#define ZMK_BT_LE_ADV_PARAM_INIT(_id, _options, _int_min, _int_max, _peer) \
{ \
        .id = _id, \
        .sid = 0, \
        .secondary_max_skip = 0, \
        .options = (_options), \
        .interval_min = (_int_min), \
        .interval_max = (_int_max), \
        .peer = (_peer), \
}

#define ZMK_BT_LE_ADV_PARAM(_id, _options, _int_min, _int_max, _peer) \
        ((struct bt_le_adv_param[]) { \
                ZMK_BT_LE_ADV_PARAM_INIT(_id, _options, _int_min, _int_max, _peer) \
        })

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL)
#define ZMK_ADV_PARAMS(_id) ZMK_BT_LE_ADV_PARAM(_id, \
                                        BT_LE_ADV_OPT_CONNECTABLE | \
                                        BT_LE_ADV_OPT_ONE_TIME | \
                                        BT_LE_ADV_OPT_USE_NAME, \
                                        BT_GAP_ADV_FAST_INT_MIN_2, \
                                        BT_GAP_ADV_FAST_INT_MAX_2, NULL)
#else
#define ZMK_ADV_PARAMS(_id) ZMK_BT_LE_ADV_PARAM(_id, \
                                        BT_LE_ADV_OPT_CONNECTABLE | \
                                        BT_LE_ADV_OPT_USE_NAME, \
                                        BT_GAP_ADV_FAST_INT_MIN_2, \
                                        BT_GAP_ADV_FAST_INT_MAX_2, NULL)
#endif

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

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL)

static bt_addr_le_t peripheral_addr;

#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL) */


int zmk_ble_adv_pause()
{
    int err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Failed to stop advertising (err %d)", err);
        return err;
    }

    return 0;
};

int zmk_ble_adv_resume()
{
    struct bt_le_adv_param *adv_params = ZMK_ADV_PARAMS(BT_ID_DEFAULT);

    LOG_DBG("");
    int err = bt_le_adv_start(adv_params, zmk_ble_ad, ARRAY_SIZE(zmk_ble_ad), NULL, 0);
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    return 0;
};

static void disconnect_host_connection(struct bt_conn *conn, void *arg)
{
    struct bt_conn_info info;
    bt_conn_get_info(conn, &info);

    if (info.role != BT_CONN_ROLE_SLAVE) {
        return;
    }

    bt_conn_disconnect(conn, BT_HCI_ERR_LOCALHOST_TERM_CONN);
};


static void unpair_non_peripheral_bonds(const struct bt_bond_info *info, void *user_data) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(&info->addr, addr, sizeof(addr));

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL)
    if (!bt_addr_le_cmp(&info->addr, &peripheral_addr)) {
        LOG_DBG("Skipping unpairing peripheral %s", log_strdup(addr));
        return;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL) */

    LOG_DBG("Unpairing %s", log_strdup(addr));
    bt_unpair(BT_ID_DEFAULT, &info->addr);
}

int zmk_ble_clear_bonds()
{
    LOG_DBG("");
    
    bt_conn_foreach(BT_ID_DEFAULT, disconnect_host_connection, NULL);
    bt_foreach_bond(BT_ID_DEFAULT, unpair_non_peripheral_bonds, NULL);

    return 0;
};

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL)

void zmk_ble_set_peripheral_addr(bt_addr_le_t *addr)
{
    memcpy(&peripheral_addr, addr, sizeof(bt_addr_le_t));
    settings_save_one("ble/peripheral_address", addr, sizeof(bt_addr_le_t));
}

#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL) */

#if IS_ENABLED(CONFIG_SETTINGS)

static int ble_profiles_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    const char *next;

    LOG_DBG("Setting BLE value %s", log_strdup(name));

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL)
    if (settings_name_steq(name, "peripheral_address", &next) && !next) {
        if (len != sizeof(bt_addr_le_t)) {
            return -EINVAL;
        }

        int err = read_cb(cb_arg, &peripheral_addr, sizeof(bt_addr_le_t));
        if (err <= 0) {
            LOG_ERR("Failed to handle peripheral address from settings (err %d)", err);
            return err;
        }
    }
#endif

    return 0;
};

struct settings_handler profiles_handler = {
    .name = "ble",
    .h_set = ble_profiles_handle_set
};
#endif /* IS_ENABLED(CONFIG_SETTINGS) */

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

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL)
    if (bt_addr_le_cmp(&peripheral_addr, BT_ADDR_LE_ANY) && bt_addr_le_cmp(&peripheral_addr, bt_conn_get_dst(conn))) {
        zmk_ble_adv_resume();
    }
#else 
    zmk_ble_adv_resume();
#endif
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


static void zmk_ble_ready(int err)
{
    LOG_DBG("ready? %d", err);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    zmk_ble_adv_resume();
}

static int zmk_ble_init(struct device *_arg)
{
    int err = bt_enable(NULL);

    if (err)
    {
        LOG_ERR("BLUETOOTH FAILED (%d)", err);
        return err;
    }

#if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();

    err = settings_register(&profiles_handler);
    if (err) {
        LOG_ERR("Failed to setup the profile settings handler (err %d)", err);
        return err;
    }

    settings_load();

#endif

#if IS_ENABLED(CONFIG_ZMK_BLE_CLEAR_BONDS_ON_START)
    LOG_WRN("Clearing all existing BLE bond information from the keyboard");
    settings_delete("bt/name");

    for (int i = 0; i < 10; i++) {
        bt_unpair(i, NULL);
    }
#endif

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&zmk_ble_auth_cb_display);

    zmk_ble_ready(0);

    return 0;
}

int zmk_ble_unpair_all()
{
    int resp = 0;
    for (int i = BT_ID_DEFAULT; i < CONFIG_BT_ID_MAX; i++) {

        int err = bt_unpair(BT_ID_DEFAULT, NULL);
        if (err) {
            resp = err;
            LOG_ERR("Failed to unpair devices (err %d)", err);
        }
    }

    return resp;
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
