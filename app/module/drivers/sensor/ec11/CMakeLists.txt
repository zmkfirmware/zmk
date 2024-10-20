# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_include_directories(.)

zephyr_library()

zephyr_library_sources(ec11.c)
zephyr_library_sources_ifdef(CONFIG_EC11_TRIGGER ec11_trigger.c)