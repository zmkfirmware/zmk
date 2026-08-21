# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

if (NOT CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE service.c)
  target_sources(app PRIVATE peripheral.c)
endif()
if (CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE central.c)
  target_sources_ifdef(CONFIG_ZMK_SPLIT_BLE_PREF_IDLE app PRIVATE central_idle.c)
endif()

if (CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_PROXY)
  target_sources(app PRIVATE central_bas_proxy.c)
endif()