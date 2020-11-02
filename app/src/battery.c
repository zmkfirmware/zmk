/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <bluetooth/services/bas.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event-manager.h>
#include <zmk/events/battery-state-changed.h>

struct device *battery;

static int zmk_battery_update(struct device *battery) {
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

    LOG_DBG("Setting BAS GATT battery level to %d.", state_of_charge.val1);

    rc = bt_gatt_bas_set_battery_level(state_of_charge.val1);

    if (rc != 0) {
        LOG_WRN("Failed to set BAS GATT battery level (err %d)", rc);
        return rc;
    }

    struct battery_state_changed *ev = new_battery_state_changed();
    ev->state_of_charge = state_of_charge.val1;
    return ZMK_EVENT_RAISE(ev);
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

static int zmk_battery_init(struct device *_arg) {
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
