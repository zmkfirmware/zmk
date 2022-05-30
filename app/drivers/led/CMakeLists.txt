
# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_sources(led.c)
add_subdirectory_ifdef(CONFIG_IS31FL3733 is31fl3733)