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

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)
#include <zephyr/bluetooth/gatt.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/central.h>
#include <zmk/workqueue.h>

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_HID)
#include <zmk/hid.h>
#include <zmk/usb_hid.h>
#endif

struct battery_part {
    char *display_name;
    uint16_t cpf;
    bool hidden;
};

#define BAS_CENTRAL_INDEX 0
#define BAS_PERIPH_OFFSET 1
#define BAS_LEN UTIL_INC(CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)

static uint8_t lowest_state_of_charge = 0;

#endif

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

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
    if (index == UINT8_MAX) {
        return bt_gatt_attr_read(conn, attr, buf, len, offset, &lowest_state_of_charge,
                                 sizeof(uint8_t));
    }
#endif

    if (index == BAS_CENTRAL_INDEX) {
        level = zmk_battery_state_of_charge();
    } else {
#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_FETCHING)
        int rc = zmk_split_central_get_peripheral_battery_level(index - BAS_PERIPH_OFFSET, &level);

        if (rc == -EINVAL) {
            LOG_ERR("Invalid peripheral index requested for battery level read: %d", index);
            return -EINVAL;
        }
#else
        level = 0;
        LOG_WRN("Battery level read requested for peripheral %d, but split fetching is disabled",
                index);
        return 0;
#endif
    }

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &level, sizeof(uint8_t));
}

#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

#define CPF_DESC_BASE                                                                              \
    COND_CODE_1(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE, (0x0001), (0x0000))

#define PERIPH_CUD(x) PERIPH_CUD_(x)
#define PERIPH_CUD_(x) "Peripheral " #x

#define ZMK_BT_GATT_SERVICE_DEFINE(_name, ...)                                                     \
    const struct bt_gatt_attr UTIL_CAT(attr_, _name)[] = {__VA_ARGS__};                            \
    const STRUCT_SECTION_ITERABLE(bt_gatt_service_static, _name) =                                 \
        BT_GATT_SERVICE(UTIL_CAT(attr_, _name))

#if DT_HAS_CHOSEN(DT_DRV_COMPAT)

#define ONE(...) 1
#define BATTERY_INFO_LEN (DT_FOREACH_CHILD_SEP(DT_CHOSEN(DT_DRV_COMPAT), ONE, (+)))
BUILD_ASSERT(BAS_LEN == BATTERY_INFO_LEN,
             "Number of battery info set in /chosen/zmk,battery-reporting must match the number of "
             "split parts in this keyboard.");

#define BAS_NAME(node_id) UTIL_CAT(bas, DT_NODE_CHILD_IDX(node_id))

#define GET_BATT_DISPLAY_NAME(node_id)                                                             \
    DT_PROP_OR(node_id, display_name,                                                              \
               (DT_NODE_CHILD_IDX(node_id) ? PERIPH_CUD(DT_NODE_CHILD_IDX(node_id)) : "Central"))

#define GET_BATT_CPF_DESC(node_id)                                                                 \
    DT_PROP_OR(node_id, cpf,                                                                       \
               (DT_NODE_CHILD_IDX(node_id)                                                         \
                    ? (CPF_DESC_BASE + DT_NODE_CHILD_IDX(node_id)) /* "first" + index */           \
                    : (COND_CODE_1(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE,        \
                                   (0x0001), (0x0106))))) /* "first", or "main" */

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

#define DEFINE_BAS_INSTANCE(node_id)                                                               \
    COND_CODE_0(                                                                                   \
        DT_PROP(node_id, hidden),                                                                  \
        (ZMK_BT_GATT_SERVICE_DEFINE(                                                               \
            BAS_NAME(node_id), BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),                               \
            BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,                                      \
                                   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ,     \
                                   read_blvl, NULL, ((uint8_t[]){DT_NODE_CHILD_IDX(node_id)})),    \
            BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),             \
            BT_GATT_CPF(((struct bt_gatt_cpf[]){{                                                  \
                .format = 0x04,     /* uint8 */                                                    \
                .exponent = 0x0,    /* */                                                          \
                .unit = 0x27AD,     /* Percentage */                                               \
                .name_space = 0x01, /* Bluetooth SIG */                                            \
                .description = GET_BATT_CPF_DESC(node_id),                                         \
            }})),                                                                                  \
            BT_GATT_CUD(GET_BATT_DISPLAY_NAME(node_id), BT_GATT_PERM_READ))),                      \
        ());

DT_FOREACH_CHILD(DT_CHOSEN(DT_DRV_COMPAT), DEFINE_BAS_INSTANCE)

#define BAS_PTR(node_id) COND_CODE_0(DT_PROP(node_id, hidden), (&BAS_NAME(node_id)), (NULL))

static const struct bt_gatt_service_static *bas[] = {
    DT_FOREACH_CHILD_SEP(DT_CHOSEN(DT_DRV_COMPAT), BAS_PTR, (, ))};

#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
#define BATTERY_PART(node_id)                                                                      \
    {                                                                                              \
        .display_name = GET_BATT_DISPLAY_NAME(node_id),                                            \
        .cpf = GET_BATT_CPF_DESC(node_id),                                                         \
        .hidden = DT_PROP(node_id, hidden),                                                        \
    }

