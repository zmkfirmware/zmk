# Copyright (c) 2024 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_linker_sources(DATA_SECTIONS ../../include/linker/zmk-rpc-subsystems.ld)
zephyr_linker_sources(SECTIONS ../../include/linker/zmk-rpc-subsystem-handlers.ld)
zephyr_linker_sources(SECTIONS ../../include/linker/zmk-rpc-event-mappers.ld)
zephyr_linker_sources(SECTIONS ../../include/linker/zmk-rpc-transport.ld)

target_sources(app PRIVATE msg_framing.c)
target_sources(app PRIVATE rpc.c)
target_sources(app PRIVATE core.c)
target_sources(app PRIVATE behavior_subsystem.c)
target_sources(app PRIVATE core_subsystem.c)
target_sources_ifdef(CONFIG_ZMK_STUDIO_TRANSPORT_UART app PRIVATE uart_rpc_transport.c)
target_sources_ifdef(CONFIG_ZMK_STUDIO_TRANSPORT_BLE app PRIVATE gatt_rpc_transport.c)