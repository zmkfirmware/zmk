# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(studio_gatt_rpc_rx)

target_include_directories(app PRIVATE ../../../include ../../../src/studio)
target_sources(app PRIVATE src/main.c ../../../src/studio/gatt_rpc_transport.c)
set_source_files_properties(
  ../../../src/studio/gatt_rpc_transport.c
  PROPERTIES COMPILE_DEFINITIONS ZMK_STUDIO_GATT_RPC_RX_TEST
)
