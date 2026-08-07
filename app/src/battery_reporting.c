/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_battery_reporting

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <sys/types.h>
#include <zephyr/kernel.h>

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)
#include <zephyr/bluetooth/gatt.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/central.h>
#include <zmk/workqueue.h>
#include <zmk/battery_names.h>

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_USB)
#include <zmk/hid.h>
#include <zmk/usb_hid.h>
#endif

// TODO before merging: is this needed? or just use a bool array or even bitfield?
// Only `hidden` is used right now, is this information useful elsewhere? For screens?
struct battery_part {
    char *display_name;
    uint16_t cpf;
    bool hidden;
};

#define BAS_LOWEST_CHARGE_INDEX UINT8_MAX
#define BAS_CENTRAL_INDEX 0
#define BAS_PERIPHERAL_INDEX_OFFSET 1

#define KEYBOARD_PARTS_NUM UTIL_INC(CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
static uint8_t lowest_state_of_charge = 0;
#endif

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    ARG_UNUSED(attr);
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("BAS Notifications %s", notif_enabled ? "enabled" : "disabled");
}

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                         uint16_t len, uint16_t offset) {
    const uint8_t index = *(uint8_t *)attr->user_data;
    uint8_t level = 0;

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
    if (index == BAS_LOWEST_CHARGE_INDEX) {
        return bt_gatt_attr_read(conn, attr, buf, len, offset, &lowest_state_of_charge,
                                 sizeof(uint8_t));
    }
#endif

    if (index == BAS_CENTRAL_INDEX) {
        level = zmk_battery_state_of_charge();
    } else {
#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_FETCHING)
        int rc = zmk_split_central_get_peripheral_battery_level(index - BAS_PERIPHERAL_INDEX_OFFSET,
                                                                &level);

        if (rc == -EINVAL) {
            LOG_ERR("Invalid peripheral index requested for battery level read: %d", index);
            return -EINVAL;
        }
#else
        LOG_WRN("Battery level read requested for peripheral %d, but split fetching is disabled",
                index);
        return -EINVAL;
#endif
    }

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &level, sizeof(level));
}

#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)

#define ZMK_BT_GATT_SERVICE_DEFINE(_name, ...)                                                     \
    const struct bt_gatt_attr UTIL_CAT(attr_, _name)[] = {__VA_ARGS__};                            \
    const STRUCT_SECTION_ITERABLE(bt_gatt_service_static, _name) =                                 \
        BT_GATT_SERVICE(UTIL_CAT(attr_, _name))

// Get default CPF description as a literal number for a keyboard part.
// For the central (part 0), default is "main" (0x0106, 262) unless reporting lowest charge, in
// which case use "first" (1). For peripherals, always start from "second" (2) onwards.
#define BAT_REPORT_GET_DEFAULT_CPF_DESC(idx)                                                       \
    COND_CODE_0(                                                                                   \
        idx, (COND_CODE_1(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE, (1), (262))),   \
        (UTIL_INC(idx)))

// Get default display name for a keyboard part.
#define BAT_REPORT_GET_DEFAULT_DISPLAY_NAME(idx)                                                   \
    GET_BATTERY_DISPLAY_NAME_BY_CPF(BAT_REPORT_GET_DEFAULT_CPF_DESC(idx))

#if DT_HAS_CHOSEN(DT_DRV_COMPAT)
// Use info from devicetree chosen node

BUILD_ASSERT(KEYBOARD_PARTS_NUM == DT_CHILD_NUM(DT_CHOSEN(DT_DRV_COMPAT)),
             "Number of battery info set in /chosen/zmk,battery-reporting must match the number of "
             "split parts in this keyboard.");

// Get CPF description as a literal number for a keyboard part from devicetree.
// Use configured cpf property if set, otherwise use default based on index.
#define BAT_REPORT_GET_DT_CPF_DESC(node_id)                                                        \
    DT_PROP_OR(node_id, cpf, BAT_REPORT_GET_DEFAULT_CPF_DESC(DT_NODE_CHILD_IDX(node_id)))

// Get display name for a keyboard part from devicetree.
// Use configured display-name property if set, otherwise use name based on CPF.
#define BAT_REPORT_GET_DT_DISPLAY_NAME(node_id)                                                    \
    DT_PROP_OR(node_id, display_name,                                                              \
               GET_BATTERY_DISPLAY_NAME_BY_CPF(BAT_REPORT_GET_DT_CPF_DESC(node_id)))

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)

// Identifier for GATT service instance for a keyboard part.
// Note: This will only sort correctly for up to 10 keyboard parts (single digit index)
#define BAS_GATT_SERVICE_IDENTIFIER(node_id) UTIL_CAT(bas, DT_NODE_CHILD_IDX(node_id))

