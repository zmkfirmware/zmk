cmake_minimum_required(VERSION 3.20.0)

list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(matrix_transform)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/matrix_transform.c)
target_sources(app PRIVATE src/main.c)