static const struct battery_part battery_parts[] = {
    DT_FOREACH_CHILD_SEP(DT_CHOSEN(DT_DRV_COMPAT), BATTERY_PART, (, ))};
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)

#else // if DT_HAS_CHOSEN(DT_DRV_COMPAT)

#define GET_BATT_DISPLAY_NAME(INDEX) (INDEX ? PERIPH_CUD(INDEX) : "Central")

#define GET_BATT_CPF_DESC(INDEX)                                                                   \
    (INDEX ? (CPF_DESC_BASE + INDEX) /* "first" + index */                                         \
           : (COND_CODE_1(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE, (0x0001),       \
                          (0x0106)))) /* "first", or "main" */

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

#define DEFINE_BAS_SERVICE(INDEX, _)                                                               \
    ZMK_BT_GATT_SERVICE_DEFINE(                                                                    \
        bas##INDEX, BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),                                          \
        BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
                               BT_GATT_PERM_READ, read_blvl, NULL, ((uint8_t[]){INDEX})),          \
        BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                 \
        BT_GATT_CPF(((struct bt_gatt_cpf[]){{                                                      \
            .format = 0x04, /* uint8 */                                                            \
            .exponent = 0x0,                                                                       \
            .unit = 0x27AD,     /* Percentage */                                                   \
            .name_space = 0x01, /* Bluetooth SIG */                                                \
            .description = GET_BATT_CPF_DESC(INDEX),                                               \
        }})),                                                                                      \
        BT_GATT_CUD(GET_BATT_DISPLAY_NAME(INDEX), BT_GATT_PERM_READ));

LISTIFY(BAS_LEN, DEFINE_BAS_SERVICE, ())

#define BAS_PTR(INDEX, _) &bas##INDEX

static const struct bt_gatt_service_static *bas[] = {LISTIFY(BAS_LEN, BAS_PTR, (, ))};

#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
#define BATTERY_PART(INDEX, _)                                                                     \
    {                                                                                              \
        .display_name = GET_BATT_DISPLAY_NAME(INDEX),                                              \
        .cpf = GET_BATT_CPF_DESC(INDEX),                                                           \
        .hidden = false,                                                                           \
    }

static const struct battery_part battery_parts[] = {LISTIFY(BAS_LEN, BATTERY_PART, (, ))};
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)

#endif // else DT_HAS_CHOSEN(DT_DRV_COMPAT)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

// note: the order of the services are based on their name.
ZMK_BT_GATT_SERVICE_DEFINE(
    aggr_bas_lowest, BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
    BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_blvl, NULL, ((uint8_t[]){UINT8_MAX})),
    BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CPF(((struct bt_gatt_cpf[]){{
        .format = 0x04, /* uint8 */
        .exponent = 0x0,
        .unit = 0x27AD,        /* Percentage */
        .name_space = 0x01,    /* Bluetooth SIG */
        .description = 0x0106, /* "main" */
    }})),
    BT_GATT_CUD("Lowest Charge", BT_GATT_PERM_READ));

#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

static void zmk_update_lowest_charge_work(struct k_work *work) {
    ARG_UNUSED(work);

    uint8_t new_lowest_level = UINT8_MAX;

    if (!battery_parts[0].hidden) {
        new_lowest_level = zmk_battery_state_of_charge();
    }

    for (size_t i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (battery_parts[i + BAS_PERIPH_OFFSET].hidden) {
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

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

        int rc = bt_gatt_notify(NULL, &aggr_bas_lowest.attrs[2], &lowest_state_of_charge,
                                sizeof(lowest_state_of_charge));
        if (rc < 0 && rc != -ENOTCONN) {
            LOG_WRN("Notify failed for lowest battery level: %d", rc);
        }

#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_HID)
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

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)
    const struct bt_gatt_service_static *svc = bas[ev->source + BAS_PERIPH_OFFSET];

    if (svc) {
        int rc = bt_gatt_notify(NULL, &svc->attrs[2], &ev->state_of_charge, sizeof(uint8_t));
        if (rc < 0 && rc != -ENOTCONN) {
            LOG_WRN("Notify failed for peripheral %d: %d", ev->source, rc);
        }
    } else {
        LOG_DBG("No service found for peripheral %d", ev->source);
    }
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
    submit_lowest_charge_work();
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

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)
    const struct bt_gatt_service_static *svc = bas[BAS_CENTRAL_INDEX];

    if (svc) {
        int rc = bt_gatt_notify(NULL, &svc->attrs[2], &ev->state_of_charge, sizeof(uint8_t));
        if (rc < 0 && rc != -ENOTCONN) {
            LOG_WRN("Notify failed for central battery level: %d", rc);
        }
    } else {
        LOG_DBG("No service found for central battery");
    }
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_BAS)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_HID) &&                                                \
    !IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
    zmk_hid_battery_set(ev->state_of_charge);
    zmk_usb_hid_send_battery_report();
#endif

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING_SPLIT_REPORT_LOWEST_CHARGE)
    submit_lowest_charge_work();
#endif

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(central_batt_state_changed_listener, central_batt_state_changed_listener);
ZMK_SUBSCRIPTION(central_batt_state_changed_listener, zmk_battery_state_changed);
