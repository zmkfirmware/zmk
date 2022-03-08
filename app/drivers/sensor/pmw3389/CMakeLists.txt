# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_include_directories(.)

zephyr_library()

zephyr_library_sources(pmw3389.c)
zephyr_library_sources_ifdef(CONFIG_PMW3389_TRIGGER pmw3389_trigger.c)