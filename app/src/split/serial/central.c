/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/split/common.h>
#include <zmk/split/serial/common.h>
#include <init.h>

#include <sys/crc.h>
#include <device.h>
#include <drivers/uart.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/matrix.h>

K_MSGQ_DEFINE(peripheral_event_msgq, sizeof(struct zmk_position_state_changed),
              CONFIG_ZMK_SPLIT_SERIAL_THREAD_QUEUE_SIZE, 4);

static void peripheral_event_work_callback(struct k_work *work) {
    struct zmk_position_state_changed ev;
    while (k_msgq_get(&peripheral_event_msgq, &ev, K_NO_WAIT) == 0) {
        LOG_DBG("Trigger key position state change for %d", ev.position);
        ZMK_EVENT_RAISE(new_zmk_position_state_changed(ev));
    }
}

K_WORK_DEFINE(peripheral_event_work, peripheral_event_work_callback);

static int split_central_notify_func(const uint8_t *data, size_t length) {
    static uint8_t position_state[SPLIT_DATA_LEN];
    uint8_t changed_positions[SPLIT_DATA_LEN];
    const split_data_t *split_data = (const split_data_t *)data;
    uint16_t crc;

    LOG_DBG("[NOTIFICATION] data %p type:%u CRC:%u", data, split_data->type, split_data->crc);

    crc = crc16_ansi(split_data->data, sizeof(split_data->data));
    if (crc != split_data->crc) {
        LOG_WRN("CRC mismatch (%x:%x), skipping data", crc, split_data->crc);
        return 0;
    }

    for (int i = 0; i < SPLIT_DATA_LEN; i++) {
        changed_positions[i] = split_data->data[i] ^ position_state[i];
        position_state[i] = split_data->data[i];
    }

    for (int i = 0; i < SPLIT_DATA_LEN; i++) {
        for (int j = 0; j < 8; j++) {
            if (changed_positions[i] & BIT(j)) {
                uint32_t position = (i * 8) + j;
                bool pressed = position_state[i] & BIT(j);
                struct zmk_position_state_changed ev = {
                    .position = position, .state = pressed, .timestamp = k_uptime_get()};

                if (position > ZMK_KEYMAP_LEN) {
                    LOG_WRN("Invalid position: %u", position);
                    continue;
                }

                k_msgq_put(&peripheral_event_msgq, &ev, K_NO_WAIT);
                k_work_submit(&peripheral_event_work);
            }
        }
    }

    return 0;
}

static int split_serial_central_init(const struct device *dev) {
    split_serial_async_init(split_central_notify_func);
    return 0;
}

SYS_INIT(split_serial_central_init, APPLICATION, CONFIG_ZMK_USB_INIT_PRIORITY);
