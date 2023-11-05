/*
 * Copyright (c) 2020 The ZMK Contributors
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
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_err.h>

#if IS_ENABLED(CONFIG_SETTINGS)

#include <zephyr/settings/settings.h>

#endif

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/ble.h>
#include <zmk/keys.h>
#include <zmk/split/bluetooth/uuid.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/ble_auth_state_changed.h>

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY) || IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)
#include <zmk/events/keycode_state_changed.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY)
#define PASSKEY_DIGITS 6

static struct bt_conn *auth_passkey_entry_conn;
RING_BUF_DECLARE(passkey_entries, PASSKEY_DIGITS);
#endif /* IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY) */

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_DISPLAY)
static bool is_displaying_passkey = false;
static unsigned int current_passkey = 0;
#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)
static struct bt_conn *auth_passkey_confirm_conn;
#endif
#endif

enum advertising_type {
    ZMK_ADV_NONE,
    ZMK_ADV_DIR,
    ZMK_ADV_CONN,
} advertising_status;

#define CURR_ADV(adv) (adv << 4)

#define ZMK_ADV_CONN_NAME                                                                          \
    BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME, BT_GAP_ADV_FAST_INT_MIN_2, \
                    BT_GAP_ADV_FAST_INT_MAX_2, NULL)

static struct zmk_ble_profile profiles[ZMK_BLE_PROFILE_COUNT];
static uint8_t active_profile;

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

BUILD_ASSERT(DEVICE_NAME_LEN <= 16, "ERROR: BLE device name is too long. Max length: 16");

static const struct bt_data zmk_ble_ad[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0xC1, 0x03),
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_SOME, 0x12, 0x18, /* HID Service */
                  0x0f, 0x18                       /* Battery Service */
                  ),
};

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

static bt_addr_le_t peripheral_addrs[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];

#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) */

static void raise_auth_state_changed_event(void) {
    ZMK_EVENT_RAISE(new_zmk_ble_auth_state_changed((struct zmk_ble_auth_state_changed){
        .state = zmk_ble_get_auth_state(),
    }));
}

static void raise_auth_state_changed_event_callback(struct k_work *work) {
    raise_auth_state_changed_event();
}

K_WORK_DEFINE(raise_auth_state_changed_event_work, raise_auth_state_changed_event_callback);

static void raise_profile_changed_event(void) {
    ZMK_EVENT_RAISE(new_zmk_ble_active_profile_changed((struct zmk_ble_active_profile_changed){
        .index = active_profile, .profile = &profiles[active_profile]}));
}

static void raise_profile_changed_event_callback(struct k_work *work) {
    raise_profile_changed_event();
}

K_WORK_DEFINE(raise_profile_changed_event_work, raise_profile_changed_event_callback);

bool zmk_ble_active_profile_is_open(void) {
    return !bt_addr_le_cmp(&profiles[active_profile].peer, BT_ADDR_LE_ANY);
}

void set_profile_address(uint8_t index, const bt_addr_le_t *addr) {
    char setting_name[15];
    char addr_str[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    memcpy(&profiles[index].peer, addr, sizeof(bt_addr_le_t));
    sprintf(setting_name, "ble/profiles/%d", index);
    LOG_DBG("Setting profile addr for %s to %s", setting_name, addr_str);
    settings_save_one(setting_name, &profiles[index], sizeof(struct zmk_ble_profile));
    k_work_submit(&raise_profile_changed_event_work);
}

bool zmk_ble_active_profile_is_connected(void) {
    struct bt_conn *conn;
    struct bt_conn_info info;
    bt_addr_le_t *addr = zmk_ble_active_profile_addr();
    if (!bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
        return false;
    } else if ((conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr)) == NULL) {
        return false;
    }

    bt_conn_get_info(conn, &info);

    bt_conn_unref(conn);

    return info.state == BT_CONN_STATE_CONNECTED;
}

static int checked_adv_stop(void) {
    int err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Failed to stop advertising (err %d)", err);
    } else {
        advertising_status = ZMK_ADV_NONE;
    }
    return err;
}

static int checked_dir_adv(void) {
    bt_addr_le_t *addr = zmk_ble_active_profile_addr();
    struct bt_conn *conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);
    if (conn != NULL) { /* TODO: Check status of connection */
        LOG_DBG("Skipping advertising, profile host is already connected");
        bt_conn_unref(conn);
        return 0;
    }
    int err = bt_le_adv_start(BT_LE_ADV_CONN_DIR_LOW_DUTY(addr), zmk_ble_ad, ARRAY_SIZE(zmk_ble_ad),
                              NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
    } else {
        advertising_status = ZMK_ADV_DIR;
    }
    return err;
}

