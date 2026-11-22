/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/bluetooth/addr.h>
#include <zmk/behavior.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE)

#include <zmk/ble.h>
#define BLE_PERIPHERAL_COUNT ZMK_SPLIT_BLE_PERIPHERAL_COUNT

#else

#define BLE_PERIPHERAL_COUNT 0

#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED)
#define WIRED_PERIPHERAL_COUNT 1
#else
#define WIRED_PERIPHERAL_COUNT 0
#endif

#define ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT MAX(BLE_PERIPHERAL_COUNT, WIRED_PERIPHERAL_COUNT)

int zmk_split_central_invoke_behavior(uint8_t source, struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event, bool state);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_STATE_SYNC)

#include <zmk/split/transport/types.h>

int zmk_split_central_get_available_source_ids(uint8_t *source_ids);
int zmk_split_central_send_command(uint8_t source, struct zmk_split_transport_central_command cmd);

/**
 * @brief Mark all enabled central state domains as changed for a peripheral, so the full state is
 * sent to it again. Call when the peripheral becomes reachable.
 */
void zmk_split_central_state_sync_resync(uint8_t source);

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_STATE_SYNC)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)

int zmk_split_central_get_peripheral_battery_level(uint8_t source, uint8_t *level);

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
