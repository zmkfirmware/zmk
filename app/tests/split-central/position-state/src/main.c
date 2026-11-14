/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

/* Enable the header fields used by central.c, without starting a BT stack. */
#define CONFIG_BT_GATT_AUTO_DISCOVER_CCC 1
#define CONFIG_BT_USER_PHY_UPDATE 1
#define CONFIG_BT_SMP 1

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zmk/event_manager.h>
#include <zmk/split/transport/central.h>

/* Compile the production central, exercising its private notification, cleanup
 * and worker paths with real kernel queues, work and spinlocks. Radio startup
 * and connection unref are replaced; unrelated radio functions are not linked.
 */
#define CONFIG_ZMK_SPLIT_BLE 1
#define CONFIG_ZMK_SPLIT_ROLE_CENTRAL 1
#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS 2
#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE 5
#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_SPLIT_RUN_STACK_SIZE 512
#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_SPLIT_RUN_QUEUE_SIZE 5
#define CONFIG_ZMK_SPLIT_BLE_PREF_INT 6
#define CONFIG_ZMK_SPLIT_BLE_PREF_LATENCY 30
#define CONFIG_ZMK_SPLIT_BLE_PREF_TIMEOUT 400
#define CONFIG_ZMK_BLE_THREAD_PRIORITY 5
#define CONFIG_ZMK_LOG_LEVEL 0

#undef SYS_INIT
#define SYS_INIT(fn, ...) static int (*const unused_init_##fn)(void) __unused = fn
#undef ZMK_SUBSCRIPTION
#define ZMK_SUBSCRIPTION(...)
#undef ZMK_SPLIT_TRANSPORT_CENTRAL_REGISTER
#define ZMK_SPLIT_TRANSPORT_CENTRAL_REGISTER(name, ...)                                            \
    const struct zmk_split_transport_central name

static void test_conn_unref(struct bt_conn *conn) { ARG_UNUSED(conn); }
#define bt_conn_unref test_conn_unref
#include "../../../../src/split/bluetooth/central.c"
#undef bt_conn_unref

/* A position outside the first bitmap byte, after the queue-filling keys. */
#define TEST_POSITION 35
static struct bt_conn *const test_conn = (struct bt_conn *)0x1234;
static struct bt_conn *const second_conn = (struct bt_conn *)0x5678;
static bool held[2][128];
static unsigned int presses[2][128], releases[2][128];
static uint8_t last_snapshot[16];
static bool inject_release;
static bool inject_disconnect;

int zmk_split_transport_central_peripheral_event_handler(
    const struct zmk_split_transport_central *transport, uint8_t source,
    struct zmk_split_transport_peripheral_event ev) {
    zassert_equal(transport, &bt_central);
    zassert_true(source < 2);
    if (ev.type == ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_KEY_POSITION_EVENT) {
        unsigned int pos = ev.data.key_position_event.position;
        zassert_not_equal(held[source][pos], ev.data.key_position_event.pressed,
                          "Duplicate position transition at %d", pos);
        held[source][pos] = ev.data.key_position_event.pressed;
        if (held[source][pos]) {
            presses[source][pos]++;
        } else {
            releases[source][pos]++;
        }
        if (inject_release) {
            inject_release = false;
            uint8_t up[16] = {0};
            split_central_notify_func(test_conn, &peripherals[0].subscribe_params, up, sizeof(up));
        }
        if (inject_disconnect) {
            inject_disconnect = false;
            zassert_ok(release_peripheral_slot(0));
        }
    }
    return 0;
}

K_SEM_DEFINE(worker_blocked, 0, 1);
K_SEM_DEFINE(worker_resume, 0, 1);
static bool paused;
static void block_worker(struct k_work *work) {
    ARG_UNUSED(work);
    k_sem_give(&worker_blocked);
    k_sem_take(&worker_resume, K_FOREVER);
}
K_WORK_DEFINE(blocking_work, block_worker);

