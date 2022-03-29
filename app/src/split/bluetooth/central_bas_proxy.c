/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>

// Initialize the charge level to a special value indicating no sampling has been made yet.
static uint8_t last_state_of_peripheral_charge[CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS] = {0};

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    ARG_UNUSED(attr);

    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

    LOG_INF("BAS Notifications %s", notif_enabled ? "enabled" : "disabled");
}

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                         uint16_t len, uint16_t offset) {
    const char *lvl8 = attr->user_data;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, lvl8, sizeof(uint8_t));
}

static const struct bt_gatt_cpf aux_level_cpf = {
    .format = 0x04, // uint8
    .exponent = 0x0,
    .unit = 0x27AD,        // Percentage
    .name_space = 0x01,    // Bluetooth SIG
    .description = 0x0108, // "auxiliary"
};

#define PERIPH_CUD_(x) "Peripheral " #x
#define PERIPH_CUD(x) PERIPH_CUD_(x)

#define PERIPH_BATT_LEVEL_ATTRS(i, _)                                                              \
    BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,     \
                           BT_GATT_PERM_READ, read_blvl, NULL,                                     \
                           &last_state_of_peripheral_charge[i]),                                   \
        BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                 \
        BT_GATT_CPF(&aux_level_cpf), BT_GATT_CUD(PERIPH_CUD(i), BT_GATT_PERM_READ),

BT_GATT_SERVICE_DEFINE(bas_aux, BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
                       LISTIFY(CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS, PERIPH_BATT_LEVEL_ATTRS,
                               ()));

int peripheral_batt_lvl_listener(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    };

    if (ev->source >= CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS) {
        LOG_WRN("Got battery level event for an out of range peripheral index");
        return ZMK_EV_EVENT_BUBBLE;
    }

    LOG_DBG("Peripheral battery level event: %u", ev->state_of_charge);
    last_state_of_peripheral_charge[ev->source] = ev->state_of_charge;

    // 1 offsets for the service attribute, then offset 5 for each battery because that's how
    // many attributes are added per battery
    uint8_t index = 1 + (5 * ev->source);

    int rc = bt_gatt_notify(NULL, &bas_aux.attrs[index], &last_state_of_peripheral_charge,
                            sizeof(last_state_of_peripheral_charge));
    if (rc < 0 && rc != -ENOTCONN) {
        LOG_WRN("Failed to notify hosts of peripheral battery level: %d", rc);
    }

    return ZMK_EV_EVENT_BUBBLE;
};

ZMK_LISTENER(peripheral_batt_lvl_listener, peripheral_batt_lvl_listener);
ZMK_SUBSCRIPTION(peripheral_batt_lvl_listener, zmk_peripheral_battery_state_changed);
