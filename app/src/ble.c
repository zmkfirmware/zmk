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

#include <zmk/ble.h>
#include <zmk/keys.h>
#include <zmk/split/bluetooth/uuid.h>
#include <zmk/event-manager.h>
#include <zmk/events/ble-active-profile-changed.h>

static struct bt_conn *auth_passkey_entry_conn;
static u8_t passkey_entries[6] = {0, 0, 0, 0, 0, 0};
static u8_t passkey_digit = 0;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL)
#define PROFILE_COUNT (CONFIG_BT_MAX_PAIRED - 1)
#else
#define PROFILE_COUNT CONFIG_BT_MAX_PAIRED
#endif


static struct zmk_ble_profile profiles[PROFILE_COUNT];
static u8_t active_profile;

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

static void raise_profile_changed_event()
{
    struct ble_active_profile_changed *ev = new_ble_active_profile_changed();
    ev->index = active_profile;
    ev->profile = &profiles[active_profile];

    ZMK_EVENT_RAISE(ev);
}

static bool active_profile_is_open()
{
    return !bt_addr_le_cmp(&profiles[active_profile].peer, BT_ADDR_LE_ANY);
}

void set_profile_address(u8_t index, const bt_addr_le_t *addr)
{
    char setting_name[15];
    char addr_str[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    memcpy(&profiles[index].peer, addr, sizeof(bt_addr_le_t));
    sprintf(setting_name, "ble/profiles/%d", index);
    LOG_DBG("Setting profile addr for %s to %s", log_strdup(setting_name), log_strdup(addr_str));
    settings_save_one(setting_name, &profiles[index], sizeof(struct zmk_ble_profile));
    raise_profile_changed_event();
}

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
    LOG_DBG("active_profile %d, directed? %s", active_profile, active_profile_is_open() ? "no" : "yes");

    int err = bt_le_adv_start(
        BT_LE_ADV_CONN_NAME,
        zmk_ble_ad, ARRAY_SIZE(zmk_ble_ad),
        NULL, 0);
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    return 0;
};

int zmk_ble_clear_bonds()
{
    LOG_DBG("");
    
    if (bt_addr_le_cmp(&profiles[active_profile].peer, BT_ADDR_LE_ANY)) {
        LOG_DBG("Unpairing!");
        bt_unpair(BT_ID_DEFAULT, &profiles[active_profile].peer);
        set_profile_address(active_profile, BT_ADDR_LE_ANY);
    }

    return 0;
};

int zmk_ble_prof_select(u8_t index)
{
    LOG_DBG("profile %d", index);
    if (active_profile == index) {
        return 0;
    }

    active_profile = index;
    return settings_save_one("ble/active_profile", &active_profile, sizeof(active_profile));

    raise_profile_changed_event();    
};

int zmk_ble_prof_next()
{
    LOG_DBG("");
    return zmk_ble_prof_select((active_profile + 1) % PROFILE_COUNT);
};

int zmk_ble_prof_prev()
{
    LOG_DBG("");
    return zmk_ble_prof_select((active_profile + PROFILE_COUNT - 1) % PROFILE_COUNT);
};

bt_addr_le_t *zmk_ble_active_profile_addr()
{
    return &profiles[active_profile].peer;
}

char *zmk_ble_active_profile_name()
{
    return profiles[active_profile].name;
}

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

    if (settings_name_steq(name, "profiles", &next) && next) {
        char *endptr;
        u8_t idx = strtoul(next, &endptr, 10);
        if (*endptr != '\0') {
            LOG_WRN("Invalid profile index: %s", log_strdup(next));
            return -EINVAL;
        }

        if (len != sizeof(struct zmk_ble_profile)) {
            LOG_ERR("Invalid profile size (got %d expected %d)", len, sizeof(struct zmk_ble_profile));
            return -EINVAL;
        }

        if (idx >= PROFILE_COUNT) {
            LOG_WRN("Profile address for index %d is larger than max of %d", idx, PROFILE_COUNT);
            return -EINVAL;
        }

        int err = read_cb(cb_arg, &profiles[idx], sizeof(struct zmk_ble_profile));
        if (err <= 0) {
            LOG_ERR("Failed to handle profile address from settings (err %d)", err);
            return err;
        }

        char addr_str[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(&profiles[idx].peer, addr_str, sizeof(addr_str));

        LOG_DBG("Loaded %s address for profile %d", log_strdup(addr_str), idx);
    } else if (settings_name_steq(name, "active_profile", &next) && !next) {
        if (len != sizeof(active_profile)) {
            return -EINVAL;
        }

        int err = read_cb(cb_arg, &active_profile, sizeof(active_profile));
        if (err <= 0) {
            LOG_ERR("Failed to handle active profile from settings (err %d)", err);
            return err;
        }
    }
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL)
    else if (settings_name_steq(name, "peripheral_address", &next) && !next) {
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
    // if (bt_addr_le_cmp(&peripheral_addr, BT_ADDR_LE_ANY) && bt_addr_le_cmp(&peripheral_addr, bt_conn_get_dst(conn))) {
    //     zmk_ble_adv_resume();
    // }
#else 
    // zmk_ble_adv_resume();
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

#if !IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL)
static enum bt_security_err auth_pairing_accept(struct bt_conn *conn, const struct bt_conn_pairing_feat *const feat)
{
    struct bt_conn_info info;
    bt_conn_get_info(conn, &info);

    LOG_DBG("role %d, open? %s", info.role, active_profile_is_open() ? "yes" : "no");
    if (info.role == BT_CONN_ROLE_SLAVE && !active_profile_is_open()) {
        LOG_WRN("Rejecting pairing request to taken profile %d", active_profile);
        return BT_SECURITY_ERR_PAIR_NOT_ALLOWED;
    }

    return BT_SECURITY_ERR_SUCCESS;
};
#endif /* !IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL) */

static void auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
    struct bt_conn_info info;
    char addr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *dst = bt_conn_get_dst(conn);

    bt_addr_le_to_str(dst, addr, sizeof(addr));
    bt_conn_get_info(conn, &info);

    if (info.role != BT_CONN_ROLE_SLAVE) {
        LOG_DBG("SKIPPING FOR ROLE %d", info.role);
        return;
    }

#if !IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL)
    if (!active_profile_is_open()) {
        LOG_ERR("Pairing completed but current profile is not open: %s", log_strdup(addr));
        bt_unpair(BT_ID_DEFAULT, dst);
        return;
    }
#endif /* !IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL) */

    set_profile_address(active_profile, dst);
};

static struct bt_conn_auth_cb zmk_ble_auth_cb_display = {
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL)
    .pairing_accept = auth_pairing_accept,
#endif /* !IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL) */
    .pairing_complete = auth_pairing_complete,
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

    for (int i = 0; i < 10; i++) {
        bt_unpair(i, NULL);
    }

    for (int i = 0; i < PROFILE_COUNT; i++) {
        char setting_name[15];
        sprintf(setting_name, "ble/profiles/%d", i);

        err = settings_delete(setting_name);
        if (err) {
            LOG_ERR("Failed to delete setting: %d", err);
        }
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
