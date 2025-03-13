zephyr_library()
zephyr_library_sources(led_widgets.c)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/../include)