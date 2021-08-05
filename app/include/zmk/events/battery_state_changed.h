/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>

struct zmk_battery_state_changed {
    
    /* Remaining capacity as a %age */
    uint8_t state_of_charge;

    /* State of Health */
    int16_t state_of_health;

    /*  Current cell voltage in units of 1.25/16mV */
    uint32_t voltage;

    /* Average current in units of 1.5625uV / Rsense */
    uint32_t current;

    /* Standby current in mA? uA? */
    int32_t current_standby;

    /* Maximum Load Current in mA? uA? */
    int32_t current_max_load;

    /* Full charge capacity in 5/Rsense uA */
    int32_t full_charge_capacity;

    /* Remaining capacity in 5/Rsense uA */
    int32_t remaining_charge_capacity;

    int32_t nominal_available_capacity;

    int32_t full_available_capacity;

    /* Average power consumption in mA? uA? */
    int32_t avg_power;

    /* Internal temperature in units of 1/256 degrees C */
    int32_t int_temp;
};

ZMK_EVENT_DECLARE(zmk_battery_state_changed);