static int checked_open_adv(void) {
    int err = bt_le_adv_start(ZMK_ADV_CONN_NAME, zmk_ble_ad, ARRAY_SIZE(zmk_ble_ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
    } else {
        advertising_status = ZMK_ADV_CONN;
    }
    return err;
}

int update_advertising(void) {
    int err = 0;
    enum advertising_type desired_adv = ZMK_ADV_NONE;

    if (zmk_ble_active_profile_is_open()) {
        desired_adv = ZMK_ADV_CONN;
    } else if (!zmk_ble_active_profile_is_connected()) {
        desired_adv = ZMK_ADV_CONN;
        // Need to fix directed advertising for privacy centrals. See
        // https://github.com/zephyrproject-rtos/zephyr/pull/14984 char
        // addr_str[BT_ADDR_LE_STR_LEN]; bt_addr_le_to_str(zmk_ble_active_profile_addr(), addr_str,
        // sizeof(addr_str));

        // LOG_DBG("Directed advertising to %s", addr_str);
        // desired_adv = ZMK_ADV_DIR;
    }
    LOG_DBG("advertising from %d to %d", advertising_status, desired_adv);

    switch (desired_adv + CURR_ADV(advertising_status)) {
    case ZMK_ADV_NONE + CURR_ADV(ZMK_ADV_DIR):
    case ZMK_ADV_NONE + CURR_ADV(ZMK_ADV_CONN):
        err = checked_adv_stop();
        break;
    case ZMK_ADV_DIR + CURR_ADV(ZMK_ADV_DIR):
    case ZMK_ADV_DIR + CURR_ADV(ZMK_ADV_CONN):
        err = checked_adv_stop();
        if (!err) {
            err = checked_dir_adv();
        }
        break;
    case ZMK_ADV_DIR + CURR_ADV(ZMK_ADV_NONE):
        err = checked_dir_adv();
        break;
    case ZMK_ADV_CONN + CURR_ADV(ZMK_ADV_DIR):
        err = checked_adv_stop();
        if (!err) {
            err = checked_open_adv();
        }
        break;
    case ZMK_ADV_CONN + CURR_ADV(ZMK_ADV_NONE):
        err = checked_open_adv();
        break;
    }

    return err;
};

static void update_advertising_callback(struct k_work *work) { update_advertising(); }

K_WORK_DEFINE(update_advertising_work, update_advertising_callback);

int zmk_ble_clear_bonds(void) {
    LOG_DBG("");

    if (bt_addr_le_cmp(&profiles[active_profile].peer, BT_ADDR_LE_ANY)) {
        LOG_DBG("Unpairing!");
        bt_unpair(BT_ID_DEFAULT, &profiles[active_profile].peer);
        set_profile_address(active_profile, BT_ADDR_LE_ANY);
    }

    update_advertising();

    return 0;
};

int zmk_ble_active_profile_index(void) { return active_profile; }

#if IS_ENABLED(CONFIG_SETTINGS)
static void ble_save_profile_work(struct k_work *work) {
    settings_save_one("ble/active_profile", &active_profile, sizeof(active_profile));
}

static struct k_work_delayable ble_save_work;
#endif

static int ble_save_profile(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    return k_work_reschedule(&ble_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#else
    return 0;
#endif
}

int zmk_ble_prof_select(uint8_t index) {
    if (index >= ZMK_BLE_PROFILE_COUNT) {
        return -ERANGE;
    }

    LOG_DBG("profile %d", index);
    if (active_profile == index) {
        return 0;
    }

    active_profile = index;
    ble_save_profile();

    update_advertising();

    raise_profile_changed_event();

    return 0;
};

int zmk_ble_prof_next(void) {
    LOG_DBG("");
    return zmk_ble_prof_select((active_profile + 1) % ZMK_BLE_PROFILE_COUNT);
};

int zmk_ble_prof_prev(void) {
    LOG_DBG("");
    return zmk_ble_prof_select((active_profile + ZMK_BLE_PROFILE_COUNT - 1) %
                               ZMK_BLE_PROFILE_COUNT);
};

bt_addr_le_t *zmk_ble_active_profile_addr(void) { return &profiles[active_profile].peer; }

char *zmk_ble_active_profile_name(void) { return profiles[active_profile].name; }

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY)
static unsigned int get_passkey_entry_passkey(void) {
    uint8_t digits[PASSKEY_DIGITS];
    uint32_t count = ring_buf_peek(&passkey_entries, digits, PASSKEY_DIGITS);

    unsigned int passkey = 0;
    for (int i = 0; i < count; i++) {
        passkey = (passkey * 10) + digits[i];
    }

    return passkey;
}

static uint8_t get_passkey_entry_cursor_index(void) {
    const uint32_t size = ring_buf_size_get(&passkey_entries);
    __ASSERT(size <= 255, "Invalid passkey size %u", size);
    return size;
}
#endif

struct zmk_ble_auth_state zmk_ble_get_auth_state(void) {
    struct zmk_ble_auth_state state = {
        .mode = ZMK_BLE_AUTH_MODE_NONE,
        .profile_index = active_profile,
    };

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)
    if (auth_passkey_confirm_conn) {
        state.mode = ZMK_BLE_AUTH_MODE_PASSKEY_CONFIRM;
        state.passkey = current_passkey;
        return state;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_DISPLAY)
    if (is_displaying_passkey) {
        state.mode = ZMK_BLE_AUTH_MODE_PASSKEY_DISPLAY;
        state.passkey = current_passkey;
        return state;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY)
    if (auth_passkey_entry_conn) {
        state.mode = ZMK_BLE_AUTH_MODE_PASSKEY_ENTRY;
        state.passkey = get_passkey_entry_passkey();
        state.cursor_index = get_passkey_entry_cursor_index();
        return state;
    }
#endif

    return state;
}

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

