/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#define ZMK_KEYMAP_SENSORS_NODE DT_INST(0, zmk_keymap_sensors)
#define ZMK_KEYMAP_HAS_SENSORS DT_NODE_HAS_STATUS(ZMK_KEYMAP_SENSORS_NODE, okay)
#define ZMK_KEYMAP_SENSORS_LEN DT_PROP_LEN(ZMK_KEYMAP_SENSORS_NODE, sensors)
#define ZMK_KEYMAP_SENSORS_BY_IDX(idx) DT_PHANDLE_BY_IDX(ZMK_KEYMAP_SENSORS_NODE, sensors, idx)
