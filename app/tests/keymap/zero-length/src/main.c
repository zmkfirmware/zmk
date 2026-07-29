/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zmk/keymap.h>

ZTEST(zero_keymap, test_position_event_is_rejected) {
    zassert_equal(zmk_keymap_position_state_changed(0, 0, true, k_uptime_get()), -ENOTSUP);
}

ZTEST_SUITE(zero_keymap, NULL, NULL, NULL, NULL, NULL);
