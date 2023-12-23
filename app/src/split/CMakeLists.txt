# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

if (CONFIG_ZMK_SPLIT_BLE AND (NOT CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
  target_sources(app PRIVATE listener.c)
  target_sources(app PRIVATE service.c)
endif()

if (CONFIG_ZMK_SPLIT_BLE AND CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE central.c)
endif()

if (CONFIG_ZMK_SPLIT_BLE)
    add_subdirectory(bluetooth)
endif()
