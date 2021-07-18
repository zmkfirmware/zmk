/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/ble.h>
#include <zmk/split/bluetooth/uuid.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <init.h>

static int start_scan(void);

// Define array for state of peripheral keys. Each array value is initialized to
// 255 inside zmk_split_bt_central_init() as no key is pressed initially. The
// array value corresponds to the peripheral_conns index for pressed keys.
#define POSITION_STATE_DATA_LEN 16
static uint8_t position_state[8 * POSITION_STATE_DATA_LEN];

// Define array for holding peripheral connections.
static struct bt_conn *peripheral_conns[CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS];

static struct bt_uuid *service_uuid = BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID);
static struct bt_uuid *characteristic_uuid =
    BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_POSITION_STATE_UUID);
static struct bt_uuid *ccc_uuid = BT_UUID_GATT_CCC;

// Track whether central is currently scanning for peripherals.
static bool is_scanning = false;

static struct bt_gatt_discover_params discover_params[CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS];
static struct bt_gatt_subscribe_params subscribe_params[CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS];

K_MSGQ_DEFINE(peripheral_event_msgq, sizeof(struct zmk_position_state_changed),
              CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE, 4);

void peripheral_event_work_callback(struct k_work *work) {
    struct zmk_position_state_changed ev;
    while (k_msgq_get(&peripheral_event_msgq, &ev, K_NO_WAIT) == 0) {
        LOG_DBG("Trigger key position state change for %d", ev.position);
        ZMK_EVENT_RAISE(new_zmk_position_state_changed(ev));
    }
}

K_WORK_DEFINE(peripheral_event_work, peripheral_event_work_callback);

static void split_set_position(int peripheral_id, int position, bool is_pressed) {
    bool was_pressed = position_state[position] != 255;

    // Determine new state for position. Ignore released key if it was pressed
    // on a different peripheral.
    if (is_pressed) {
        position_state[position] = peripheral_id;
    } else if (position_state[position] == peripheral_id) {
        position_state[position] = 255;
    } else {
        return;
    }

    // Handle event if state changed.
    bool changed = was_pressed != is_pressed;
    if (changed) {
        struct zmk_position_state_changed ev = {
            .position = position, .state = is_pressed, .timestamp = k_uptime_get()};
        k_msgq_put(&peripheral_event_msgq, &ev, K_NO_WAIT);
        k_work_submit(&peripheral_event_work);
    }
}

static int split_central_get_peripheral_id(struct bt_conn *conn) {
    for (int i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (peripheral_conns[i] == conn) {
            return i;
        }
    }
    return -1;
}

