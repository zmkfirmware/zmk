/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <drivers/sensor.h>

int zmk_split_bt_position_pressed(uint8_t position);
int zmk_split_bt_position_released(uint8_t position);
int zmk_split_bt_sensor_triggered(uint8_t sensor_number, struct sensor_value value);
