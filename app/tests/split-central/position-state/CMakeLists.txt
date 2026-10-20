# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(split_central_position_state)

target_include_directories(app PRIVATE ../../../include)
target_sources(app PRIVATE src/main.c)