static uint8_t split_central_notify_func(struct bt_conn *conn,
                                         struct bt_gatt_subscribe_params *params, const void *data,
                                         uint16_t length) {
    if (!data) {
        LOG_DBG("[UNSUBSCRIBED]");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[NOTIFICATION] data %p length %u", data, length);

    int peripheral_id = split_central_get_peripheral_id(conn);
    if (peripheral_id == -1) {
        LOG_ERR("unable to identify peripheral connection");
        return BT_GATT_ITER_STOP;
    }
    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {
        for (int j = 0; j < 8; j++) {
            int position = (i * 8) + j;
            bool is_pressed = ((uint8_t *)data)[i] & BIT(j);
            split_set_position(peripheral_id, position, is_pressed);
        }
    }

    return BT_GATT_ITER_CONTINUE;
}

static int split_central_subscribe(struct bt_conn *conn) {
    int peripheral_id = split_central_get_peripheral_id(conn);
    if (peripheral_id == -1) {
        LOG_ERR("unable to identify peripheral connection");
        return BT_GATT_ITER_STOP;
    }

    int err = bt_gatt_subscribe(conn, &subscribe_params[peripheral_id]);
    switch (err) {
    case -EALREADY:
        LOG_DBG("[ALREADY SUBSCRIBED]");
        break;
    case 0:
        LOG_DBG("[SUBSCRIBED]");
        break;
    default:
        LOG_ERR("Subscribe failed (err %d)", err);
        break;
    }

    return 0;
}

static uint8_t split_central_discovery_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                            struct bt_gatt_discover_params *params) {
    int err;

    if (!attr) {
        LOG_DBG("Discover complete");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[ATTRIBUTE] handle %u", attr->handle);

    int peripheral_id = split_central_get_peripheral_id(conn);
    if (peripheral_id == -1) {
        LOG_ERR("unable to identify peripheral connection");
        return BT_GATT_ITER_STOP;
    }

    // After discovering the split service, discover the position state
    // characteristic.
    if (!bt_uuid_cmp(discover_params[peripheral_id].uuid, service_uuid)) {
        discover_params[peripheral_id].uuid = characteristic_uuid;
        discover_params[peripheral_id].start_handle = attr->handle + 1;
        discover_params[peripheral_id].type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &discover_params[peripheral_id]);
        if (err) {
            LOG_ERR("Discover failed (err %d)", err);
        }
        return BT_GATT_ITER_STOP;
    }

    // After discovering the position state characteristic, discover the CCC
    // descriptor.
    if (!bt_uuid_cmp(discover_params[peripheral_id].uuid, characteristic_uuid)) {
        discover_params[peripheral_id].uuid = ccc_uuid;
        discover_params[peripheral_id].start_handle = attr->handle + 2;
        discover_params[peripheral_id].type = BT_GATT_DISCOVER_DESCRIPTOR;
        subscribe_params[peripheral_id].value_handle = bt_gatt_attr_value_handle(attr);

        err = bt_gatt_discover(conn, &discover_params[peripheral_id]);
        if (err) {
            LOG_ERR("Discover failed (err %d)", err);
        }
        return BT_GATT_ITER_STOP;
    }

    // After discovering the CCC descriptor, enable notifications.
    subscribe_params[peripheral_id].notify = split_central_notify_func;
    subscribe_params[peripheral_id].value = BT_GATT_CCC_NOTIFY;
    subscribe_params[peripheral_id].ccc_handle = attr->handle;
    split_central_subscribe(conn);
    return BT_GATT_ITER_STOP;
}

static void split_central_process_connection(struct bt_conn *conn, int peripheral_id) {
    LOG_DBG("Current security for connection: %d", bt_conn_get_security(conn));

    int err = bt_conn_set_security(conn, BT_SECURITY_L2);
    if (err) {
        LOG_ERR("Failed to set security (reason %d)", err);
        return;
    }

    LOG_DBG("Starting discovery for peripheral #%d", peripheral_id);

    // Clear discovery parameters from previous run.
    (void)memset(&discover_params[peripheral_id], 0, sizeof(discover_params[peripheral_id]));

    discover_params[peripheral_id].uuid = service_uuid;
    discover_params[peripheral_id].func = split_central_discovery_func;
    discover_params[peripheral_id].start_handle = 0x0001;
    discover_params[peripheral_id].end_handle = 0xffff;
    discover_params[peripheral_id].type = BT_GATT_DISCOVER_PRIMARY;

    err = bt_gatt_discover(peripheral_conns[peripheral_id], &discover_params[peripheral_id]);
    if (err) {
        LOG_ERR("Discover failed(err %d)", err);
        return;
    }

    struct bt_conn_info info;
    bt_conn_get_info(conn, &info);
    LOG_DBG("New connection params: Interval: %d, Latency: %d, PHY: %d", info.le.interval,
            info.le.latency, info.le.phy->rx_phy);
}

