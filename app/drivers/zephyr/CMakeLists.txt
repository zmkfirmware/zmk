if(CONFIG_ZMK_KSCAN_GPIO_DRIVER)
  zephyr_include_directories(.)

  zephyr_library()
  zephyr_library_sources(
    kscan_gpio_matrix.c
    kscan_gpio_direct.c
    kscan_gpio_demux.c
    )

  zephyr_library_sources_ifdef(CONFIG_EC11 ec11.c)
  zephyr_library_sources_ifdef(CONFIG_EC11_TRIGGER ec11_trigger.c)
  zephyr_library_sources_ifdef(CONFIG_ZMK_BATTERY_VOLTAGE_DIVIDER battery_voltage_divider.c)
endif()
