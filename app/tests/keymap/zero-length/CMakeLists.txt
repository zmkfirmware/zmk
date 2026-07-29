# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)

set(ZMK_APP_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
set(DTS_ROOT ${ZMK_APP_DIR})
set(ZEPHYR_EXTRA_MODULES ${ZMK_APP_DIR}/module)
set(SYSCALL_INCLUDE_DIRS ${ZMK_APP_DIR}/include)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(keymap_zero_length)

zephyr_syscall_header(${ZMK_APP_DIR}/include/drivers/behavior.h)

target_include_directories(app PRIVATE ${ZMK_APP_DIR}/include)
target_compile_definitions(app PRIVATE CONFIG_ZMK_LOG_LEVEL=0)
target_sources(app PRIVATE src/main.c ${ZMK_APP_DIR}/src/keymap.c)
