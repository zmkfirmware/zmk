# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

if (CONFIG_ZMK_SPLIT_BLE)
    add_subdirectory(bluetooth)
endif()

if (CONFIG_ZMK_SPLIT_WIRED)
    add_subdirectory(wired)
endif()

if (CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    target_sources(app PRIVATE central.c)
    zephyr_linker_sources(SECTIONS ../../include/linker/zmk-split-transport-central.ld)
else()
    target_sources(app PRIVATE peripheral.c)
    zephyr_linker_sources(SECTIONS ../../include/linker/zmk-split-transport-peripheral.ld)
endif()