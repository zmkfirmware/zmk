/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <cobs.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/crc.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <zmk/sensors.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "private.h"

/* Let's provide enough space for multiple messages to reduce the risk of
 * having to drop new ones. */
RING_BUF_DECLARE(zmk_split_serial_rx_ringbuf, MAX_MESSAGE_LEN * 2);

static void on_rx_done(struct net_buf_simple *const buf) {
    static uint8_t positions[POSITION_STATE_DATA_LEN];
    const int64_t timestamp = k_uptime_get();

    if (buf->len < 3) {
        LOG_ERR("Message is smaller than it's header");
        return;
    }

    const uint16_t crc_received = net_buf_simple_remove_le16(buf);
    const uint16_t crc_calculated = crc16_ccitt(0, buf->data, buf->len);
    if (crc_received != crc_calculated) {
        LOG_ERR("Invalid checksum. received=%04X calculated=%04x", crc_received, crc_calculated);
        return;
    }

    const uint8_t event_type = net_buf_simple_pull_u8(buf);
    switch (event_type) {
    case SPLIT_EVENT_POSITION: {
        const uint8_t *const new_positions = buf->data;
        const size_t new_positions_len = buf->len;

        if (new_positions_len > ARRAY_SIZE(positions)) {
            LOG_ERR("Got %zu positions but we only support %zu", new_positions_len,
                    ARRAY_SIZE(positions));
            return;
        }

        for (size_t positions_index = 0; positions_index < new_positions_len;
             positions_index += 1) {

            const uint8_t state = new_positions[positions_index];
            const uint8_t changed = state ^ positions[positions_index];
            positions[positions_index] = state;

            for (size_t bit_index = 0; bit_index < 8; bit_index += 1) {
                if (changed & BIT(bit_index)) {
                    const struct zmk_position_state_changed ev = {
                        .source = 0,
                        .position = positions_index * 8 + bit_index,
                        .state = state & BIT(bit_index),
                        .timestamp = timestamp,
                    };
                    LOG_DBG("Trigger key position state change for %d", ev.position);

                    ZMK_EVENT_RAISE(new_zmk_position_state_changed(ev));
                }
            }
        }
        break;
    }
    default:
        LOG_ERR("Unsupported event type: %02X", event_type);
        break;
    }
}

static void rx_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, MAX_MESSAGE_LEN);
    static struct cobs_decode cobs_decode;

    for (;;) {
        uint8_t encoded_byte;
        const size_t num_read =
            ring_buf_get(&zmk_split_serial_rx_ringbuf, &encoded_byte, sizeof(encoded_byte));
        if (num_read == 0) {
            /* No data, we're done here. */
            return;
        }
        __ASSERT_NO_MSG(num_read == 1);

        uint8_t decoded_byte = 0x00;
        bool decoded_byte_available = false;
        enum cobs_decode_result decode_result =
            cobs_decode_stream(&cobs_decode, encoded_byte, &decoded_byte, &decoded_byte_available);

        if (decoded_byte_available) {
            if (net_buf_simple_tailroom(&rx_buf) == 0) {
                LOG_DBG("message is too big");
                net_buf_simple_reset(&rx_buf);
                cobs_decode_reset(&cobs_decode);
                continue;
            }

            net_buf_simple_add_u8(&rx_buf, decoded_byte);
        }

        switch (decode_result) {
        case COBS_DECODE_RESULT_CONSUMED:
            break;

        case COBS_DECODE_RESULT_FINISHED: {
            cobs_decode_reset(&cobs_decode);
            on_rx_done(&rx_buf);
            net_buf_simple_reset(&rx_buf);
            break;
        }

        case COBS_DECODE_RESULT_UNEXPECTED_ZERO:
            LOG_DBG("unexpected zero in COBS data");
            net_buf_simple_reset(&rx_buf);
            cobs_decode_reset(&cobs_decode);
            break;

        case COBS_DECODE_RESULT_ERROR:
            LOG_DBG("COBS error");
            net_buf_simple_reset(&rx_buf);
            cobs_decode_reset(&cobs_decode);
            break;
        }
    }
}
K_WORK_DEFINE(zmk_split_serial_rx_work, rx_work_handler);
