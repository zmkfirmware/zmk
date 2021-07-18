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
#include <zmk/sensors.h>
#include <zmk/split/bluetooth/uuid.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <init.h>

static int start_scan(void);

#define POSITION_STATE_DATA_LEN 16

static struct bt_conn *default_conn;

static struct bt_uuid_128 uuid = BT_UUID_INIT_128(ZMK_SPLIT_BT_SERVICE_UUID);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

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

#if ZMK_KEYMAP_HAS_SENSORS
K_MSGQ_DEFINE(peripheral_sensor_event_msgq, sizeof(struct zmk_sensor_event),
              CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE, 4);

void peripheral_sensor_event_work_callback(struct k_work *work) {
    struct zmk_sensor_event ev;
    while (k_msgq_get(&peripheral_sensor_event_msgq, &ev, K_NO_WAIT) == 0) {
        LOG_DBG("Trigger sensor change for %d", ev.sensor_number);
        ZMK_EVENT_RAISE(new_zmk_sensor_event(ev));
    }
}

K_WORK_DEFINE(peripheral_sensor_event_work, peripheral_sensor_event_work_callback);

struct sensor_event {
    uint8_t sensor_number;
    struct sensor_value value;
};

static uint8_t split_central_sensor_notify_func(struct bt_conn *conn,
                                                struct bt_gatt_subscribe_params *params,
                                                const void *data, uint16_t length) {

    const struct sensor_event *sensor_event = data;

    if (!data) {
        LOG_DBG("[UNSUBSCRIBED]");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }
    LOG_DBG("[SENSOR NOTIFICATION] data %p length %u", data, length);

    struct zmk_sensor_event ev = {
        .sensor_number = sensor_event->sensor_number,
        .value = {.val1 = (sensor_event->value).val1, .val2 = (sensor_event->value).val2},
        .timestamp = k_uptime_get()};

    k_msgq_put(&peripheral_sensor_event_msgq, &ev, K_NO_WAIT);
    k_work_submit(&peripheral_sensor_event_work);

    return BT_GATT_ITER_CONTINUE;
}
#endif /* ZMK_KEYMAP_HAS_SENSORS */

static uint8_t split_central_notify_func(struct bt_conn *conn,
                                         struct bt_gatt_subscribe_params *params, const void *data,
                                         uint16_t length) {
    static uint8_t position_state[POSITION_STATE_DATA_LEN];

    uint8_t changed_positions[POSITION_STATE_DATA_LEN];

    if (!data) {
        LOG_DBG("[UNSUBSCRIBED]");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[NOTIFICATION] data %p length %u", data, length);

    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {
        changed_positions[i] = ((uint8_t *)data)[i] ^ position_state[i];
        position_state[i] = ((uint8_t *)data)[i];
    }

    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {
        for (int j = 0; j < 8; j++) {
            if (changed_positions[i] & BIT(j)) {
                uint32_t position = (i * 8) + j;
                bool pressed = position_state[i] & BIT(j);
                struct zmk_position_state_changed ev = {
                    .position = position, .state = pressed, .timestamp = k_uptime_get()};

                k_msgq_put(&peripheral_event_msgq, &ev, K_NO_WAIT);
                k_work_submit(&peripheral_event_work);
            }
        }
    }

    return BT_GATT_ITER_CONTINUE;
}

static int split_central_subscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params) {
    int err = bt_gatt_subscribe(conn, params);
    switch (err) {
    case -EALREADY:
        LOG_DBG("[ALREADY SUBSCRIBED]");
        break;
        // break;
        // bt_gatt_unsubscribe(conn, &subscribe_params);
        // return split_central_subscribe(conn);
    case 0:
        LOG_DBG("[SUBSCRIBED]");
        break;
    default:
        LOG_ERR("Subscribe failed (err %d)", err);
        break;
    }

    return 0;
}

#if ZMK_KEYMAP_HAS_SENSORS
static struct bt_uuid_128 sensor_uuid = BT_UUID_INIT_128(ZMK_SPLIT_BT_SERVICE_UUID);
static struct bt_gatt_discover_params sensor_discover_params;
static struct bt_gatt_subscribe_params sensor_subscribe_params;
static uint8_t split_central_sensor_desc_discovery_func(struct bt_conn *conn,
                                                        const struct bt_gatt_attr *attr,
                                                        struct bt_gatt_discover_params *params) {
    int err;

    if (!bt_uuid_cmp(sensor_discover_params.uuid,
                     BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_SENSOR_STATE_UUID))) {
        memcpy(&sensor_uuid, BT_UUID_GATT_CCC, sizeof(uuid));
        sensor_discover_params.uuid = &sensor_uuid.uuid;
        sensor_discover_params.start_handle = attr->handle;
        sensor_discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

        sensor_subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

        err = bt_gatt_discover(conn, &sensor_discover_params);
        if (err) {
            LOG_ERR("Discover failed (err %d)", err);
        }
    } else {
        sensor_subscribe_params.notify = split_central_sensor_notify_func;
        sensor_subscribe_params.value = BT_GATT_CCC_NOTIFY;
        sensor_subscribe_params.ccc_handle = attr->handle;
        split_central_subscribe(conn, &sensor_subscribe_params);
    }

    return BT_GATT_ITER_STOP;
}
#endif /* ZMK_KEYMAP_HAS_SENSORS */

