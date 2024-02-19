/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "msg_framing.h"

bool studio_framing_process_byte(enum studio_framing_state *rpc_framing_state, uint8_t c) {
    switch (*rpc_framing_state) {
    case FRAMING_STATE_ERR:
        switch (c) {
        case FRAMING_EOF:
            *rpc_framing_state = FRAMING_STATE_IDLE;
            return false;
        case FRAMING_SOF:
            *rpc_framing_state = FRAMING_STATE_AWAITING_DATA;
            return false;
        default:
            LOG_WRN("Discarding unexpected data 0x%02x", c);
            return false;
        }

        return false;
    case FRAMING_STATE_IDLE:
    case FRAMING_STATE_EOF:
        switch (c) {
        case FRAMING_SOF:
            *rpc_framing_state = FRAMING_STATE_AWAITING_DATA;
            return false;
        default:
            LOG_WRN("Expected SOF, got 0x%02x", c);
            return false;
        }
        return false;
    case FRAMING_STATE_AWAITING_DATA:
        switch (c) {
        case FRAMING_SOF:
            LOG_WRN("Unescaped SOF mid-data");
            *rpc_framing_state = FRAMING_STATE_ERR;
            return false;
        case FRAMING_ESC:
            *rpc_framing_state = FRAMING_STATE_ESCAPED;
            return false;
        case FRAMING_EOF:
            *rpc_framing_state = FRAMING_STATE_EOF;
            return false;
        default:
            return true;
        }

        break;
    case FRAMING_STATE_ESCAPED:
        *rpc_framing_state = FRAMING_STATE_AWAITING_DATA;
        return true;
    }

    return false;
}