// Define a BAS instance for each non-hidden part.
#define DEFINE_BAS_INSTANCE_DT(node_id)                                                            \
    COND_CODE_0(                                                                                   \
        DT_PROP(node_id, hidden),                                                                  \
        (ZMK_BT_GATT_SERVICE_DEFINE(                                                               \
            BAS_GATT_SERVICE_IDENTIFIER(node_id), BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),            \
            BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,                                      \
                                   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ,     \
                                   read_blvl, NULL, ((uint8_t[]){DT_NODE_CHILD_IDX(node_id)})),    \
            BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),             \
            BT_GATT_CPF(((struct bt_gatt_cpf[]){{                                                  \
                .format = 0x04,     /* uint8 */                                                    \
                .exponent = 0x0,    /* */                                                          \
                .unit = 0x27AD,     /* Percentage */                                               \
                .name_space = 0x01, /* Bluetooth SIG */                                            \
                .description = BAT_REPORT_GET_DT_CPF_DESC(node_id),                                \
            }})),                                                                                  \
            BT_GATT_CUD(BAT_REPORT_GET_DT_DISPLAY_NAME(node_id), BT_GATT_PERM_READ))),             \
        ());

DT_FOREACH_CHILD(DT_CHOSEN(DT_DRV_COMPAT), DEFINE_BAS_INSTANCE_DT)

#define BAS_PTR_DT(node_id)                                                                        \
    COND_CODE_0(DT_PROP(node_id, hidden), (&BAS_GATT_SERVICE_IDENTIFIER(node_id)), (NULL))

static const struct bt_gatt_service_static *bas[] = {
    DT_FOREACH_CHILD_SEP(DT_CHOSEN(DT_DRV_COMPAT), BAS_PTR_DT, (, ))};

#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
#define BATTERY_PART_DT(node_id)                                                                   \
    {                                                                                              \
        .display_name = BAT_REPORT_GET_DT_DISPLAY_NAME(node_id),                                   \
        .cpf = BAT_REPORT_GET_DT_CPF_DESC(node_id),                                                \
        .hidden = DT_PROP(node_id, hidden),                                                        \
    }

static const struct battery_part battery_parts[] = {
    DT_FOREACH_CHILD_SEP(DT_CHOSEN(DT_DRV_COMPAT), BATTERY_PART_DT, (, ))};
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)

#else // if DT_HAS_CHOSEN(DT_DRV_COMPAT)
// No chosen devicetree node, use defaults

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)

#define BAS_GATT_SERVICE_IDENTIFIER(idx) UTIL_CAT(bas, idx)

#define DEFINE_BAS_SERVICE_IDX(idx, _)                                                             \
    ZMK_BT_GATT_SERVICE_DEFINE(                                                                    \
        BAS_GATT_SERVICE_IDENTIFIER(idx), BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),                    \
        BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
                               BT_GATT_PERM_READ, read_blvl, NULL, ((uint8_t[]){idx})),            \
        BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                 \
        BT_GATT_CPF(((struct bt_gatt_cpf[]){{                                                      \
            .format = 0x04, /* uint8 */                                                            \
            .exponent = 0x0,                                                                       \
            .unit = 0x27AD,     /* Percentage */                                                   \
            .name_space = 0x01, /* Bluetooth SIG */                                                \
            .description = BAT_REPORT_GET_DEFAULT_CPF_DESC(idx),                                   \
        }})),                                                                                      \
        BT_GATT_CUD(BAT_REPORT_GET_DEFAULT_DISPLAY_NAME(idx), BT_GATT_PERM_READ));

LISTIFY(KEYBOARD_PARTS_NUM, DEFINE_BAS_SERVICE_IDX, ())

#define BAS_PTR_IDX(idx, _) &BAS_GATT_SERVICE_IDENTIFIER(idx)

static const struct bt_gatt_service_static *bas[] = {
    LISTIFY(KEYBOARD_PARTS_NUM, BAS_PTR_IDX, (, ))};

#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
#define BATTERY_PART_IDX(idx, _)                                                                   \
    {                                                                                              \
        .display_name = BAT_REPORT_GET_DEFAULT_DISPLAY_NAME(idx),                                  \
        .cpf = BAT_REPORT_GET_DEFAULT_CPF_DESC(idx),                                               \
        .hidden = false,                                                                           \
    }

static const struct battery_part battery_parts[] = {
    LISTIFY(KEYBOARD_PARTS_NUM, BATTERY_PART_IDX, (, ))};
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)