int zmk_ble_put_peripheral_addr(const bt_addr_le_t *addr) {
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        // If the address is recognized and already stored in settings, return
        // index and no additional action is necessary.
        if (bt_addr_le_cmp(&peripheral_addrs[i], addr) == 0) {
            LOG_DBG("Found existing peripheral address in slot %d", i);
            return i;
        } else {
            char addr_str[BT_ADDR_LE_STR_LEN];
            bt_addr_le_to_str(&peripheral_addrs[i], addr_str, sizeof(addr_str));
            LOG_DBG("peripheral slot %d occupied by %s", i, addr_str);
        }

        // If the peripheral address slot is open, store new peripheral in the
        // slot and return index. This compares against BT_ADDR_LE_ANY as that
        // is the zero value.
        if (bt_addr_le_cmp(&peripheral_addrs[i], BT_ADDR_LE_ANY) == 0) {
            char addr_str[BT_ADDR_LE_STR_LEN];
            bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
            LOG_DBG("Storing peripheral %s in slot %d", addr_str, i);
            bt_addr_le_copy(&peripheral_addrs[i], addr);

            char setting_name[32];
            sprintf(setting_name, "ble/peripheral_addresses/%d", i);
            settings_save_one(setting_name, addr, sizeof(bt_addr_le_t));

            return i;
        }
    }

    // The peripheral does not match a known peripheral and there is no
    // available slot.
    return -ENOMEM;
}

#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) */

#if IS_ENABLED(CONFIG_SETTINGS)

