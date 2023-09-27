/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/matrix.h>
#include <zmk/sensors.h>

/**
 * Gets the virtual key position to use for the sensor with the given index.
 */
#define ZMK_VIRTUAL_KEY_POSITION_SENSOR(index) (ZMK_KEYMAP_LEN + (index))

/**
 * Gets the sensor number from the virtual key position.
 */
#define ZMK_SENSOR_POSITION_FROM_VIRTUAL_KEY_POSITION(vkp) ((vkp)-ZMK_KEYMAP_LEN)

/**
 * Gets the virtual key position to use for the combo with the given index.
 */
#define ZMK_VIRTUAL_KEY_POSITION_COMBO(index) (ZMK_KEYMAP_LEN + ZMK_KEYMAP_SENSORS_LEN + (index))
