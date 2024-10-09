/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BATTERY_BATTERY_CHARGING_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BATTERY_BATTERY_CHARGING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum sensor_channel_bvd {
    /** Charging state, bool **/
    SENSOR_CHAN_CHARGING = SENSOR_CHAN_PRIV_START,
};

#endif