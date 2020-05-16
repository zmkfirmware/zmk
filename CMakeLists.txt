# Find Zephyr. This also loads Zephyr's build system.
cmake_minimum_required(VERSION 3.13.1)

list(APPEND BOARD_ROOT ${CMAKE_SOURCE_DIR})
list(APPEND DTS_ROOT ${CMAKE_SOURCE_DIR})

include(cmake/keymap.cmake)

find_package(Zephyr)
project(zmk)

# Add your source file to the "app" target. This must come after
# find_package(Zephyr) which defines the target.
target_include_directories(app PRIVATE include)
target_sources(app PRIVATE src/kscan.c)
target_sources(app PRIVATE src/keymap.c)
target_sources(app PRIVATE src/hid.c)
target_sources(app PRIVATE src/ble.c)
target_sources(app PRIVATE src/usb_hid.c)
target_sources(app PRIVATE src/hog.c)
target_sources(app PRIVATE src/endpoints.c)
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/handlers.c)