static bool split_central_eir_found(struct bt_data *data, void *user_data) {
    bt_addr_le_t *addr = user_data;
    LOG_DBG("[AD]: %u data_len %u", data->type, data->data_len);

    switch (data->type) {
    case BT_DATA_UUID128_SOME:
    case BT_DATA_UUID128_ALL:
        if (data->data_len % 16 != 0U) {
            LOG_ERR("AD malformed");
            return true;
        }

        for (int i = 0; i < data->data_len; i += 16) {
            struct bt_le_conn_param *param;
            struct bt_uuid_128 uuid;
            int err;

            if (!bt_uuid_create(&uuid.uuid, &data->data[i], 16)) {
                LOG_ERR("Unable to load UUID");
                continue;
            }

            if (bt_uuid_cmp(&uuid.uuid, service_uuid)) {
                char uuid_str[BT_UUID_STR_LEN];
                char service_uuid_str[BT_UUID_STR_LEN];

                bt_uuid_to_str(&uuid.uuid, uuid_str, sizeof(uuid_str));
                bt_uuid_to_str(service_uuid, service_uuid_str, sizeof(service_uuid_str));
                LOG_DBG("UUID %s does not match split UUID: %s", log_strdup(uuid_str),
                        log_strdup(service_uuid_str));
                continue;
            }

            LOG_DBG("Found the split service");

            // Store peripheral address. If this operation fails, the peripheral
            // must not match any of the known peripherals. Return false to stop
            // parsing advertising data for peripheral.
            int peripheral_i = zmk_ble_put_peripheral_addr(addr);
            if (peripheral_i == -1) {
                return false;
            }

            // Stop scanning so we can connect to the peripheral device.
            LOG_DBG("Stopping peripheral scanning");
            is_scanning = false;
            err = bt_le_scan_stop();
            if (err) {
                LOG_ERR("Stop LE scan failed (err %d)", err);
                continue;
            }

            // Create connection to peripheral with the given connection
            // parameters.
            param = BT_LE_CONN_PARAM(0x0006, 0x0006, 30, 400);
            err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param,
                                    &peripheral_conns[peripheral_i]);
            if (err) {
                LOG_ERR("Create conn failed (err %d) (create conn? 0x%04x)", err,
                        BT_HCI_OP_LE_CREATE_CONN);
                start_scan();
                return false;
            }

            err = bt_conn_le_phy_update(peripheral_conns[peripheral_i], BT_CONN_LE_PHY_PARAM_2M);
            if (err) {
                LOG_ERR("Update phy conn failed (err %d)", err);
                start_scan();
                return false;
            }

            // Stop processing advertisement data.
            return false;
        }
    }

    return true;
}

static void split_central_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                                       struct net_buf_simple *ad) {
    char dev[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, dev, sizeof(dev));
    LOG_DBG("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i", log_strdup(dev), type, ad->len,
            rssi);

    /* We're only interested in connectable events */
    if (type == BT_GAP_ADV_TYPE_ADV_IND || type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        bt_data_parse(ad, split_central_eir_found, (void *)addr);
    }
}

static int start_scan(void) {
    // No action is necessary if central is already scanning.
    if (is_scanning) {
        LOG_DBG("scanning is already on");
        return 0;
    }

    // If all the devices are connected, there is no need to scan.
    bool has_unconnected = false;
    for (int i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (peripheral_conns[i] == NULL) {
            has_unconnected = true;
            break;
        }
    }
    if (!has_unconnected) {
        LOG_DBG("all devices are connected");
        return 0;
    }

    // Start scanning otherwise.
    is_scanning = true;
    int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, split_central_device_found);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return err;
    }

    LOG_DBG("Scanning successfully started");
    return 0;
}

static void split_central_connected(struct bt_conn *conn, uint8_t conn_err) {
    // Only handle connection if it corresponds to a peripheral.
    for (int i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (conn == peripheral_conns[i]) {
            char addr[BT_ADDR_LE_STR_LEN];
            bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

            if (conn_err) {
                LOG_ERR("Failed to connect to %s (%u)", log_strdup(addr), conn_err);

                bt_conn_unref(peripheral_conns[i]);
                peripheral_conns[i] = NULL;
            } else {
                LOG_DBG("Connected: %s", log_strdup(addr));
                split_central_process_connection(conn, i);
            }

            // Start scanning again if necessary.
            start_scan();
        }
    }
}

static void split_central_disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Disconnected: %s (reason %d)", log_strdup(addr), reason);

    // Only handle connection if it corresponds to a peripheral.
    for (int i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (conn == peripheral_conns[i]) {
            // Unset peripheral connection.
            bt_conn_unref(peripheral_conns[i]);
            peripheral_conns[i] = NULL;

            // Release all keys that were held by the peripheral.
            for (int position = 0; position < 8 * POSITION_STATE_DATA_LEN; position++) {
                split_set_position(i, position, false);
            }

            // Start scanning again if necessary.
            start_scan();
        }
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected = split_central_connected,
    .disconnected = split_central_disconnected,
};

int zmk_split_bt_central_init(const struct device *_arg) {
    // Initialize array for state of peripheral keys. Set array to 255, which
    // signifies that no key is active.
    (void)memset(position_state, 255, 8 * POSITION_STATE_DATA_LEN);

    bt_conn_cb_register(&conn_callbacks);
    return start_scan();
}

SYS_INIT(zmk_split_bt_central_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
