# Find Zephyr. This also loads Zephyr's build system.
cmake_minimum_required(VERSION 3.13.1)

list(APPEND BOARD_ROOT .)

get_filename_component(ZEPHYR_RUST ${CMAKE_CURRENT_SOURCE_DIR}/zephyr-rust ABSOLUTE)
list(APPEND ZEPHYR_EXTRA_MODULES ${ZEPHYR_RUST})

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
find_package(Zephyr)

project(zmk)

# Add your source file to the "app" target. This must come after
# find_package(Zephyr) which defines the target.
target_sources(app PRIVATE src/main.c)

