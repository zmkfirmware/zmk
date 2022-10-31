cmake_minimum_required(VERSION 3.13.1)

set(CONFIG_APPLICATION_DEFINED_SYSCALL true)

# Add our custom Zephyr module for drivers w/ syscalls, etc.
list(APPEND DTS_ROOT ${CMAKE_SOURCE_DIR}/drivers/zephyr)

set(ZephyrBuildConfiguration_ROOT ${CMAKE_SOURCE_DIR}/cmake)

list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/module
)

# Find Zephyr. This also loads Zephyr's build system.
find_package(Zephyr REQUIRED HINTS ../zephyr)
project(zmk)

zephyr_linker_sources(RODATA include/linker/zmk-events.ld)

# Add your source file to the "app" target. This must come after
# find_package(Zephyr) which defines the target.
target_include_directories(app PRIVATE include)
target_sources(app PRIVATE src/stdlib.c)
target_sources(app PRIVATE src/activity.c)
target_sources(app PRIVATE src/kscan.c)
target_sources(app PRIVATE src/matrix_transform.c)
target_sources(app PRIVATE src/sensors.c)
target_sources_ifdef(CONFIG_ZMK_WPM app PRIVATE src/wpm.c)
target_sources(app PRIVATE src/event_manager.c)
target_sources_ifdef(CONFIG_ZMK_EXT_POWER app PRIVATE src/ext_power_generic.c)
target_sources(app PRIVATE src/events/activity_state_changed.c)
target_sources(app PRIVATE src/events/position_state_changed.c)
target_sources(app PRIVATE src/events/sensor_event.c)
target_sources_ifdef(CONFIG_ZMK_WPM app PRIVATE src/events/wpm_state_changed.c)
target_sources_ifdef(CONFIG_USB_DEVICE_STACK app PRIVATE src/events/usb_conn_state_changed.c)
target_sources(app PRIVATE src/behaviors/behavior_reset.c)
target_sources_ifdef(CONFIG_ZMK_EXT_POWER app PRIVATE src/behaviors/behavior_ext_power.c)
if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE src/hid.c)
  target_sources(app PRIVATE src/behaviors/behavior_key_press.c)
  target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_KEY_TOGGLE app PRIVATE src/behaviors/behavior_key_toggle.c)
  target_sources(app PRIVATE src/behaviors/behavior_hold_tap.c)
  target_sources(app PRIVATE src/behaviors/behavior_sticky_key.c)
  target_sources(app PRIVATE src/behaviors/behavior_caps_word.c)
  target_sources(app PRIVATE src/behaviors/behavior_key_repeat.c)
  target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_MACRO app PRIVATE src/behaviors/behavior_macro.c)
  target_sources(app PRIVATE src/behaviors/behavior_momentary_layer.c)
  target_sources(app PRIVATE src/behaviors/behavior_mod_morph.c)
  target_sources(app PRIVATE src/behaviors/behavior_outputs.c)
  target_sources(app PRIVATE src/behaviors/behavior_toggle_layer.c)
  target_sources(app PRIVATE src/behaviors/behavior_to_layer.c)
  target_sources(app PRIVATE src/behaviors/behavior_transparent.c)
  target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_TRI_STATE app PRIVATE src/behaviors/behavior_tri_state.c)
  target_sources(app PRIVATE src/behaviors/behavior_none.c)
  target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_SENSOR_ROTATE app PRIVATE src/behaviors/behavior_sensor_rotate.c)
  target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_SENSOR_ROTATE_VAR app PRIVATE src/behaviors/behavior_sensor_rotate_var.c)
  target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_SENSOR_ROTATE_COMMON app PRIVATE src/behaviors/behavior_sensor_rotate_common.c)
  target_sources(app PRIVATE src/combo.c)
  target_sources(app PRIVATE src/behaviors/behavior_tap_dance.c)
  target_sources(app PRIVATE src/behavior_queue.c)
  target_sources(app PRIVATE src/conditional_layer.c)
  target_sources(app PRIVATE src/endpoints.c)
  target_sources(app PRIVATE src/events/endpoint_changed.c)
  target_sources(app PRIVATE src/hid_listener.c)
  target_sources(app PRIVATE src/keymap.c)
  target_sources(app PRIVATE src/events/layer_state_changed.c)
  target_sources(app PRIVATE src/events/modifiers_state_changed.c)
  target_sources(app PRIVATE src/events/keycode_state_changed.c)

  if (CONFIG_ZMK_BLE)
    target_sources(app PRIVATE src/events/ble_active_profile_changed.c)
    target_sources(app PRIVATE src/behaviors/behavior_bt.c)
    target_sources(app PRIVATE src/ble.c)
    target_sources(app PRIVATE src/hog.c)
  endif()
endif()

target_sources_ifdef(CONFIG_ZMK_RGB_UNDERGLOW app PRIVATE src/behaviors/behavior_rgb_underglow.c)
target_sources_ifdef(CONFIG_ZMK_BACKLIGHT app PRIVATE src/behaviors/behavior_backlight.c)

target_sources_ifdef(CONFIG_ZMK_BATTERY_REPORTING app PRIVATE src/events/battery_state_changed.c)
target_sources_ifdef(CONFIG_ZMK_BATTERY_REPORTING app PRIVATE src/battery.c)

target_sources_ifdef(CONFIG_ZMK_SPLIT app PRIVATE src/events/split_peripheral_status_changed.c)
add_subdirectory(src/split)

target_sources_ifdef(CONFIG_USB_DEVICE_STACK app PRIVATE src/usb.c)
target_sources_ifdef(CONFIG_ZMK_USB app PRIVATE src/usb_hid.c)
target_sources_ifdef(CONFIG_ZMK_RGB_UNDERGLOW app PRIVATE src/rgb_underglow.c)
target_sources_ifdef(CONFIG_ZMK_BACKLIGHT app PRIVATE src/backlight.c)
target_sources(app PRIVATE src/workqueue.c)
target_sources(app PRIVATE src/main.c)

add_subdirectory(src/display/)

zephyr_cc_option(-Wfatal-errors)
