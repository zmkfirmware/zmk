if(CONFIG_ZMK_KSCAN_GPIO_DRIVER)
  zephyr_include_directories(.)

  zephyr_library()
  zephyr_library_sources(
    kscan_gpio_matrix.c
    kscan_gpio_direct.c
    )

  zephyr_library_sources_ifdef(CONFIG_EN11 en11.c)
  zephyr_library_sources_ifdef(CONFIG_EN11_TRIGGER en11_trigger.c)
endif()