static uint8_t split_central_discovery_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                            struct bt_gatt_discover_params *params) {
    int err;

    if (!attr) {
        LOG_DBG("Discover complete");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[ATTRIBUTE] handle %u", attr->handle);

    if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID))) {
        memcpy(&uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_POSITION_STATE_UUID), sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.start_handle = attr->handle + 1;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            LOG_ERR("Discover failed (err %d)", err);
        }
    } else if (!bt_uuid_cmp(discover_params.uuid,
                            BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_POSITION_STATE_UUID))) {
        memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.start_handle = attr->handle + 2;
        discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
        subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            LOG_ERR("Discover failed (err %d)", err);
        }

#if ZMK_KEYMAP_HAS_SENSORS
        memcpy(&sensor_uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_SENSOR_STATE_UUID),
               sizeof(sensor_uuid));
        sensor_discover_params.uuid = &sensor_uuid.uuid;
        sensor_discover_params.start_handle = attr->handle;
        sensor_discover_params.end_handle = 0xffff;
        sensor_discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
        sensor_discover_params.func = split_central_sensor_desc_discovery_func;

        err = bt_gatt_discover(conn, &sensor_discover_params);
        if (err) {
            LOG_ERR("Discover failed (err %d)", err);
        }
#endif /* ZMK_KEYMAP_HAS_SENSORS */
    } else {
        subscribe_params.notify = split_central_notify_func;
        subscribe_params.value = BT_GATT_CCC_NOTIFY;
        subscribe_params.ccc_handle = attr->handle;

        split_central_subscribe(conn, &subscribe_params);

        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_STOP;
}

static void split_central_process_connection(struct bt_conn *conn) {
    int err;

    LOG_DBG("Current security for connection: %d", bt_conn_get_security(conn));

    err = bt_conn_set_security(conn, BT_SECURITY_L2);
    if (err) {
        LOG_ERR("Failed to set security (reason %d)", err);
        return;
    }

    if (conn == default_conn && !subscribe_params.value) {
        discover_params.uuid = &uuid.uuid;
        discover_params.func = split_central_discovery_func;
        discover_params.start_handle = 0x0001;
        discover_params.end_handle = 0xffff;
        discover_params.type = BT_GATT_DISCOVER_PRIMARY;

        err = bt_gatt_discover(default_conn, &discover_params);
        if (err) {
            LOG_ERR("Discover failed(err %d)", err);
            return;
        }
    }

    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);

    LOG_DBG("New connection params: Interval: %d, Latency: %d, PHY: %d", info.le.interval,
            info.le.latency, info.le.phy->rx_phy);
}

static bool split_central_eir_found(struct bt_data *data, void *user_data) {
    bt_addr_le_t *addr = user_data;
    int i;

    LOG_DBG("[AD]: %u data_len %u", data->type, data->data_len);

    switch (data->type) {
    case BT_DATA_UUID128_SOME:
    case BT_DATA_UUID128_ALL:
        if (data->data_len % 16 != 0U) {
            LOG_ERR("AD malformed");
            return true;
        }

        for (i = 0; i < data->data_len; i += 16) {
            struct bt_le_conn_param *param;
            struct bt_uuid_128 uuid;
            int err;

            if (!bt_uuid_create(&uuid.uuid, &data->data[i], 16)) {
                LOG_ERR("Unable to load UUID");
                continue;
            }

            if (bt_uuid_cmp(&uuid.uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID))) {
                char uuid_str[BT_UUID_STR_LEN];
                char service_uuid_str[BT_UUID_STR_LEN];

                bt_uuid_to_str(&uuid.uuid, uuid_str, sizeof(uuid_str));
                bt_uuid_to_str(BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID), service_uuid_str,
                               sizeof(service_uuid_str));
                LOG_DBG("UUID %s does not match split UUID: %s", log_strdup(uuid_str),
                        log_strdup(service_uuid_str));
                continue;
            }

            LOG_DBG("Found the split service");

            zmk_ble_set_peripheral_addr(addr);

            err = bt_le_scan_stop();
            if (err) {
                LOG_ERR("Stop LE scan failed (err %d)", err);
                continue;
            }

            default_conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);
            if (default_conn) {
                LOG_DBG("Found existing connection");
                split_central_process_connection(default_conn);
            } else {
                param = BT_LE_CONN_PARAM(0x0006, 0x0006, 30, 400);

                err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
                if (err) {
                    LOG_ERR("Create conn failed (err %d) (create conn? 0x%04x)", err,
                            BT_HCI_OP_LE_CREATE_CONN);
                    start_scan();
                }

                err = bt_conn_le_phy_update(default_conn, BT_CONN_LE_PHY_PARAM_2M);
                if (err) {
                    LOG_ERR("Update phy conn failed (err %d)", err);
                    start_scan();
                }
            }

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
    int err;

    err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, split_central_device_found);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return err;
    }

    LOG_DBG("Scanning successfully started");
    return 0;
}

static void split_central_connected(struct bt_conn *conn, uint8_t conn_err) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err) {
        LOG_ERR("Failed to connect to %s (%u)", log_strdup(addr), conn_err);

        bt_conn_unref(default_conn);
        default_conn = NULL;

        start_scan();
        return;
    }

    LOG_DBG("Connected: %s", log_strdup(addr));

    split_central_process_connection(conn);
}

static void split_central_disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Disconnected: %s (reason %d)", log_strdup(addr), reason);

    if (default_conn != conn) {
        return;
    }

    bt_conn_unref(default_conn);
    default_conn = NULL;

    start_scan();
}

static struct bt_conn_cb conn_callbacks = {
    .connected = split_central_connected,
    .disconnected = split_central_disconnected,
};

int zmk_split_bt_central_init(const struct device *_arg) {
    bt_conn_cb_register(&conn_callbacks);

    return start_scan();
}

SYS_INIT(zmk_split_bt_central_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