static int ble_profiles_handle_set(const char *name, size_t len, settings_read_cb read_cb,
                                   void *cb_arg) {
    const char *next;

    LOG_DBG("Setting BLE value %s", name);

    if (settings_name_steq(name, "profiles", &next) && next) {
        char *endptr;
        uint8_t idx = strtoul(next, &endptr, 10);
        if (*endptr != '\0') {
            LOG_WRN("Invalid profile index: %s", next);
            return -EINVAL;
        }

        if (len != sizeof(struct zmk_ble_profile)) {
            LOG_ERR("Invalid profile size (got %d expected %d)", len,
                    sizeof(struct zmk_ble_profile));
            return -EINVAL;
        }

        if (idx >= ZMK_BLE_PROFILE_COUNT) {
            LOG_WRN("Profile address for index %d is larger than max of %d", idx,
                    ZMK_BLE_PROFILE_COUNT);
            return -EINVAL;
        }

        int err = read_cb(cb_arg, &profiles[idx], sizeof(struct zmk_ble_profile));
        if (err <= 0) {
            LOG_ERR("Failed to handle profile address from settings (err %d)", err);
            return err;
        }

        char addr_str[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(&profiles[idx].peer, addr_str, sizeof(addr_str));

        LOG_DBG("Loaded %s address for profile %d", addr_str, idx);
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
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    else if (settings_name_steq(name, "peripheral_addresses", &next) && next) {
        if (len != sizeof(bt_addr_le_t)) {
            return -EINVAL;
        }

        int i = atoi(next);
        if (i < 0 || i >= ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
            LOG_ERR("Failed to store peripheral address in memory");
        } else {
            int err = read_cb(cb_arg, &peripheral_addrs[i], sizeof(bt_addr_le_t));
            if (err <= 0) {
                LOG_ERR("Failed to handle peripheral address from settings (err %d)", err);
                return err;
            }
        }
    }
#endif

    return 0;
};

struct settings_handler profiles_handler = {.name = "ble", .h_set = ble_profiles_handle_set};
#endif /* IS_ENABLED(CONFIG_SETTINGS) */

static bool is_conn_active_profile(const struct bt_conn *conn) {
    return bt_addr_le_cmp(bt_conn_get_dst(conn), &profiles[active_profile].peer) == 0;
}

static void connected(struct bt_conn *conn, uint8_t err) {
    char addr[BT_ADDR_LE_STR_LEN];
    struct bt_conn_info info;
    LOG_DBG("Connected thread: %p", k_current_get());

    bt_conn_get_info(conn, &info);

    if (info.role != BT_CONN_ROLE_PERIPHERAL) {
        LOG_DBG("SKIPPING FOR ROLE %d", info.role);
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    advertising_status = ZMK_ADV_NONE;

    if (err) {
        LOG_WRN("Failed to connect to %s (%u)", addr, err);
        update_advertising();
        return;
    }

    LOG_DBG("Connected %s", addr);

    if (bt_conn_set_security(conn, BT_SECURITY_L2)) {
        LOG_ERR("Failed to set security");
    }

    update_advertising();

    if (is_conn_active_profile(conn)) {
        LOG_DBG("Active profile connected");
        k_work_submit(&raise_profile_changed_event_work);
    }
}

static void passkey_cancel(void);

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];
    struct bt_conn_info info;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Disconnected from %s (reason 0x%02x)", addr, reason);

    bt_conn_get_info(conn, &info);

    if (info.role != BT_CONN_ROLE_PERIPHERAL) {
        LOG_DBG("SKIPPING FOR ROLE %d", info.role);
        return;
    }

    passkey_cancel();

    // We need to do this in a work callback, otherwise the advertising update will still see the
    // connection for a profile as active, and not start advertising yet.
    k_work_submit(&update_advertising_work);

    if (is_conn_active_profile(conn)) {
        LOG_DBG("Active profile disconnected");
        k_work_submit(&raise_profile_changed_event_work);
    }
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

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_DISPLAY)

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Passkey for %s: %06u", addr, passkey);

    is_displaying_passkey = true;
    current_passkey = passkey;

    k_work_submit(&raise_auth_state_changed_event_work);
}

static void auth_passkey_display_stop(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)
    if (auth_passkey_confirm_conn) {
        LOG_DBG("Passkey confirmation stopped");
        bt_conn_unref(auth_passkey_confirm_conn);
        auth_passkey_confirm_conn = NULL;
    }
#endif

    if (is_displaying_passkey) {
        LOG_DBG("Passkey display stopped");

        is_displaying_passkey = false;
        current_passkey = 0;
        k_work_submit(&raise_auth_state_changed_event_work);
    }
}
#endif // IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_DISPLAY)

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey) {
    LOG_DBG("Waiting for passkey confirmation");

    auth_passkey_confirm_conn = bt_conn_ref(conn);
    auth_passkey_display(conn, passkey);
}

#endif // IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY)

static void auth_passkey_entry(struct bt_conn *conn) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Passkey entry requested for %s", addr);
    ring_buf_reset(&passkey_entries);
    auth_passkey_entry_conn = bt_conn_ref(conn);

    k_work_submit(&raise_auth_state_changed_event_work);
}

static void auth_passkey_entry_stop(void) {
    if (auth_passkey_entry_conn) {
        LOG_DBG("Passkey entry stopped");

        bt_conn_unref(auth_passkey_entry_conn);
        auth_passkey_entry_conn = NULL;
        k_work_submit(&raise_auth_state_changed_event_work);
    }
}

#endif // IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY)

static void passkey_cancel(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_DISPLAY)
    auth_passkey_display_stop();
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY)
    auth_passkey_entry_stop();
#endif
}

static void auth_cancel(struct bt_conn *conn) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Pairing cancelled: %s", addr);

    passkey_cancel();
}

