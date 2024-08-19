/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>

enum studio_framing_state {
    FRAMING_STATE_IDLE,
    FRAMING_STATE_AWAITING_DATA,
    FRAMING_STATE_ESCAPED,
    FRAMING_STATE_ERR,
    FRAMING_STATE_EOF,
};

#define FRAMING_SOF 0xAB
#define FRAMING_ESC 0xAC
#define FRAMING_EOF 0xAD

/**
 * @brief Process an incoming byte from a frame. Will possibly update the framing state depending on
 * what data is received.
 * @retval true if data is a non-framing byte, and is real data to be handled by the upper level
 * logic.
 * @retval false if data is a framing byte, and should be ignored. Also indicates the framing state
 * has been updated.
 */
bool studio_framing_process_byte(enum studio_framing_state *frame_state, uint8_t data);
