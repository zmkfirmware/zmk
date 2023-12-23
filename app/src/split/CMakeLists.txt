# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

if (CONFIG_ZMK_SPLIT_BLE AND (NOT CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
  target_sources(app PRIVATE listener.c)
endif()

if (CONFIG_ZMK_SPLIT_BLE)
    add_subdirectory(bluetooth)
endif()
