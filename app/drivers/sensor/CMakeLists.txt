# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

add_subdirectory_ifdef(CONFIG_ZMK_BATTERY_VOLTAGE_DIVIDER battery_voltage_divider)
add_subdirectory_ifdef(CONFIG_EC11 ec11)