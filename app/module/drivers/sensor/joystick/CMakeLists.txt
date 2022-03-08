# Copyright (c) 2020-2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_include_directories(.)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_JOYSTICK_THB thb.c)
