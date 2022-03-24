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

#include <zmk/stdlib.h>
#include <zmk/ble.h>
#include <zmk/behavior.h>
#include <zmk/sensors.h>
#include <zmk/split/bluetooth/uuid.h>
#include <zmk/split/bluetooth/service.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <init.h>

static int start_scan(void);

#define POSITION_STATE_DATA_LEN 16

enum peripheral_slot_state {
    PERIPHERAL_SLOT_STATE_OPEN,
    PERIPHERAL_SLOT_STATE_CONNECTING,
    PERIPHERAL_SLOT_STATE_CONNECTED,
};

struct peripheral_slot {
    enum peripheral_slot_state state;
    struct bt_conn *conn;
    struct bt_gatt_discover_params discover_params;
    struct bt_gatt_subscribe_params subscribe_params;
    struct bt_gatt_discover_params sub_discover_params;
    uint16_t run_behavior_handle;
    uint8_t position_state[POSITION_STATE_DATA_LEN];
    uint8_t changed_positions[POSITION_STATE_DATA_LEN];
};

static struct peripheral_slot peripherals[ZMK_BLE_SPLIT_PERIPHERAL_COUNT];

static const struct bt_uuid_128 split_service_uuid = BT_UUID_INIT_128(ZMK_SPLIT_BT_SERVICE_UUID);

K_MSGQ_DEFINE(peripheral_event_msgq, sizeof(struct zmk_position_state_changed),
              CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE, 4);

int peripheral_slot_index_for_conn(struct bt_conn *conn) {
    for (int i = 0; i < ZMK_BLE_SPLIT_PERIPHERAL_COUNT; i++) {
        if (peripherals[i].conn == conn) {
            return i;
        }
    }

    return -EINVAL;
}

struct peripheral_slot *peripheral_slot_for_conn(struct bt_conn *conn) {
    int idx = peripheral_slot_index_for_conn(conn);
    if (idx < 0) {
        return NULL;
    }

    return &peripherals[idx];
}

int release_peripheral_slot(int index) {
    if (index < 0 || index >= ZMK_BLE_SPLIT_PERIPHERAL_COUNT) {
        return -EINVAL;
    }

    struct peripheral_slot *slot = &peripherals[index];

    if (slot->state == PERIPHERAL_SLOT_STATE_OPEN) {
        return -EINVAL;
    }

    LOG_DBG("Releasing peripheral slot at %d", index);

    if (slot->conn != NULL) {
        bt_conn_unref(slot->conn);
        slot->conn = NULL;
    }
    slot->state = PERIPHERAL_SLOT_STATE_OPEN;

    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {
        slot->position_state[i] = 0U;
        slot->changed_positions[i] = 0U;
    }

    // Clean up previously discovered handles;
    slot->subscribe_params.value_handle = 0;
    slot->run_behavior_handle = 0;

    return 0;
}

int reserve_peripheral_slot() {
    for (int i = 0; i < ZMK_BLE_SPLIT_PERIPHERAL_COUNT; i++) {
        if (peripherals[i].state == PERIPHERAL_SLOT_STATE_OPEN) {
            // Be sure the slot is fully reinitialized.
            release_peripheral_slot(i);
            peripherals[i].state = PERIPHERAL_SLOT_STATE_CONNECTING;
            return i;
        }
    }

    return -ENOMEM;
}

int release_peripheral_slot_for_conn(struct bt_conn *conn) {
    int idx = peripheral_slot_index_for_conn(conn);
    if (idx < 0) {
        return idx;
    }

    return release_peripheral_slot(idx);
}

int confirm_peripheral_slot_conn(struct bt_conn *conn) {
    int idx = peripheral_slot_index_for_conn(conn);
    if (idx < 0) {
        return idx;
    }

    peripherals[idx].state = PERIPHERAL_SLOT_STATE_CONNECTED;
    return 0;
}

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
    struct peripheral_slot *slot = peripheral_slot_for_conn(conn);

    if (slot == NULL) {
        LOG_ERR("No peripheral state found for connection");
        return BT_GATT_ITER_CONTINUE;
    }

    if (!data) {
        LOG_DBG("[UNSUBSCRIBED]");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[NOTIFICATION] data %p length %u", data, length);

    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {
        slot->changed_positions[i] = ((uint8_t *)data)[i] ^ slot->position_state[i];
        slot->position_state[i] = ((uint8_t *)data)[i];
        LOG_DBG("data: %d", slot->position_state[i]);
    }

    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {
        for (int j = 0; j < 8; j++) {
            if (slot->changed_positions[i] & BIT(j)) {
                uint32_t position = (i * 8) + j;
                bool pressed = slot->position_state[i] & BIT(j);
                struct zmk_position_state_changed ev = {.source =
                                                            peripheral_slot_index_for_conn(conn),
                                                        .position = position,
                                                        .state = pressed,
                                                        .timestamp = k_uptime_get()};

                k_msgq_put(&peripheral_event_msgq, &ev, K_NO_WAIT);
                k_work_submit(&peripheral_event_work);
            }
        }
    }

    return BT_GATT_ITER_CONTINUE;
}