static void pause_worker(void) {
    if (!paused) {
        k_work_submit(&blocking_work);
        zassert_ok(k_sem_take(&worker_blocked, K_SECONDS(1)));
        paused = true;
    }
}

static void drain(void) {
    struct k_work_sync sync;
    paused = false;
    k_sem_give(&worker_resume);
    k_work_flush(&peripheral_event_work, &sync);
    pause_worker();
}

static void before(void *fixture) {
    ARG_UNUSED(fixture);
    pause_worker();
    k_msgq_purge(&peripheral_event_msgq);
    memset(peripherals, 0, sizeof(peripherals));
    memset(held, 0, sizeof(held));
    memset(presses, 0, sizeof(presses));
    memset(releases, 0, sizeof(releases));
    memset(last_snapshot, 0, sizeof(last_snapshot));
    inject_release = false;
    inject_disconnect = false;
    peripherals[0].conn = test_conn;
    peripherals[0].state = PERIPHERAL_SLOT_STATE_CONNECTED;
    peripherals[1].conn = second_conn;
    peripherals[1].state = PERIPHERAL_SLOT_STATE_CONNECTED;
}

static void snapshot(const uint8_t *data) {
    zassert_equal(split_central_notify_func(test_conn, &peripherals[0].subscribe_params, data, 16),
                  BT_GATT_ITER_CONTINUE);
    memcpy(last_snapshot, data, sizeof(last_snapshot));
}

static void press_test_position(void) {
    uint8_t data[16] = {0};
    data[TEST_POSITION / 8] = BIT(TEST_POSITION % 8);
    snapshot(data);
    drain();
    zassert_true(held[0][TEST_POSITION], "Precondition: test-position press reached downstream");
}

static void fill_queue(void) {
    /* Five real key changes fill the production queue while work is blocked. */
    uint8_t data[16];
    memcpy(data, last_snapshot, sizeof(data));
    data[0] ^= 0x1f;
    snapshot(data);
    zassert_equal(k_msgq_num_used_get(&peripheral_event_msgq), 5);
}

ZTEST(split_central, test_full_queue_release_without_another_notification) {
    press_test_position();
    fill_queue();
    uint8_t up[16] = {0};
    snapshot(up);
    drain();
    zassert_false(held[0][TEST_POSITION], "Full queue discarded test-position release");
    zassert_equal(releases[0][TEST_POSITION], 1);
}

ZTEST(split_central, test_identical_snapshots_and_later_other_key_change) {
    press_test_position();
    fill_queue();
    uint8_t data[16] = {0};
    snapshot(data);
    snapshot(data);
    data[0] = BIT(3);
    snapshot(data);
    drain();
    zassert_false(held[0][TEST_POSITION],
                  "Identical/later snapshots forgot pending test-position release");
    zassert_true(held[0][3]);
    zassert_equal(releases[0][TEST_POSITION], 1);
    snapshot(data);
    drain();
    zassert_equal(presses[0][3], 1, "Identical snapshot duplicated an edge");
    data[0] = 0;
    snapshot(data);
    drain();
    zassert_false(held[0][3]);
}

ZTEST(split_central, test_disconnect_releases_delivered_keys_with_full_queue) {
    press_test_position();
    fill_queue();
    zassert_ok(release_peripheral_slot(0));
    drain();
    zassert_false(held[0][TEST_POSITION], "Full queue discarded disconnect release");
    zassert_equal(releases[0][TEST_POSITION], 1);
}

ZTEST(split_central, test_disconnect_after_release_was_dropped) {
    press_test_position();
    fill_queue();
    uint8_t up[16] = {0};
    snapshot(up);
    zassert_ok(release_peripheral_slot(0));
    drain();
    zassert_false(held[0][TEST_POSITION],
                  "Disconnect forgot release after received bitmap advanced");
}

ZTEST(split_central, test_queued_press_then_disconnect_is_released) {
    uint8_t data[16] = {0};
    data[TEST_POSITION / 8] = BIT(TEST_POSITION % 8);
    snapshot(data);
    zassert_ok(release_peripheral_slot(0));
    drain();
    zassert_false(held[0][TEST_POSITION], "Queued press survived disconnect");
}

