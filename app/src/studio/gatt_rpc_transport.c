/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <sys/types.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/ring_buffer.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include "rpc_tx.h"

#include "uuid.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk_studio, CONFIG_ZMK_STUDIO_LOG_LEVEL);

static bool handling_rx = false;

static K_SEM_DEFINE(indicate_sem, 1, 1);
static uint32_t indicate_ticket;

static void rpc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    ARG_UNUSED(attr);

    bool notif_enabled = (value == BT_GATT_CCC_INDICATE);

    LOG_INF("RPC Notifications %s", notif_enabled ? "enabled" : "disabled");

#if CONFIG_ZMK_STUDIO_TRANSPORT_BLE_PREF_LATENCY < CONFIG_BT_PERIPHERAL_PREF_LATENCY
    struct bt_conn *conn = zmk_ble_active_profile_conn();
    if (conn) {
        uint8_t latency = notif_enabled ? CONFIG_ZMK_STUDIO_TRANSPORT_BLE_PREF_LATENCY
                                        : CONFIG_BT_PERIPHERAL_PREF_LATENCY;

        int ret = bt_conn_le_param_update(
            conn,
            BT_LE_CONN_PARAM(CONFIG_BT_PERIPHERAL_PREF_MIN_INT, CONFIG_BT_PERIPHERAL_PREF_MAX_INT,
                             latency, CONFIG_BT_PERIPHERAL_PREF_TIMEOUT));
        if (ret < 0) {
            LOG_WRN("Failed to request lower latency while studio is active (%d)", ret);
        }

        bt_conn_unref(conn);
    }
#endif
}

static ssize_t read_rpc_resp(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                             uint16_t len, uint16_t offset) {

    LOG_DBG("Read response for length %d at offset %d", len, offset);
    return 0;
}

static ssize_t write_rpc_req(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
                             uint16_t len, uint16_t offset, uint8_t flags) {
    if (!handling_rx) {
        return len;
    }

    if (zmk_rpc_rx_put(ZMK_TRANSPORT_BLE, buf, len) < 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
    }
    return len;
}

BT_GATT_SERVICE_DEFINE(
    rpc_interface, BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(ZMK_STUDIO_BT_SERVICE_UUID)),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(ZMK_STUDIO_BT_RPC_CHRC_UUID),
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, read_rpc_resp,
                           write_rpc_req, NULL),
    BT_GATT_CCC(rpc_ccc_cfg_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT));

static int gatt_start_rx(void) {
    handling_rx = true;
    return 0;
}

static int gatt_stop_rx(void) {
    handling_rx = false;
    return 0;
}

static uint8_t indicate_buffer[27];

static K_SEM_DEFINE(tx_ready, 0, 1);

static void gatt_tx_start(void) { k_sem_give(&tx_ready); }

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err) {
    studio_tx_complete(indicate_ticket, err ? -EIO : (int)params->len);
}

static void indicate_destroy(struct bt_gatt_indicate_params *params) {
    k_sem_give(&indicate_sem);
    gatt_tx_start();
}

static struct bt_gatt_indicate_params rpc_indicate_params = {
    .attr = &rpc_interface.attrs[1],
    .data = indicate_buffer,
    .func = indicate_cb,
    .destroy = indicate_destroy,
};

static void send_indication(void) {
    if (k_sem_take(&indicate_sem, K_NO_WAIT) < 0) {
        return;
    }
    uint32_t expected_session = studio_tx_session();
    struct bt_conn *conn = zmk_ble_active_profile_conn();
    size_t limit = conn ? MIN(sizeof(indicate_buffer), bt_gatt_get_mtu(conn) - 3U) : 1;
    size_t len = studio_tx_peek(ZMK_TRANSPORT_BLE, indicate_buffer, limit, &indicate_ticket);
    // If the active profile changed during lookup, its kick is pending in tx_ready.
    if (studio_tx_session() != expected_session) {
        len = 0;
    }
    int err = -ENOTCONN;
    if (len && conn &&
        bt_gatt_is_subscribed(conn, rpc_indicate_params.attr, BT_GATT_CCC_INDICATE)) {
        rpc_indicate_params.len = len;
        err = bt_gatt_indicate(conn, &rpc_indicate_params);
    }
    if (conn) {
        bt_conn_unref(conn);
    }
    if (err) {
        if (len) {
            studio_tx_complete(indicate_ticket, err);
        }
        k_sem_give(&indicate_sem);
    }
}

// ATT allocation may wait. A slow peer must never occupy the system workqueue.
static void gatt_tx_main(void *a, void *b, void *c) {
    for (;;) {
        k_sem_take(&tx_ready, K_FOREVER);
        send_indication();
    }
}

K_THREAD_DEFINE(studio_gatt_tx_thread, CONFIG_ZMK_STUDIO_RPC_THREAD_STACK_SIZE, gatt_tx_main, NULL,
                NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

ZMK_RPC_TRANSPORT(gatt, ZMK_TRANSPORT_BLE, gatt_start_rx, gatt_stop_rx, gatt_tx_start);

#if IS_ENABLED(CONFIG_ZMK_STUDIO_LOCK_ON_DISCONNECT)
static int gatt_rpc_listener(const zmk_event_t *eh) {
    struct bt_conn *conn = zmk_ble_active_profile_conn();

    if (!conn) {
        zmk_studio_core_lock();
    } else {
        bt_conn_unref(conn);
    }

    return 0;
}

ZMK_LISTENER(gatt_rpc_listener, gatt_rpc_listener);
ZMK_SUBSCRIPTION(gatt_rpc_listener, zmk_ble_active_profile_changed);
#endif
