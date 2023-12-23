# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

if (NOT CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  if (CONFIG_ZMK_SPLIT_BLE OR CONFIG_ZMK_SPLIT_SERIAL)
    target_sources(app PRIVATE listener.c)
    target_sources(app PRIVATE service.c)
  endif()
endif()

if (CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  if (CONFIG_ZMK_SPLIT_BLE OR CONFIG_ZMK_SPLIT_SERIAL)
    target_sources(app PRIVATE central.c)
  endif()
endif()

if (CONFIG_ZMK_SPLIT_BLE)
    add_subdirectory(bluetooth)
endif()

if (CONFIG_ZMK_SPLIT_SERIAL)
    add_subdirectory(serial)
endif()
