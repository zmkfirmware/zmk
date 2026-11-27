# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(studio_transport_regression)

# Compile against the schemas and nanopb options used by the tested firmware.
if(NOT EXISTS "${STUDIO_BUILD}/proto/zmk/studio.pb.c")
  message(FATAL_ERROR "Set STUDIO_BUILD to a configured ZMK Studio firmware build")
endif()
get_filename_component(zmk "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
target_include_directories(app PRIVATE ${zmk}/include ${zmk}/src/studio ${STUDIO_BUILD})
target_compile_definitions(app PRIVATE CONFIG_ZMK_STUDIO_RPC_RX_BUF_SIZE=96
  CONFIG_ZMK_STUDIO_RPC_TX_BUF_SIZE=8 CONFIG_ZMK_STUDIO_RPC_THREAD_STACK_SIZE=4096
  CONFIG_ZMK_STUDIO_TRANSPORT_BLE=1 CONFIG_ZMK_STUDIO_TRANSPORT_BLE_PREF_LATENCY=99
  CONFIG_ZMK_STUDIO_LOG_LEVEL=0 CONFIG_ZMK_LOG_LEVEL=0)
foreach(schema studio core keymap behaviors lighting meta)
  target_sources(app PRIVATE ${STUDIO_BUILD}/proto/zmk/${schema}.pb.c)
endforeach()
zephyr_linker_sources(RODATA ${zmk}/include/linker/zmk-events.ld)
zephyr_linker_sources(DATA_SECTIONS ${zmk}/include/linker/zmk-rpc-subsystems.ld)
foreach(section event-mappers subsystem-handlers transport)
  zephyr_linker_sources(SECTIONS ${zmk}/include/linker/zmk-rpc-${section}.ld)
endforeach()
target_sources(app PRIVATE test.c ${zmk}/src/event_manager.c
  ${zmk}/src/events/endpoint_changed.c ${zmk}/src/events/led_matrix_state_changed.c
  ${zmk}/src/led_matrix/studio_notification.c ${zmk}/src/studio/core.c
  ${zmk}/src/studio/rpc.c ${zmk}/src/studio/rpc_tx.c ${zmk}/src/studio/msg_framing.c
  ${zmk}/src/studio/uart_rpc_transport.c ${zmk}/src/studio/gatt_rpc_transport.c)
foreach(symbol bt_conn_unref bt_gatt_indicate bt_gatt_get_mtu bt_gatt_is_subscribed)
  zephyr_ld_options(-Wl,--wrap=${symbol})
endforeach()