static enum bt_security_err auth_pairing_accept(struct bt_conn *conn,
                                                const struct bt_conn_pairing_feat *const feat) {
    struct bt_conn_info info;
    bt_conn_get_info(conn, &info);

    LOG_DBG("role %d, open? %s", info.role, zmk_ble_active_profile_is_open() ? "yes" : "no");
    if (info.role == BT_CONN_ROLE_PERIPHERAL && !zmk_ble_active_profile_is_open()) {
        LOG_WRN("Rejecting pairing request to taken profile %d", active_profile);
        return BT_SECURITY_ERR_PAIR_NOT_ALLOWED;
    }

    return BT_SECURITY_ERR_SUCCESS;
};

static void auth_pairing_complete(struct bt_conn *conn, bool bonded) {
    struct bt_conn_info info;
    char addr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *dst = bt_conn_get_dst(conn);

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_DISPLAY)
    auth_passkey_display_stop();
#endif

    bt_addr_le_to_str(dst, addr, sizeof(addr));
    bt_conn_get_info(conn, &info);

    if (info.role != BT_CONN_ROLE_PERIPHERAL) {
        LOG_DBG("SKIPPING FOR ROLE %d", info.role);
        return;
    }

    if (!zmk_ble_active_profile_is_open()) {
        LOG_ERR("Pairing completed but current profile is not open: %s", addr);
        bt_unpair(BT_ID_DEFAULT, dst);
        return;
    }

    LOG_DBG("Pairing complete: %s", addr);

    set_profile_address(active_profile, dst);
    update_advertising();
};

static struct bt_conn_auth_cb zmk_ble_auth_cb_display = {
    .pairing_accept = auth_pairing_accept,
#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)
    .passkey_confirm = auth_passkey_confirm,
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_DISPLAY)
    .passkey_display = auth_passkey_display,
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY)
    .passkey_entry = auth_passkey_entry,
#endif
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb zmk_ble_auth_info_cb_display = {
    .pairing_complete = auth_pairing_complete,
};

static void zmk_ble_ready(int err) {
    LOG_DBG("ready? %d", err);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    update_advertising();
}

static int zmk_ble_init(const struct device *_arg) {
    int err = bt_enable(NULL);

    if (err) {
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

    k_work_init_delayable(&ble_save_work, ble_save_profile_work);

    settings_load_subtree("ble");
    settings_load_subtree("bt");

#endif

#if IS_ENABLED(CONFIG_ZMK_BLE_CLEAR_BONDS_ON_START)
    LOG_WRN("Clearing all existing BLE bond information from the keyboard");

    bt_unpair(BT_ID_DEFAULT, NULL);

    for (int i = 0; i < 8; i++) {
        char setting_name[15];
        sprintf(setting_name, "ble/profiles/%d", i);

        err = settings_delete(setting_name);
        if (err) {
            LOG_ERR("Failed to delete setting: %d", err);
        }
    }

    // Hardcoding a reasonable hardcoded value of peripheral addresses
    // to clear so we properly clear a split central as well.
    for (int i = 0; i < 8; i++) {
        char setting_name[32];
        sprintf(setting_name, "ble/peripheral_addresses/%d", i);

        err = settings_delete(setting_name);
        if (err) {
            LOG_ERR("Failed to delete setting: %d", err);
        }
    }

#endif // IS_ENABLED(CONFIG_ZMK_BLE_CLEAR_BONDS_ON_START)

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&zmk_ble_auth_cb_display);
    bt_conn_auth_info_cb_register(&zmk_ble_auth_info_cb_display);

    zmk_ble_ready(0);

    return 0;
}

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY)

static bool zmk_ble_numeric_usage_to_value(const zmk_key_t key, const zmk_key_t one,
                                           const zmk_key_t zero, uint8_t *value) {
    if (key < one || key > zero) {
        return false;
    }

    *value = (key == zero) ? 0 : (key - one + 1);
    return true;
}

static bool get_numeric_usage_value(const zmk_key_t key, uint8_t *value) {
    return zmk_ble_numeric_usage_to_value(key, HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION,
                                          HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS, value) ||
           zmk_ble_numeric_usage_to_value(key, HID_USAGE_KEY_KEYPAD_1_AND_END,
                                          HID_USAGE_KEY_KEYPAD_0_AND_INSERT, value);
}