static void split_central_subscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params) {
    int err = bt_gatt_subscribe(conn, params);
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
        memcpy(&sensor_uuid, BT_UUID_GATT_CCC, sizeof(sensor_uuid));
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

static uint8_t split_central_chrc_discovery_func(struct bt_conn *conn,
                                                 const struct bt_gatt_attr *attr,
                                                 struct bt_gatt_discover_params *params) {
    if (!attr) {
        LOG_DBG("Discover complete");
        return BT_GATT_ITER_STOP;
    }

    if (!attr->user_data) {
        LOG_ERR("Required user data not passed to discovery");
        return BT_GATT_ITER_STOP;
    }

    struct peripheral_slot *slot = peripheral_slot_for_conn(conn);
    if (slot == NULL) {
        LOG_ERR("No peripheral state found for connection");
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[ATTRIBUTE] handle %u", attr->handle);

    if (!bt_uuid_cmp(((struct bt_gatt_chrc *)attr->user_data)->uuid,
                     BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_POSITION_STATE_UUID))) {
        LOG_DBG("Found position state characteristic");
        slot->discover_params.uuid = NULL;
        slot->discover_params.start_handle = attr->handle + 2;
        slot->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        slot->subscribe_params.disc_params = &slot->sub_discover_params;
        slot->subscribe_params.end_handle = slot->discover_params.end_handle;
        slot->subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);
        slot->subscribe_params.notify = split_central_notify_func;
        slot->subscribe_params.value = BT_GATT_CCC_NOTIFY;
        split_central_subscribe(conn, &slot->subscribe_params);
    } else if (!bt_uuid_cmp(((struct bt_gatt_chrc *)attr->user_data)->uuid,
                            BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_RUN_BEHAVIOR_UUID))) {
        LOG_DBG("Found run behavior handle");
        slot->run_behavior_handle = bt_gatt_attr_value_handle(attr);
    }

    bool subscribed = (slot->run_behavior_handle && slot->subscribe_params.value_handle);

    return subscribed ? BT_GATT_ITER_STOP : BT_GATT_ITER_CONTINUE;
}

static uint8_t split_central_service_discovery_func(struct bt_conn *conn,
                                                    const struct bt_gatt_attr *attr,
                                                    struct bt_gatt_discover_params *params) {
    if (!attr) {
        LOG_DBG("Discover complete");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[ATTRIBUTE] handle %u", attr->handle);

    struct peripheral_slot *slot = peripheral_slot_for_conn(conn);
    if (slot == NULL) {
        LOG_ERR("No peripheral state found for connection");
        return BT_GATT_ITER_STOP;
    }

    if (bt_uuid_cmp(slot->discover_params.uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID))) {
        LOG_DBG("Found other service");
        return BT_GATT_ITER_CONTINUE;
    }

    LOG_DBG("Found split service");
    slot->discover_params.uuid = NULL;
    slot->discover_params.func = split_central_chrc_discovery_func;
    slot->discover_params.start_handle = attr->handle + 1;
    slot->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

    int err = bt_gatt_discover(conn, &slot->discover_params);
    if (err) {
        LOG_ERR("Failed to start discovering split service characteristics (err %d)", err);
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

    struct peripheral_slot *slot = peripheral_slot_for_conn(conn);
    if (slot == NULL) {
        LOG_ERR("No peripheral state found for connection");
        return;
    }

    if (!slot->subscribe_params.value_handle) {
        slot->discover_params.uuid = &split_service_uuid.uuid;
        slot->discover_params.func = split_central_service_discovery_func;
        slot->discover_params.start_handle = 0x0001;
        slot->discover_params.end_handle = 0xffff;
        slot->discover_params.type = BT_GATT_DISCOVER_PRIMARY;

        err = bt_gatt_discover(slot->conn, &slot->discover_params);
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

            uint8_t slot_idx = reserve_peripheral_slot();
            if (slot_idx < 0) {
                LOG_ERR("Faild to reserve peripheral slot (err %d)", slot_idx);
                continue;
            }

            struct peripheral_slot *slot = &peripherals[slot_idx];

            slot->conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);
            if (slot->conn) {
                LOG_DBG("Found existing connection");
                split_central_process_connection(slot->conn);
                err = bt_conn_le_phy_update(slot->conn, BT_CONN_LE_PHY_PARAM_2M);
                if (err) {
                    LOG_ERR("Update phy conn failed (err %d)", err);
                }
            } else {
                param = BT_LE_CONN_PARAM(0x0006, 0x0006, 30, 400);

                err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &slot->conn);
                if (err) {
                    LOG_ERR("Create conn failed (err %d) (create conn? 0x%04x)", err,
                            BT_HCI_OP_LE_CREATE_CONN);
                    start_scan();
                }

                err = bt_conn_le_phy_update(slot->conn, BT_CONN_LE_PHY_PARAM_2M);
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

        release_peripheral_slot_for_conn(conn);

        start_scan();
        return;
    }

    LOG_DBG("Connected: %s", log_strdup(addr));

    confirm_peripheral_slot_conn(conn);
    split_central_process_connection(conn);
}

