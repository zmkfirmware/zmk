/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <init.h>
#include <sys/types.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <bluetooth/gatt.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>

const uint8_t NULL_BATTERY_LEVEL = 0xFF;

// Initialize the charge level to a special value indicating no sampling has been made yet.
static uint8_t last_state_of_charge = NULL_BATTERY_LEVEL;
static uint8_t last_state_of_peripheral_charge = NULL_BATTERY_LEVEL;

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

BT_GATT_SERVICE_DEFINE(
    bas, BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
    BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_blvl, NULL, &last_state_of_charge),
    BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CUD("Central", BT_GATT_PERM_READ),
    BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_blvl, NULL, &last_state_of_peripheral_charge),
    BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CUD("Peripheral", BT_GATT_PERM_READ));

const struct device *battery;

int peripheral_batt_lvl_listener(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    };
    LOG_DBG("Peripheral battery level event: %u", ev->state_of_charge);
    last_state_of_peripheral_charge = ev->state_of_charge;

    // TODO: super fragile because of hardcoded attribute index
    int rc = bt_gatt_notify(NULL, &bas.attrs[5], &last_state_of_peripheral_charge,
                            sizeof(last_state_of_peripheral_charge));
    return rc;
};

ZMK_LISTENER(peripheral_batt_lvl_listener, peripheral_batt_lvl_listener);
ZMK_SUBSCRIPTION(peripheral_batt_lvl_listener, zmk_peripheral_battery_state_changed);

uint8_t zmk_battery_state_of_charge() { return last_state_of_charge; }

static int zmk_battery_update(const struct device *battery) {
    struct sensor_value state_of_charge;

    int rc = sensor_sample_fetch_chan(battery, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE);

    if (rc != 0) {
        LOG_DBG("Failed to fetch battery values: %d", rc);
        return rc;
    }

    rc = sensor_channel_get(battery, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &state_of_charge);

    if (rc != 0) {
        LOG_DBG("Failed to get battery state of charge: %d", rc);
        return rc;
    }

    if (last_state_of_charge != state_of_charge.val1) {
        last_state_of_charge = state_of_charge.val1;

        LOG_DBG("Setting BAS GATT battery level to %d.", last_state_of_charge);

        rc = bt_gatt_notify(NULL, &bas.attrs[1], &last_state_of_charge,
                            sizeof(last_state_of_charge));
        if (rc != 0 && rc != -ENOTCONN) {
            LOG_WRN("Failed to set BAS GATT battery level (err %d)", rc);
            return rc;
        }

        rc = ZMK_EVENT_RAISE(new_zmk_battery_state_changed(
            (struct zmk_battery_state_changed){.state_of_charge = last_state_of_charge}));
    }

    return rc;
}

static void zmk_battery_work(struct k_work *work) {
    int rc = zmk_battery_update(battery);

    if (rc != 0) {
        LOG_DBG("Failed to update battery value: %d.", rc);
    }
}

K_WORK_DEFINE(battery_work, zmk_battery_work);

static void zmk_battery_timer(struct k_timer *timer) { k_work_submit(&battery_work); }

K_TIMER_DEFINE(battery_timer, zmk_battery_timer, NULL);

static int zmk_battery_init(const struct device *_arg) {
    battery = device_get_binding("BATTERY");

    if (battery == NULL) {
        LOG_DBG("No battery device labelled BATTERY found.");
        return -ENODEV;
    }

    int rc = zmk_battery_update(battery);

    if (rc != 0) {
        LOG_DBG("Failed to update battery value: %d.", rc);
        return rc;
    }

    k_timer_start(&battery_timer, K_MINUTES(1), K_MINUTES(1));

    return 0;
}

SYS_INIT(zmk_battery_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