static void add_passkey_digit(uint8_t value) {
    if (ring_buf_space_get(&passkey_entries) <= 0) {
        uint8_t discard_val;
        ring_buf_get(&passkey_entries, &discard_val, 1);
    }

    ring_buf_put(&passkey_entries, &value, 1);
    LOG_DBG("value entered: %d, digits collected so far: %d", value,
            ring_buf_size_get(&passkey_entries));

    k_work_submit(&raise_auth_state_changed_event_work);
}

static void remove_passkey_digit(void) {
    uint8_t digits[PASSKEY_DIGITS];
    uint32_t count = ring_buf_get(&passkey_entries, digits, PASSKEY_DIGITS);

    if (count == 0) {
        return;
    }

    ring_buf_put(&passkey_entries, digits, count - 1);

    k_work_submit(&raise_auth_state_changed_event_work);
}

static int handle_key_passkey_entry(const zmk_key_t key) {
    LOG_DBG("Passkey entry: key %d", key);

    if (key == HID_USAGE_KEY_KEYBOARD_ESCAPE) {
        LOG_DBG("User canceled passkey entry");
        bt_conn_auth_cancel(auth_passkey_entry_conn);
        return ZMK_EV_EVENT_HANDLED;
    }

    if (key == HID_USAGE_KEY_KEYBOARD_DELETE_BACKSPACE || key == HID_USAGE_KEY_KEYPAD_BACKSPACE) {
        remove_passkey_digit();
        return ZMK_EV_EVENT_HANDLED;
    }

    if (key == HID_USAGE_KEY_KEYBOARD_RETURN || key == HID_USAGE_KEY_KEYBOARD_RETURN_ENTER) {
        if (get_passkey_entry_cursor_index() < PASSKEY_DIGITS) {
            LOG_DBG("Ignoring incomplete passkey");
            return ZMK_EV_EVENT_HANDLED;
        }

        const unsigned int passkey = get_passkey_entry_passkey();
        LOG_DBG("Final passkey: %d", passkey);
        bt_conn_auth_passkey_entry(auth_passkey_entry_conn, passkey);

        auth_passkey_entry_stop();

        return ZMK_EV_EVENT_HANDLED;
    }

    uint8_t val;
    if (get_numeric_usage_value(key, &val)) {
        add_passkey_digit(val);
        return ZMK_EV_EVENT_HANDLED;
    }

    LOG_DBG("Key not used for passkey entry, ignoring");
    return ZMK_EV_EVENT_BUBBLE;
}
#endif // IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY)

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)
static int handle_key_passkey_confirm(const zmk_key_t key) {
    LOG_DBG("Passkey confirm: key %d", key);

    if (key == HID_USAGE_KEY_KEYBOARD_ESCAPE) {
        LOG_DBG("User canceled passkey confirmation");
        bt_conn_auth_cancel(auth_passkey_confirm_conn);
        return ZMK_EV_EVENT_HANDLED;
    }

    if (key == HID_USAGE_KEY_KEYBOARD_RETURN || key == HID_USAGE_KEY_KEYBOARD_RETURN_ENTER) {
        LOG_DBG("Passkey confirmed");
        bt_conn_auth_passkey_confirm(auth_passkey_confirm_conn);
        auth_passkey_display_stop();

        return ZMK_EV_EVENT_HANDLED;
    }

    LOG_DBG("Key not used for passkey confirmation, ignoring");
    return ZMK_EV_EVENT_BUBBLE;
}
#endif // IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY) || IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)

static int zmk_ble_handle_key_user(struct zmk_keycode_state_changed *event) {
    if (!event->state) {
        // Key released. Ignore.
        return ZMK_EV_EVENT_BUBBLE;
    }

#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)
    if (auth_passkey_confirm_conn) {
        return handle_key_passkey_confirm(event->keycode);
    }
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY)
    if (auth_passkey_entry_conn) {
        return handle_key_passkey_entry(event->keycode);
    }
#endif

    // Passkey entry/confirm not active. Ignore.
    return ZMK_EV_EVENT_BUBBLE;
}

static int zmk_ble_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *kc_state;

    kc_state = as_zmk_keycode_state_changed(eh);

    if (kc_state != NULL) {
        return zmk_ble_handle_key_user(kc_state);
    }

    return 0;
}

ZMK_LISTENER(zmk_ble, zmk_ble_listener);
ZMK_SUBSCRIPTION(zmk_ble, zmk_keycode_state_changed);
#endif // IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_ENTRY) || IS_ENABLED(CONFIG_ZMK_BLE_PASSKEY_CONFIRM)

SYS_INIT(zmk_ble_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