static void split_central_disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Disconnected: %s (reason %d)", log_strdup(addr), reason);

    release_peripheral_slot_for_conn(conn);

    start_scan();
}

static struct bt_conn_cb conn_callbacks = {
    .connected = split_central_connected,
    .disconnected = split_central_disconnected,
};

K_THREAD_STACK_DEFINE(split_central_split_run_q_stack,
                      CONFIG_ZMK_BLE_SPLIT_CENTRAL_SPLIT_RUN_STACK_SIZE);

struct k_work_q split_central_split_run_q;

struct zmk_split_run_behavior_payload_wrapper {
    uint8_t source;
    struct zmk_split_run_behavior_payload payload;
};

K_MSGQ_DEFINE(zmk_split_central_split_run_msgq,
              sizeof(struct zmk_split_run_behavior_payload_wrapper),
              CONFIG_ZMK_BLE_SPLIT_CENTRAL_SPLIT_RUN_QUEUE_SIZE, 4);

void split_central_split_run_callback(struct k_work *work) {
    struct zmk_split_run_behavior_payload_wrapper payload_wrapper;

    LOG_DBG("");

    while (k_msgq_get(&zmk_split_central_split_run_msgq, &payload_wrapper, K_NO_WAIT) == 0) {
        if (peripherals[payload_wrapper.source].state != PERIPHERAL_SLOT_STATE_CONNECTED) {
            LOG_ERR("Source not connected");
            continue;
        }

        int err = bt_gatt_write_without_response(
            peripherals[payload_wrapper.source].conn,
            peripherals[payload_wrapper.source].run_behavior_handle, &payload_wrapper.payload,
            sizeof(struct zmk_split_run_behavior_payload), true);

        if (err) {
            LOG_ERR("Failed to write the behavior characteristic (err %d)", err);
        }
    }
}

K_WORK_DEFINE(split_central_split_run_work, split_central_split_run_callback);

static int
split_bt_invoke_behavior_payload(struct zmk_split_run_behavior_payload_wrapper payload_wrapper) {
    LOG_DBG("");

    int err = k_msgq_put(&zmk_split_central_split_run_msgq, &payload_wrapper, K_MSEC(100));
    if (err) {
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Consumer message queue full, popping first message and queueing again");
            struct zmk_split_run_behavior_payload_wrapper discarded_report;
            k_msgq_get(&zmk_split_central_split_run_msgq, &discarded_report, K_NO_WAIT);
            return split_bt_invoke_behavior_payload(payload_wrapper);
        }
        default:
            LOG_WRN("Failed to queue behavior to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&split_central_split_run_q, &split_central_split_run_work);

    return 0;
};

int zmk_split_bt_invoke_behavior(uint8_t source, struct zmk_behavior_binding *binding,
                                 struct zmk_behavior_binding_event event, bool state) {
    struct zmk_split_run_behavior_payload payload = {.data = {
                                                         .param1 = binding->param1,
                                                         .param2 = binding->param2,
                                                         .position = event.position,
                                                         .state = state ? 1 : 0,
                                                     }};
    const size_t payload_dev_size = sizeof(payload.behavior_dev);
    if (strlcpy(payload.behavior_dev, binding->behavior_dev, payload_dev_size) >=
        payload_dev_size) {
        LOG_ERR("Truncated behavior label %s to %s before invoking peripheral behavior",
                log_strdup(binding->behavior_dev), log_strdup(payload.behavior_dev));
    }

    struct zmk_split_run_behavior_payload_wrapper wrapper = {.source = source, .payload = payload};
    return split_bt_invoke_behavior_payload(wrapper);
}

int zmk_split_bt_central_init(const struct device *_arg) {
    k_work_q_start(&split_central_split_run_q, split_central_split_run_q_stack,
                   K_THREAD_STACK_SIZEOF(split_central_split_run_q_stack),
                   CONFIG_ZMK_BLE_THREAD_PRIORITY);
    bt_conn_cb_register(&conn_callbacks);

    return start_scan();
}

SYS_INIT(zmk_split_bt_central_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
