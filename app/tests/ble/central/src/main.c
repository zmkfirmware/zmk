/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_central, 4);

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>

#ifdef CONFIG_ARCH_POSIX

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "cmdline.h"
#include "soc.h"

static bool disconnect_and_reconnect = false;
static bool clear_bond_on_disconnect = false;
static bool halt_after_bonding = false;
static bool read_hid_report_on_connect = false;
static bool skip_set_security_on_connect = false;
static bool skip_discovery_on_connect = false;
static bool read_directly_on_discovery = false;
static int32_t wait_on_start = 0;

static void ble_central_native_posix_options(void) {
    static struct args_struct_t options[] = {
        {.is_switch = true,
         .option = "disconnect_and_reconnect",
         .type = 'b',
         .dest = (void *)&disconnect_and_reconnect,
         .descript = "Disconnect and reconnect after the initial connection"},
        {.is_switch = true,
         .option = "halt_after_bonding",
         .type = 'b',
         .dest = (void *)&halt_after_bonding,
         .descript = "Halt any further logic after bonding the first time"},
        {.is_switch = true,
         .option = "clear_bond_on_disconnect",
         .type = 'b',
         .dest = (void *)&clear_bond_on_disconnect,
         .descript = "Clear bonds on disconnect and reconnect"},
        {.is_switch = true,
         .option = "skip_set_security_on_connect",
         .type = 'b',
         .dest = (void *)&skip_set_security_on_connect,
         .descript = "Skip set security level after connecting"},
        {.is_switch = true,
         .option = "read_hid_report_on_connect",
         .type = 'b',
         .dest = (void *)&read_hid_report_on_connect,
         .descript = "Read the peripheral HID report after connecting"},
        {.is_switch = true,
         .option = "skip_discovery_on_connect",
         .type = 'b',
         .dest = (void *)&skip_discovery_on_connect,
         .descript = "Skip GATT characteristic discovery after connecting"},
        {.is_switch = true,
         .option = "read_directly_on_discovery",
         .type = 'b',
         .dest = (void *)&read_directly_on_discovery,
         .descript = "Read HIDS report after GATT characteristic discovery"},
        {.option = "wait_on_start",
         .name = "milliseconds",
         .type = 'u',
         .dest = (void *)&wait_on_start,
         .descript = "Time in milliseconds to wait before starting the test process"},
        ARG_TABLE_ENDMARKER};

    native_add_command_line_opts(options);
}

NATIVE_TASK(ble_central_native_posix_options, PRE_BOOT_1, 1);

#endif

static void start_scan(void);

static struct bt_conn *default_conn;

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                           const void *data, uint16_t length) {
    if (!data) {
        LOG_DBG("[UNSUBSCRIBED]");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    LOG_HEXDUMP_DBG(data, length, "payload");

    return BT_GATT_ITER_CONTINUE;
}

static struct bt_gatt_read_params read_params;
static const struct bt_uuid_16 hids_uuid = BT_UUID_INIT_16(BT_UUID_HIDS_REPORT_VAL);

static uint8_t read_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
                       const void *data, uint16_t length) {
    LOG_DBG("Read err: %d, length %d", err, length);

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params) {
    int err;

    if (!attr) {
        LOG_DBG("[Discover complete]");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[ATTRIBUTE] handle %u", attr->handle);

    if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HIDS)) {
        memcpy(&uuid, BT_UUID_HIDS_REPORT, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.start_handle = attr->handle + 1;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            LOG_DBG("[Discover failed] (err %d)", err);
        }
    } else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HIDS_REPORT)) {
        if (read_directly_on_discovery) {
            read_params.single.handle = bt_gatt_attr_value_handle(attr);
            read_params.single.offset = 0;
            read_params.handle_count = 1;
            read_params.func = read_cb;

            bt_gatt_read(conn, &read_params);
        } else {
            memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
            discover_params.uuid = &uuid.uuid;
            discover_params.start_handle = attr->handle + 2;
            discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
            subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

            err = bt_gatt_discover(conn, &discover_params);
            if (err) {
                LOG_DBG("[Discover failed] (err %d)", err);
            }
        }
    } else {
        subscribe_params.notify = notify_func;
        subscribe_params.value = BT_GATT_CCC_NOTIFY;
        subscribe_params.ccc_handle = attr->handle;

        err = bt_gatt_subscribe(conn, &subscribe_params);
        if (err && err != -EALREADY) {
            LOG_DBG("[Subscribe failed] (err %d)", err);
        } else {
            LOG_DBG("[SUBSCRIBED]");
        }

        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_STOP;
}

static void reconnect(const bt_addr_le_t *addr) {
    struct bt_le_conn_param *param;
    int err = bt_le_scan_stop();
    if (err < 0) {
        LOG_DBG("Stop LE scan failed (err %d)", err);
    }

    param = BT_LE_CONN_PARAM_DEFAULT;
    err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
    if (err < 0) {
        LOG_DBG("Create conn failed (err %d)", err);
        start_scan();
    }
}

