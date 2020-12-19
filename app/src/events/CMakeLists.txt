# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

target_sources(app PRIVATE activity_state_changed.c)
target_sources(app PRIVATE position_state_changed.c)
target_sources(app PRIVATE layer_state_changed.c)
target_sources(app PRIVATE keycode_state_changed.c)
target_sources(app PRIVATE modifiers_state_changed.c)
target_sources(app PRIVATE sensor_event.c)
target_sources_ifdef(CONFIG_ZMK_BLE app PRIVATE ble_active_profile_changed.c)
target_sources_ifdef(CONFIG_ZMK_BLE app PRIVATE battery_state_changed.c)
target_sources_ifdef(CONFIG_USB app PRIVATE usb_conn_state_changed.c)