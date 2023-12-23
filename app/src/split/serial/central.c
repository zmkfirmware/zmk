/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zmk/split/central.h>
#include <zmk/split/serial/serial.h>
#include <zmk/events/position_state_changed.h>

// TODO TODO TODO
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(slicemk);

#define POSITION_STATE_DATA_LEN 16
static uint8_t position_state[POSITION_STATE_DATA_LEN];
static uint8_t changed_positions[POSITION_STATE_DATA_LEN];

static void serial_handle_bitmap(uint8_t *data, uint8_t len) {
    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {
        changed_positions[i] = ((uint8_t *)data)[i] ^ position_state[i];
        position_state[i] = ((uint8_t *)data)[i];
        LOG_DBG("TODO TODO TODO data: %d", position_state[i]);
    }

    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {
        for (int j = 0; j < 8; j++) {
            if (changed_positions[i] & BIT(j)) {
                uint32_t position = (i * 8) + j;
                bool pressed = position_state[i] & BIT(j);

                // TODO TODO TODO does zero make sense? check ble central. what
                // slot is central itself?
                int slot = 0;

                struct zmk_position_state_changed ev = {.source = slot,
                                                        .position = position,
                                                        .state = pressed,
                                                        .timestamp = k_uptime_get()};
                zmk_position_state_change_handle(&ev);
            }
        }
    }
}

void serial_handle_rx(uint32_t cmd, uint8_t *data, uint8_t len) {
    switch (cmd) {
    // Handle split bitmap transformed (sbt) version 0.
    case 0x73627400:
        serial_handle_bitmap(data, len);
        break;

    default:
        LOG_ERR("Received unexpected UART command 0x%08x", cmd);
        break;
    }
}
