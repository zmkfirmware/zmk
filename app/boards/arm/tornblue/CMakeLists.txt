zephyr_library()
zephyr_library_sources(led_driver.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
