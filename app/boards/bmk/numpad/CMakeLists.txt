if(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_sources(
    custom_status_screen.c
    display/status_screen.c
  )

  if(CONFIG_ZMK_WIDGET_LED_MATRIX_EFFECT_STATUS)
    zephyr_library_sources(display/widgets/led_matrix_effect_status.c)
  endif()

  if(CONFIG_ZMK_HID_INDICATORS)
    zephyr_library_sources(display/widgets/hid_indicators_status.c)
  endif()
endif()

# The no-op behavior remains available when disabled so the default keymap still builds.
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
zephyr_library_sources(calculator/behavior.c)
if(CONFIG_NUMPAD_CALCULATOR)
  zephyr_library_sources(calculator/engine.c calculator/calculator.c
                         calculator/sender.c calculator/widget.c)
endif()
