# Find Zephyr. This also loads Zephyr's build system.
cmake_minimum_required(VERSION 3.13.1)

list(APPEND BOARD_ROOT ${CMAKE_SOURCE_DIR})

find_package(Zephyr)
project(zmk)

# Add your source file to the "app" target. This must come after
# find_package(Zephyr) which defines the target.
target_sources(app PRIVATE src/kscan.c)
target_sources(app PRIVATE src/main.c)

