# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

add_subdirectory_ifdef(CONFIG_ZMK_DRIVERS_GPIO gpio)
add_subdirectory(kscan)
add_subdirectory(sensor)
add_subdirectory(display)