#endif // if DT_HAS_CHOSEN(DT_DRV_COMPAT) .. else ..

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)
// Hote: the order of services are based on their name, so we prefix it with "a" to ensure it
// appears first.
ZMK_BT_GATT_SERVICE_DEFINE(abas_lowest, BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
                           BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
                                                  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                                                  BT_GATT_PERM_READ, read_blvl, NULL,
                                                  ((uint8_t[]){BAS_LOWEST_CHARGE_INDEX})),
                           BT_GATT_CCC(blvl_ccc_cfg_changed,
                                       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
                           BT_GATT_CPF(((struct bt_gatt_cpf[]){{
                               .format = 0x04, /* uint8 */
                               .exponent = 0x0,
                               .unit = 0x27AD,        /* Percentage */
                               .name_space = 0x01,    /* Bluetooth SIG */
                               .description = 0x0106, /* "main" */
                           }})),
                           BT_GATT_CUD("Lowest Charge", BT_GATT_PERM_READ));
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)

static void zmk_update_lowest_charge_work(struct k_work *work) {
    ARG_UNUSED(work);

    uint8_t new_lowest_level = UINT8_MAX;

    if (!battery_parts[BAS_CENTRAL_INDEX].hidden) {
        new_lowest_level = zmk_battery_state_of_charge();
    }

    for (size_t i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (battery_parts[i + BAS_PERIPHERAL_INDEX_OFFSET].hidden) {
            continue;
        }
        uint8_t level = 0;
        int rc = zmk_split_central_get_peripheral_battery_level(i, &level);
        if (rc < 0) {
            continue;
        }
        if (level != 0 && level < new_lowest_level) {
            new_lowest_level = level;
        }
    }

    if (new_lowest_level == UINT8_MAX) {
        new_lowest_level = 0;
        LOG_DBG("No valid battery levels found, setting lowest to 0");
    }

    if (new_lowest_level != lowest_state_of_charge) {
        lowest_state_of_charge = new_lowest_level;

        LOG_DBG("Lowest state of charge: %d", lowest_state_of_charge);

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)
        int rc = bt_gatt_notify(NULL, &abas_lowest.attrs[2], &lowest_state_of_charge,
                                sizeof(lowest_state_of_charge));
        if (rc < 0 && rc != -ENOTCONN) {
            LOG_WRN("Notify failed for lowest battery level: %d", rc);
        }
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_USB)
        zmk_hid_battery_set(lowest_state_of_charge);
        zmk_usb_hid_send_battery_report();
#endif
    }
}

K_WORK_DEFINE(update_lowest_charge_work, zmk_update_lowest_charge_work);

static void submit_lowest_charge_work() {
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &update_lowest_charge_work);
}

#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)

int peripheral_batt_report_lvl_listener(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->source >= CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS) {
        LOG_WRN("Out of range peripheral index: %d", ev->source);
        return ZMK_EV_EVENT_BUBBLE;
    }

    LOG_DBG("Peripheral %d battery level: %u", ev->source, ev->state_of_charge);

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)
    const struct bt_gatt_service_static *svc = bas[ev->source + BAS_PERIPHERAL_INDEX_OFFSET];

    if (svc) {
        int rc = bt_gatt_notify(NULL, &svc->attrs[2], &ev->state_of_charge, sizeof(uint8_t));
        if (rc < 0 && rc != -ENOTCONN) {
            LOG_WRN("Notify failed for peripheral %d: %d", ev->source, rc);
        }
    } else {
        LOG_DBG("No service found for peripheral %d", ev->source);
    }
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
    if (ev->state_of_charge != 0) {
        // Ugly workaround to not wake up the host on keyboard part disconnect, so users don't need
        // to turn off their keyboard parts before sleeping/hibernating their computer. We ideally
        // should differentiate between disconnect and 0% charge. This shouldn't be an issue for
        // most users, if a part is dead due to low battery, it likely was reporting <5% before
        // disconnecting so user is already aware.
        submit_lowest_charge_work();
    }
#endif

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(peripheral_batt_report_lvl_listener, peripheral_batt_report_lvl_listener);
ZMK_SUBSCRIPTION(peripheral_batt_report_lvl_listener, zmk_peripheral_battery_state_changed);

int central_batt_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)
    const struct bt_gatt_service_static *svc = bas[BAS_CENTRAL_INDEX];

    if (svc) {
        int rc = bt_gatt_notify(NULL, &svc->attrs[2], &ev->state_of_charge, sizeof(uint8_t));
        if (rc < 0 && rc != -ENOTCONN) {
            LOG_WRN("Notify failed for central battery level: %d", rc);
        }
    } else {
        LOG_DBG("No service found for central battery");
    }
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BLE)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_USB) &&                                                \
    !IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
    zmk_hid_battery_set(ev->state_of_charge);
    zmk_usb_hid_send_battery_report();
#endif

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
    if (battery_parts[BAS_CENTRAL_INDEX].hidden == false) {
        submit_lowest_charge_work();
    }
#endif

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(central_batt_state_changed_listener, central_batt_state_changed_listener);
ZMK_SUBSCRIPTION(central_batt_state_changed_listener, zmk_battery_state_changed);
