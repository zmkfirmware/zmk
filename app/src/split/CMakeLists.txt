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
    target_sources_ifdef(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_STATE_SYNC app PRIVATE central_state_sync.c)
    zephyr_linker_sources(SECTIONS ../../include/linker/zmk-split-transport-central.ld)
else()
    target_sources(app PRIVATE peripheral.c)
    target_sources_ifdef(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_STATE_SYNC app PRIVATE peripheral_state_sync.c)
    zephyr_linker_sources(SECTIONS ../../include/linker/zmk-split-transport-peripheral.ld)
endif()
