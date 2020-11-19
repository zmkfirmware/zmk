# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

add_subdirectory_ifdef(CONFIG_KSCAN kscan)
add_subdirectory_ifdef(CONFIG_SENSOR sensor)