static bool eir_found(struct bt_data *data, void *user_data) {
    bt_addr_le_t *addr = user_data;
    int i;

    LOG_DBG("[AD]: %u data_len %u", data->type, data->data_len);

    switch (data->type) {
    case BT_DATA_UUID16_SOME:
    case BT_DATA_UUID16_ALL:
        if (data->data_len % sizeof(uint16_t) != 0U) {
            LOG_DBG("[AD malformed]");
            return true;
        }

        for (i = 0; i < data->data_len; i += sizeof(uint16_t)) {
            struct bt_le_conn_param *param;
            struct bt_uuid *uuid;
            uint16_t u16;
            int err;

            memcpy(&u16, &data->data[i], sizeof(u16));
            uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
            if (bt_uuid_cmp(uuid, BT_UUID_HIDS)) {
                continue;
            }

            err = bt_le_scan_stop();
            if (err) {
                LOG_DBG("[Stop LE scan failed] (err %d)", err);
                continue;
            }

            param = BT_LE_CONN_PARAM_DEFAULT;
            err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
            if (err) {
                LOG_DBG("[Create conn failed] (err %d)", err);
                start_scan();
            }

            return false;
        }
    }

    return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad) {
    char dev[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, dev, sizeof(dev));
    LOG_DBG("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i", dev, type, ad->len, rssi);

    /* We're only interested in connectable events */
    if (type == BT_GAP_ADV_TYPE_ADV_IND) {
        bt_data_parse(ad, eir_found, (void *)addr);
    } else if (type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        reconnect(addr);
    }
}

static void start_scan(void) {
    int err;

    /* Use active scanning and disable duplicate filtering to handle any
     * devices that might update their advertising data at runtime. */
    struct bt_le_scan_param scan_param = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    err = bt_le_scan_start(&scan_param, device_found);
    if (err) {
        LOG_DBG("[Scanning failed to start] (err %d)", err);
        return;
    }

    LOG_DBG("[Scanning successfully started]");
}

static void discover_conn(struct bt_conn *conn) {
    int err;

    LOG_DBG("[Discovery started for conn]");
    memcpy(&uuid, BT_UUID_HIDS, sizeof(uuid));
    discover_params.uuid = &uuid.uuid;
    discover_params.func = discover_func;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    err = bt_gatt_discover(default_conn, &discover_params);
    if (err) {
        LOG_DBG("[Discover failed] (err %d)", err);
        return;
    }
}

static void connected(struct bt_conn *conn, uint8_t conn_err) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err) {
        LOG_DBG("[Failed to connect to %s] (%u)", addr, conn_err);

        bt_conn_unref(default_conn);
        default_conn = NULL;

        start_scan();
        return;
    }

    LOG_DBG("[Connected]: %s", addr);

    if (conn == default_conn) {
        if (bt_conn_get_security(conn) >= BT_SECURITY_L2 && !skip_discovery_on_connect) {
            LOG_DBG("[Discovering characteristics for the connection]");
            discover_conn(conn);
        } else if (!skip_set_security_on_connect) {
            LOG_DBG("[Setting the security for the connection]");
            bt_conn_set_security(conn, BT_SECURITY_L2);
        }

        if (read_hid_report_on_connect) {
            read_params.func = read_cb;
            read_params.handle_count = 0;
            read_params.by_uuid.start_handle = 0x0001;
            read_params.by_uuid.end_handle = 0xFFFF;
            read_params.by_uuid.uuid = &hids_uuid.uuid;

            bt_gatt_read(conn, &read_params);
        }
    }
}

static bool first_connect = true;
static void pairing_complete(struct bt_conn *conn, bool bonded) { LOG_DBG("Pairing complete"); }

static void do_disconnect_of_active(struct k_work *work) {
    bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (clear_bond_on_disconnect) {
        bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(default_conn));
    }
}

static K_WORK_DELAYABLE_DEFINE(disconnect_work, do_disconnect_of_active);

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    if (err > BT_SECURITY_ERR_SUCCESS) {
        LOG_DBG("[Security Change Failed]");
        exit(1);
    }

    if (halt_after_bonding) {
        exit(1);
    }

    bool do_disconnect = first_connect && disconnect_and_reconnect;
    first_connect = false;
    if (do_disconnect) {
        k_work_reschedule(&disconnect_work, K_MSEC(500));
    } else if (!skip_discovery_on_connect) {
        discover_conn(conn);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("[Disconnected]: %s (reason 0x%02x)", addr, reason);

    if (default_conn != conn) {
        return;
    }

    bt_conn_unref(default_conn);
    default_conn = NULL;

    if (!halt_after_bonding) {
        start_scan();
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

struct bt_conn_auth_info_cb auth_info_cb = {
    .pairing_complete = pairing_complete,
};

void main(void) {
    int err;

    if (wait_on_start > 0) {
        k_sleep(K_MSEC(wait_on_start));
    }

    err = bt_conn_auth_info_cb_register(&auth_info_cb);

    err = bt_enable(NULL);

    if (err) {
        LOG_DBG("[Bluetooth init failed] (err %d)", err);
        return;
    }

    LOG_DBG("[Bluetooth initialized]");

    start_scan();
}
