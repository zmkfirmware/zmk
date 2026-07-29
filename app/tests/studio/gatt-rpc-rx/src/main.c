/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/ztest.h>

#include "gatt_rpc_transport.h"

static uint8_t ring_data[8];
static struct ring_buf ring;
static const uint8_t payload[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
static uint32_t notify_count;
static uint32_t ring_size_at_notify;

struct ring_buf *zmk_rpc_get_rx_buf(void) { return &ring; }

void zmk_rpc_rx_notify(void) {
    ring_size_at_notify = ring_buf_size_get(&ring);
    notify_count++;
}

static void before(void *fixture) {
    ARG_UNUSED(fixture);

    ring_buf_init(&ring, sizeof(ring_data), ring_data);
    notify_count = 0;
    ring_size_at_notify = 0;
    zmk_studio_gatt_rpc_rx_set_active_for_test(true);
}

ZTEST(studio_gatt_rx, test_exact_free_space_is_accepted) {
    zassert_equal(zmk_studio_rpc_rx_write(&ring, payload, 8), 8);
    zassert_equal(ring_buf_size_get(&ring), 8);
}

ZTEST(studio_gatt_rx, test_free_plus_one_is_rejected_without_partial_enqueue) {
    zassert_equal(zmk_studio_rpc_rx_write(&ring, payload, 9), -ENOMEM);
    zassert_equal(ring_buf_size_get(&ring), 0);
}

ZTEST(studio_gatt_rx, test_full_ring_rejects_in_bounded_time) {
    ring_buf_put(&ring, payload, 8);
    zassert_equal(zmk_studio_rpc_rx_write(&ring, payload, 1), -ENOMEM);
    zassert_equal(ring_buf_size_get(&ring), 8);
}

ZTEST(studio_gatt_rx, test_gatt_write_rejects_nonzero_offset_without_enqueue_or_notify) {
    zassert_equal(zmk_studio_gatt_rpc_rx_write_for_test(payload, 1, 1),
                  BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET));
    zassert_equal(ring_buf_size_get(&ring), 0);
    zassert_equal(notify_count, 0);
}

ZTEST(studio_gatt_rx, test_gatt_write_maps_oversize_to_att_error_without_enqueue_or_notify) {
    zassert_equal(zmk_studio_gatt_rpc_rx_write_for_test(payload, 9, 0),
                  BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES));
    zassert_equal(ring_buf_size_get(&ring), 0);
    zassert_equal(notify_count, 0);
}

ZTEST(studio_gatt_rx, test_gatt_write_notifies_once_after_full_enqueue) {
    zassert_equal(zmk_studio_gatt_rpc_rx_write_for_test(payload, 8, 0), 8);
    zassert_equal(ring_buf_size_get(&ring), 8);
    zassert_equal(notify_count, 1);
    zassert_equal(ring_size_at_notify, 8);
}

ZTEST_SUITE(studio_gatt_rx, NULL, NULL, before, NULL, NULL);