ZTEST(split_central, test_normal_press_release_before_worker_preserves_both_edges) {
    uint8_t data[16] = {0};
    data[TEST_POSITION / 8] = BIT(TEST_POSITION % 8);
    snapshot(data);
    data[TEST_POSITION / 8] = 0;
    snapshot(data);
    drain();
    zassert_equal(presses[0][TEST_POSITION], 1, "Normal short tap was coalesced away");
    zassert_equal(releases[0][TEST_POSITION], 1);
    zassert_false(held[0][TEST_POSITION]);
}

ZTEST(split_central, test_many_positions_exceed_queue_and_disconnect) {
    uint8_t data[16] = {0};
    memset(data, 0xff, sizeof(data));
    snapshot(data);
    drain();
    for (int i = 0; i < 128; i++) {
        zassert_true(held[0][i], "Position %d lost from full bitmap", i);
        zassert_equal(presses[0][i], 1);
    }
    fill_queue();
    zassert_ok(release_peripheral_slot(0));
    drain();
    for (int i = 0; i < 128; i++) {
        zassert_false(held[0][i], "Disconnect left position %d held", i);
        zassert_equal(releases[0][i], 1);
    }
}

ZTEST(split_central, test_notification_during_dispatch_is_reconciled) {
    uint8_t data[16] = {0};
    data[TEST_POSITION / 8] = BIT(TEST_POSITION % 8);
    snapshot(data);
    inject_release = true;
    drain();
    zassert_equal(presses[0][TEST_POSITION], 1);
    zassert_equal(releases[0][TEST_POSITION], 1);
    zassert_false(held[0][TEST_POSITION]);
}

ZTEST(split_central, test_disconnect_during_dispatch_is_reconciled) {
    uint8_t data[16] = {0};
    data[TEST_POSITION / 8] = BIT(TEST_POSITION % 8);
    snapshot(data);
    inject_disconnect = true;
    drain();
    zassert_false(held[0][TEST_POSITION]);
    zassert_equal(releases[0][TEST_POSITION], 1);
}

ZTEST(split_central, test_reconnect_cannot_erase_pending_disconnect_release) {
    press_test_position();
    fill_queue();
    zassert_ok(release_peripheral_slot(0));
    /* Reuse the slot before its pending cleanup can run. */
    peripherals[0].conn = test_conn;
    peripherals[0].state = PERIPHERAL_SLOT_STATE_CONNECTED;
    uint8_t data[16] = {0};
    data[TEST_POSITION / 8] = BIT(TEST_POSITION % 8);
    snapshot(data);
    drain();
    zassert_equal(releases[0][TEST_POSITION], 1, "New snapshot erased disconnect all-up boundary");
    zassert_equal(presses[0][TEST_POSITION], 2);
    zassert_true(held[0][TEST_POSITION]);
}

ZTEST(split_central, test_both_peripherals_converge_through_shared_full_queue) {
    uint8_t down[16], up[16] = {0};
    memset(down, 0xff, sizeof(down));
    snapshot(down);
    split_central_notify_func(second_conn, &peripherals[1].subscribe_params, down, sizeof(down));
    drain();
    for (int source = 0; source < 2; source++) {
        for (int pos = 0; pos < 128; pos++) {
            zassert_true(held[source][pos]);
            zassert_equal(presses[source][pos], 1);
        }
    }
    fill_queue();
    zassert_ok(release_peripheral_slot(0));
    split_central_notify_func(second_conn, &peripherals[1].subscribe_params, up, sizeof(up));
    drain();
    for (int source = 0; source < 2; source++) {
        for (int pos = 0; pos < 128; pos++) {
            zassert_false(held[source][pos]);
            zassert_equal(releases[source][pos], 1);
        }
    }
}

ZTEST_SUITE(split_central, NULL, NULL, before, NULL, NULL);
