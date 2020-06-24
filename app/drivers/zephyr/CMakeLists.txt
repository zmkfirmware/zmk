if(CONFIG_ZMK_KSCAN_GPIO_DRIVER)
  # Add hello_world_driver.h to the set of global include paths.
  zephyr_include_directories(.)

  zephyr_library()
  zephyr_library_sources(
    kscan_gpio_matrix.c
    kscan_gpio_direct.c
    )
endif()
