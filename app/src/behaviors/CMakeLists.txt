# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

if (NOT CONFIG_ZMK_SPLIT_BLE_ROLE_PERIPHERAL)
  target_sources(app PRIVATE behavior_key_press.c)
  target_sources(app PRIVATE behavior_reset.c)
  target_sources(app PRIVATE behavior_hold_tap.c)
  target_sources(app PRIVATE behavior_sticky_key.c)
  target_sources(app PRIVATE behavior_momentary_layer.c)
  target_sources(app PRIVATE behavior_outputs.c)
  target_sources(app PRIVATE behavior_toggle_layer.c)
  target_sources(app PRIVATE behavior_transparent.c)
  target_sources(app PRIVATE behavior_none.c)
  target_sources(app PRIVATE behavior_sensor_rotate_key_press.c)
  target_sources_ifdef(CONFIG_ZMK_EXT_POWER app PRIVATE behavior_ext_power.c)
endif()
target_sources_ifdef(CONFIG_ZMK_RGB_UNDERGLOW app PRIVATE behavior_rgb_underglow.c)
target_sources_ifdef(CONFIG_ZMK_BLE app PRIVATE behavior_bt.c)