/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/types.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>

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
#include <zmk/events/battery_state_changed.h>
#include <zmk/pointing/input_split.h>
#include <zmk/hid_indicators_types.h>
#include <zmk/physical_layouts.h>

static int start_scanning(void);

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
    struct bt_gatt_subscribe_params sensor_subscribe_params;
    struct bt_gatt_discover_params sub_discover_params;
    uint16_t run_behavior_handle;
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
    struct bt_gatt_subscribe_params batt_lvl_subscribe_params;
    struct bt_gatt_read_params batt_lvl_read_params;
#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING) */
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    uint16_t update_hid_indicators;
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    uint16_t selected_physical_layout_handle;
    uint8_t position_state[POSITION_STATE_DATA_LEN];
    uint8_t changed_positions[POSITION_STATE_DATA_LEN];
};

#if IS_ENABLED(CONFIG_ZMK_INPUT_SPLIT)

static const struct bt_uuid *gatt_ccc_uuid = BT_UUID_GATT_CCC;
static const struct bt_uuid *gatt_cpf_uuid = BT_UUID_GATT_CPF;

struct peripheral_input_slot {
    struct bt_conn *conn;
    struct bt_gatt_subscribe_params sub;
    uint8_t reg;
};

#define COUNT_INPUT_SPLIT(n) +1

static struct peripheral_input_slot
    peripheral_input_slots[(0 DT_FOREACH_STATUS_OKAY(zmk_input_split, COUNT_INPUT_SPLIT))];

static bool input_slot_is_open(size_t i) {
    return i < ARRAY_SIZE(peripheral_input_slots) && peripheral_input_slots[i].conn == NULL;
}

static bool input_slot_is_pending(size_t i) {
    return i < ARRAY_SIZE(peripheral_input_slots) && peripheral_input_slots[i].conn != NULL &&
           (!peripheral_input_slots[i].sub.value_handle ||
            !peripheral_input_slots[i].sub.ccc_handle || !peripheral_input_slots[i].reg);
}

static int reserve_next_open_input_slot(struct peripheral_input_slot **slot, struct bt_conn *conn) {
    for (size_t i = 0; i < ARRAY_SIZE(peripheral_input_slots); i++) {
        if (input_slot_is_open(i)) {
            peripheral_input_slots[i].conn = conn;

            // Clear out any previously set values
            peripheral_input_slots[i].sub.value_handle = 0;
            peripheral_input_slots[i].sub.ccc_handle = 0;
            peripheral_input_slots[i].reg = 0;
            *slot = &peripheral_input_slots[i];
            return i;
        }
    }

    return -ENOMEM;
}

static int find_pending_input_slot(struct peripheral_input_slot **slot, struct bt_conn *conn) {
    for (size_t i = 0; i < ARRAY_SIZE(peripheral_input_slots); i++) {
        if (peripheral_input_slots[i].conn == conn && input_slot_is_pending(i)) {
            *slot = &peripheral_input_slots[i];
            return i;
        }
    }

    return -ENODEV;
}

void release_peripheral_input_subs(struct bt_conn *conn) {
    for (size_t i = 0; i < ARRAY_SIZE(peripheral_input_slots); i++) {
        if (peripheral_input_slots[i].conn == conn) {
            peripheral_input_slots[i].conn = NULL;
            // memset(&peripheral_input_slots[i], 0, sizeof(struct peripheral_input_slot));
        }
    }
}

#endif // IS_ENABLED(CONFIG_ZMK_INPUT_SPLIT)

static struct peripheral_slot peripherals[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];

static bool is_scanning = false;

static const struct bt_uuid_128 split_service_uuid = BT_UUID_INIT_128(ZMK_SPLIT_BT_SERVICE_UUID);

K_MSGQ_DEFINE(peripheral_event_msgq, sizeof(struct zmk_position_state_changed),
              CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE, 4);

void peripheral_event_work_callback(struct k_work *work) {
    struct zmk_position_state_changed ev;
    while (k_msgq_get(&peripheral_event_msgq, &ev, K_NO_WAIT) == 0) {
        LOG_DBG("Trigger key position state change for %d", ev.position);
        raise_zmk_position_state_changed(ev);
    }
}

K_WORK_DEFINE(peripheral_event_work, peripheral_event_work_callback);

int peripheral_slot_index_for_conn(struct bt_conn *conn) {
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
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
    if (index < 0 || index >= ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
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

    // Raise events releasing any active positions from this peripheral
    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {
        for (int j = 0; j < 8; j++) {
            if (slot->position_state[i] & BIT(j)) {
                uint32_t position = (i * 8) + j;
                struct zmk_position_state_changed ev = {.source = index,
                                                        .position = position,
                                                        .state = false,
                                                        .timestamp = k_uptime_get()};

                k_msgq_put(&peripheral_event_msgq, &ev, K_NO_WAIT);
                k_work_submit(&peripheral_event_work);
            }
        }
    }

    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {
        slot->position_state[i] = 0U;
        slot->changed_positions[i] = 0U;
    }

    // Clean up previously discovered handles;
    slot->subscribe_params.value_handle = 0;
    slot->run_behavior_handle = 0;
    slot->selected_physical_layout_handle = 0;
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    slot->update_hid_indicators = 0;
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

    return 0;
}

int reserve_peripheral_slot(const bt_addr_le_t *addr) {
    int i = zmk_ble_put_peripheral_addr(addr);
    if (i >= 0) {
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

#if ZMK_KEYMAP_HAS_SENSORS
K_MSGQ_DEFINE(peripheral_sensor_event_msgq, sizeof(struct zmk_sensor_event),
              CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE, 4);

void peripheral_sensor_event_work_callback(struct k_work *work) {
    struct zmk_sensor_event ev;
    while (k_msgq_get(&peripheral_sensor_event_msgq, &ev, K_NO_WAIT) == 0) {
        LOG_DBG("Trigger sensor change for %d", ev.sensor_index);
        raise_zmk_sensor_event(ev);
    }
}

K_WORK_DEFINE(peripheral_sensor_event_work, peripheral_sensor_event_work_callback);

static uint8_t split_central_sensor_notify_func(struct bt_conn *conn,
                                                struct bt_gatt_subscribe_params *params,
                                                const void *data, uint16_t length) {
    if (!data) {
        LOG_DBG("[UNSUBSCRIBED]");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[SENSOR NOTIFICATION] data %p length %u", data, length);

    if (length < offsetof(struct sensor_event, channel_data)) {
        LOG_WRN("Ignoring sensor notify with insufficient data length (%d)", length);
        return BT_GATT_ITER_STOP;
    }

    struct sensor_event sensor_event;
    memcpy(&sensor_event, data, MIN(length, sizeof(sensor_event)));
    struct zmk_sensor_event ev = {
        .sensor_index = sensor_event.sensor_index,
        .channel_data_size = MIN(sensor_event.channel_data_size, ZMK_SENSOR_EVENT_MAX_CHANNELS),
        .timestamp = k_uptime_get()};

    memcpy(ev.channel_data, sensor_event.channel_data,
           sizeof(struct zmk_sensor_channel_data) * sensor_event.channel_data_size);
    k_msgq_put(&peripheral_sensor_event_msgq, &ev, K_NO_WAIT);
    k_work_submit(&peripheral_sensor_event_work);

    return BT_GATT_ITER_CONTINUE;
}
#endif /* ZMK_KEYMAP_HAS_SENSORS */

#if IS_ENABLED(CONFIG_ZMK_INPUT_SPLIT)

struct zmk_input_event_msg {
    uint8_t reg;
    struct zmk_split_input_event_payload payload;
};

K_MSGQ_DEFINE(peripheral_input_event_msgq, sizeof(struct zmk_input_event_msg), 5, 4);
//   CONFIG_ZMK_SPLIT_BLE_CENTRAL_INPUT_QUEUE_SIZE, 4);

void peripheral_input_event_work_callback(struct k_work *work) {
    struct zmk_input_event_msg msg;
    while (k_msgq_get(&peripheral_input_event_msgq, &msg, K_NO_WAIT) == 0) {
        int ret = zmk_input_split_report_peripheral_event(
            msg.reg, msg.payload.type, msg.payload.code, msg.payload.value, msg.payload.sync);
        if (ret < 0) {
            LOG_WRN("Failed to report peripheral event %d", ret);
        }
    }
}

K_WORK_DEFINE(input_event_work, peripheral_input_event_work_callback);

static uint8_t peripheral_input_event_notify_cb(struct bt_conn *conn,
                                                struct bt_gatt_subscribe_params *params,
                                                const void *data, uint16_t length) {
    if (!data) {
        LOG_DBG("[UNSUBSCRIBED]");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[INPUT EVENT] data %p length %u", data, length);

    if (length != sizeof(struct zmk_split_input_event_payload)) {
        LOG_WRN("Ignoring input event notify with incorrect data length (%d)", length);
        return BT_GATT_ITER_STOP;
    }

    struct zmk_input_event_msg msg;

    memcpy(&msg.payload, data, MIN(length, sizeof(struct zmk_split_input_event_payload)));

    LOG_DBG("Got an input event with type %d, code %d, value %d, sync %d", msg.payload.type,
            msg.payload.code, msg.payload.value, msg.payload.sync);

    for (size_t i = 0; i < ARRAY_SIZE(peripheral_input_slots); i++) {
        if (&peripheral_input_slots[i].sub == params) {
            msg.reg = peripheral_input_slots[i].reg;
            k_msgq_put(&peripheral_input_event_msgq, &msg, K_NO_WAIT);
            k_work_submit(&input_event_work);
        }
    }

    return BT_GATT_ITER_CONTINUE;
}

#endif

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

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)

static uint8_t peripheral_battery_levels[ZMK_SPLIT_BLE_PERIPHERAL_COUNT] = {0};

int zmk_split_get_peripheral_battery_level(uint8_t source, uint8_t *level) {
    if (source >= ARRAY_SIZE(peripheral_battery_levels)) {
        return -EINVAL;
    }

    if (peripherals[source].state != PERIPHERAL_SLOT_STATE_CONNECTED) {
        return -ENOTCONN;
    }

    *level = peripheral_battery_levels[source];
    return 0;
}

K_MSGQ_DEFINE(peripheral_batt_lvl_msgq, sizeof(struct zmk_peripheral_battery_state_changed),
              CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_QUEUE_SIZE, 4);

void peripheral_batt_lvl_change_callback(struct k_work *work) {
    struct zmk_peripheral_battery_state_changed ev;
    while (k_msgq_get(&peripheral_batt_lvl_msgq, &ev, K_NO_WAIT) == 0) {
        LOG_DBG("Triggering peripheral battery level change %u", ev.state_of_charge);
        peripheral_battery_levels[ev.source] = ev.state_of_charge;
        raise_zmk_peripheral_battery_state_changed(ev);
    }
}

K_WORK_DEFINE(peripheral_batt_lvl_work, peripheral_batt_lvl_change_callback);

static uint8_t split_central_battery_level_notify_func(struct bt_conn *conn,
                                                       struct bt_gatt_subscribe_params *params,
                                                       const void *data, uint16_t length) {
    struct peripheral_slot *slot = peripheral_slot_for_conn(conn);

    if (!slot) {
        LOG_ERR("No peripheral state found for connection");
        return BT_GATT_ITER_CONTINUE;
    }

    if (!data) {
        LOG_DBG("[UNSUBSCRIBED]");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    if (length == 0) {
        LOG_ERR("Zero length battery notification received");
        return BT_GATT_ITER_CONTINUE;
    }

    LOG_DBG("[BATTERY LEVEL NOTIFICATION] data %p length %u", data, length);
    uint8_t battery_level = ((uint8_t *)data)[0];
    LOG_DBG("Battery level: %u", battery_level);
    struct zmk_peripheral_battery_state_changed ev = {
        .source = peripheral_slot_index_for_conn(conn), .state_of_charge = battery_level};
    k_msgq_put(&peripheral_batt_lvl_msgq, &ev, K_NO_WAIT);
    k_work_submit(&peripheral_batt_lvl_work);

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t split_central_battery_level_read_func(struct bt_conn *conn, uint8_t err,
                                                     struct bt_gatt_read_params *params,
                                                     const void *data, uint16_t length) {
    if (err > 0) {
        LOG_ERR("Error during reading peripheral battery level: %u", err);
        return BT_GATT_ITER_STOP;
    }

    struct peripheral_slot *slot = peripheral_slot_for_conn(conn);

    if (!slot) {
        LOG_ERR("No peripheral state found for connection");
        return BT_GATT_ITER_CONTINUE;
    }

    if (!data) {
        LOG_DBG("[READ COMPLETED]");
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[BATTERY LEVEL READ] data %p length %u", data, length);

    if (length == 0) {
        LOG_ERR("Zero length battery notification received");
        return BT_GATT_ITER_CONTINUE;
    }

    uint8_t battery_level = ((uint8_t *)data)[0];

    LOG_DBG("Battery level: %u", battery_level);

    struct zmk_peripheral_battery_state_changed ev = {
        .source = peripheral_slot_index_for_conn(conn), .state_of_charge = battery_level};
    k_msgq_put(&peripheral_batt_lvl_msgq, &ev, K_NO_WAIT);
    k_work_submit(&peripheral_batt_lvl_work);

    return BT_GATT_ITER_CONTINUE;
}

#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING) */

static int split_central_subscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params) {
    atomic_set(params->flags, BT_GATT_SUBSCRIBE_FLAG_NO_RESUB);
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

    return err;
}

static int update_peripheral_selected_layout(struct peripheral_slot *slot, uint8_t layout_idx) {
    if (slot->state != PERIPHERAL_SLOT_STATE_CONNECTED) {
        return -ENOTCONN;
    }

    if (slot->selected_physical_layout_handle == 0) {
        // It appears that sometimes the peripheral is considered connected
        // before the GATT characteristics have been discovered. If this is
        // the case, the selected_physical_layout_handle will not yet be set.
        return -EAGAIN;
    }

    if (bt_conn_get_security(slot->conn) < BT_SECURITY_L2) {
        return -EAGAIN;
    }

    int err = bt_gatt_write_without_response(slot->conn, slot->selected_physical_layout_handle,
                                             &layout_idx, sizeof(layout_idx), true);

    if (err < 0) {
        LOG_ERR("Failed to write physical layout index to peripheral (err %d)", err);
    }

    return err;
}

static void update_peripherals_selected_physical_layout(struct k_work *_work) {
    uint8_t layout_idx = zmk_physical_layouts_get_selected();
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        if (peripherals[i].state != PERIPHERAL_SLOT_STATE_CONNECTED) {
            continue;
        }

        update_peripheral_selected_layout(&peripherals[i], layout_idx);
    }
}

K_WORK_DEFINE(update_peripherals_selected_layouts_work,
              update_peripherals_selected_physical_layout);

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
    switch (params->type) {
    case BT_GATT_DISCOVER_CHARACTERISTIC: {
        const struct bt_uuid *chrc_uuid = ((struct bt_gatt_chrc *)attr->user_data)->uuid;

        if (bt_uuid_cmp(chrc_uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_POSITION_STATE_UUID)) ==
            0) {
            LOG_DBG("Found position state characteristic");
            slot->subscribe_params.disc_params = &slot->sub_discover_params;
            slot->subscribe_params.end_handle = slot->discover_params.end_handle;
            slot->subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);
            slot->subscribe_params.notify = split_central_notify_func;
            slot->subscribe_params.value = BT_GATT_CCC_NOTIFY;
            split_central_subscribe(conn, &slot->subscribe_params);
#if ZMK_KEYMAP_HAS_SENSORS
        } else if (bt_uuid_cmp(chrc_uuid,
                               BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_SENSOR_STATE_UUID)) == 0) {
            slot->discover_params.uuid = NULL;
            slot->discover_params.start_handle = attr->handle + 2;
            slot->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

            slot->sensor_subscribe_params.disc_params = &slot->sub_discover_params;
            slot->sensor_subscribe_params.end_handle = slot->discover_params.end_handle;
            slot->sensor_subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);
            slot->sensor_subscribe_params.notify = split_central_sensor_notify_func;
            slot->sensor_subscribe_params.value = BT_GATT_CCC_NOTIFY;
            split_central_subscribe(conn, &slot->sensor_subscribe_params);
#endif /* ZMK_KEYMAP_HAS_SENSORS */
#if IS_ENABLED(CONFIG_ZMK_INPUT_SPLIT)
        } else if (bt_uuid_cmp(chrc_uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_INPUT_EVENT_UUID)) ==
                   0) {
            LOG_DBG("Found an input characteristic");
            struct peripheral_input_slot *input_slot;
            int ret = reserve_next_open_input_slot(&input_slot, conn);
            if (ret < 0) {
                LOG_WRN("No available slot for peripheral input subscriptions (%d)", ret);

                slot->discover_params.uuid = NULL;
                slot->discover_params.start_handle = attr->handle + 1;
                slot->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
            } else {
                LOG_DBG("Reserved a slot for the input subscription");
                input_slot->sub.value_handle = bt_gatt_attr_value_handle(attr);

                slot->discover_params.uuid = gatt_ccc_uuid;
                slot->discover_params.start_handle = attr->handle;
                slot->discover_params.type = BT_GATT_DISCOVER_STD_CHAR_DESC;
            }
#endif // IS_ENABLED(CONFIG_ZMK_INPUT_SPLIT)
        } else if (bt_uuid_cmp(chrc_uuid,
                               BT_UUID_DECLARE_128(ZMK_SPLIT_BT_CHAR_RUN_BEHAVIOR_UUID)) == 0) {
            LOG_DBG("Found run behavior handle");
            slot->discover_params.uuid = NULL;
            slot->discover_params.start_handle = attr->handle + 2;
            slot->run_behavior_handle = bt_gatt_attr_value_handle(attr);
        } else if (!bt_uuid_cmp(((struct bt_gatt_chrc *)attr->user_data)->uuid,
                                BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SELECT_PHYS_LAYOUT_UUID))) {
            LOG_DBG("Found select physical layout handle");
            slot->selected_physical_layout_handle = bt_gatt_attr_value_handle(attr);
            k_work_submit(&update_peripherals_selected_layouts_work);
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
        } else if (!bt_uuid_cmp(((struct bt_gatt_chrc *)attr->user_data)->uuid,
                                BT_UUID_DECLARE_128(ZMK_SPLIT_BT_UPDATE_HID_INDICATORS_UUID))) {
            LOG_DBG("Found update HID indicators handle");
            slot->update_hid_indicators = bt_gatt_attr_value_handle(attr);
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
        } else if (!bt_uuid_cmp(((struct bt_gatt_chrc *)attr->user_data)->uuid,
                                BT_UUID_BAS_BATTERY_LEVEL)) {
            LOG_DBG("Found battery level characteristics");
            slot->batt_lvl_subscribe_params.disc_params = &slot->sub_discover_params;
            slot->batt_lvl_subscribe_params.end_handle = slot->discover_params.end_handle;
            slot->batt_lvl_subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);
            slot->batt_lvl_subscribe_params.notify = split_central_battery_level_notify_func;
            slot->batt_lvl_subscribe_params.value = BT_GATT_CCC_NOTIFY;
            split_central_subscribe(conn, &slot->batt_lvl_subscribe_params);

            slot->batt_lvl_read_params.func = split_central_battery_level_read_func;
            slot->batt_lvl_read_params.handle_count = 1;
            slot->batt_lvl_read_params.single.handle = bt_gatt_attr_value_handle(attr);
            slot->batt_lvl_read_params.single.offset = 0;
            bt_gatt_read(conn, &slot->batt_lvl_read_params);
#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING) */
        }
        break;
    }
    case BT_GATT_DISCOVER_STD_CHAR_DESC:
#if IS_ENABLED(CONFIG_ZMK_INPUT_SPLIT)
        if (bt_uuid_cmp(slot->discover_params.uuid, BT_UUID_GATT_CCC) == 0) {
            LOG_DBG("Found input CCC descriptor");
            struct peripheral_input_slot *input_slot;
            int ret = find_pending_input_slot(&input_slot, conn);
            if (ret < 0) {
                LOG_DBG("No pending input slot (%d)", ret);
                slot->discover_params.uuid = NULL;
                slot->discover_params.start_handle = attr->handle + 1;
                slot->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
            } else {
                LOG_DBG("Found pending input slot");
                input_slot->sub.ccc_handle = attr->handle;

                slot->discover_params.uuid = gatt_cpf_uuid;
                slot->discover_params.start_handle = attr->handle + 1;
                slot->discover_params.type = BT_GATT_DISCOVER_STD_CHAR_DESC;
            }
        } else if (bt_uuid_cmp(slot->discover_params.uuid, BT_UUID_GATT_CPF) == 0) {
            LOG_DBG("Found input CPF descriptor");
            struct bt_gatt_cpf *cpf = attr->user_data;
            struct peripheral_input_slot *input_slot;
            int ret = find_pending_input_slot(&input_slot, conn);
            if (ret < 0) {
                LOG_DBG("No pending input slot (%d)", ret);
            } else {
                LOG_DBG("Found pending input slot");
                input_slot->reg = cpf->description;
                input_slot->sub.notify = peripheral_input_event_notify_cb;
                input_slot->sub.value = BT_GATT_CCC_NOTIFY;
                int err = split_central_subscribe(conn, &input_slot->sub);
                if (err < 0) {
                    LOG_WRN("Failed to subscribe to input notifications %d", err);
                }
            }

            slot->discover_params.uuid = NULL;
            slot->discover_params.start_handle = attr->handle + 1;
            slot->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
        }
#endif // IS_ENABLED(CONFIG_ZMK_INPUT_SPLIT)
        break;
    }

    bool subscribed = slot->run_behavior_handle && slot->subscribe_params.value_handle &&
                      slot->selected_physical_layout_handle;

#if ZMK_KEYMAP_HAS_SENSORS
    subscribed = subscribed && slot->sensor_subscribe_params.value_handle;
#endif /* ZMK_KEYMAP_HAS_SENSORS */

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    subscribed = subscribed && slot->update_hid_indicators;
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
    subscribed = subscribed && slot->batt_lvl_subscribe_params.value_handle;
#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING) */
#if IS_ENABLED(CONFIG_ZMK_INPUT_SPLIT)
    for (size_t i = 0; i < ARRAY_SIZE(peripheral_input_slots); i++) {
        if (input_slot_is_open(i) || input_slot_is_pending(i)) {
            subscribed = false;
            break;
        }
    }
#endif // IS_ENABLED(CONFIG_ZMK_INPUT_SPLIT)

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

    if (bt_uuid_cmp(slot->discover_params.uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID)) !=
        0) {
        LOG_DBG("Found other service");
        return BT_GATT_ITER_CONTINUE;
    }

    LOG_DBG("Found split service");
    slot->discover_params.uuid = NULL;
    slot->discover_params.func = split_central_chrc_discovery_func;
    slot->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

    int err = bt_gatt_discover(conn, &slot->discover_params);
    if (err) {
        LOG_ERR("Failed to start discovering split service characteristics (err %d)", err);
    }
    return BT_GATT_ITER_STOP;
}

static void split_central_process_connection(struct bt_conn *conn) {
    int err;

    LOG_DBG("Current security for connection: %d", bt_conn_get_security(conn));

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

    // Restart scanning if necessary.
    start_scanning();
}

static int stop_scanning(void) {
    LOG_DBG("Stopping peripheral scanning");
    is_scanning = false;

    int err = bt_le_scan_stop();
    if (err < 0) {
        LOG_ERR("Stop LE scan failed (err %d)", err);
        return err;
    }

    return 0;
}

static bool split_central_eir_found(const bt_addr_le_t *addr) {
    LOG_DBG("Found the split service");

    // Reserve peripheral slot. Once the central has bonded to its peripherals,
    // the peripheral MAC addresses will be validated internally and the slot
    // reservation will fail if there is a mismatch.
    int slot_idx = reserve_peripheral_slot(addr);
    if (slot_idx < 0) {
        LOG_INF("Unable to reserve peripheral slot (err %d)", slot_idx);
        return false;
    }
    struct peripheral_slot *slot = &peripherals[slot_idx];

    // Stop scanning so we can connect to the peripheral device.
    int err = stop_scanning();
    if (err < 0) {
        return false;
    }

    LOG_DBG("Initiating new connection");
    struct bt_le_conn_param *param =
        BT_LE_CONN_PARAM(CONFIG_ZMK_SPLIT_BLE_PREF_INT, CONFIG_ZMK_SPLIT_BLE_PREF_INT,
                         CONFIG_ZMK_SPLIT_BLE_PREF_LATENCY, CONFIG_ZMK_SPLIT_BLE_PREF_TIMEOUT);
    err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &slot->conn);
    if (err < 0) {
        LOG_ERR("Create conn failed (err %d) (create conn? 0x%04x)", err, BT_HCI_OP_LE_CREATE_CONN);
        release_peripheral_slot(slot_idx);
        start_scanning();
    }

    return false;
}

static bool split_central_eir_parse(struct bt_data *data, void *user_data) {
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
            struct bt_uuid_128 uuid;

            if (!bt_uuid_create(&uuid.uuid, &data->data[i], 16)) {
                LOG_ERR("Unable to load UUID");
                continue;
            }

            if (bt_uuid_cmp(&uuid.uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID)) != 0) {
                char uuid_str[BT_UUID_STR_LEN];
                char service_uuid_str[BT_UUID_STR_LEN];

                bt_uuid_to_str(&uuid.uuid, uuid_str, sizeof(uuid_str));
                bt_uuid_to_str(BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID), service_uuid_str,
                               sizeof(service_uuid_str));
                LOG_DBG("UUID %s does not match split UUID: %s", uuid_str, service_uuid_str);
                continue;
            }

            return split_central_eir_found(addr);
        }
    }

    return true;
}

static void split_central_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                                       struct net_buf_simple *ad) {
    char dev[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, dev, sizeof(dev));
    LOG_DBG("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i", dev, type, ad->len, rssi);

    /* We're only interested in connectable events */
    if (type == BT_GAP_ADV_TYPE_ADV_IND) {
        bt_data_parse(ad, split_central_eir_parse, (void *)addr);
    } else if (type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        split_central_eir_found(addr);
    }
}

static int start_scanning(void) {
    // No action is necessary if central is already scanning.
    if (is_scanning) {
        LOG_DBG("Scanning already running");
        return 0;
    }

    // If all the devices are connected, there is no need to scan.
    bool has_unconnected = false;
    for (int i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (peripherals[i].conn == NULL) {
            has_unconnected = true;
            break;
        }
    }
    if (!has_unconnected) {
        LOG_DBG("All devices are connected, scanning is unnecessary");
        return 0;
    }

    // Start scanning otherwise.
    is_scanning = true;
    int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, split_central_device_found);
    if (err < 0) {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return err;
    }

    LOG_DBG("Scanning successfully started");
    return 0;
}

static void split_central_connected(struct bt_conn *conn, uint8_t conn_err) {
    char addr[BT_ADDR_LE_STR_LEN];
    struct bt_conn_info info;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    bt_conn_get_info(conn, &info);

    if (info.role != BT_CONN_ROLE_CENTRAL) {
        LOG_DBG("SKIPPING FOR ROLE %d", info.role);
        return;
    }

    if (conn_err) {
        LOG_ERR("Failed to connect to %s (%u)", addr, conn_err);

        release_peripheral_slot_for_conn(conn);

        start_scanning();
        return;
    }

    LOG_DBG("Connected: %s", addr);

    confirm_peripheral_slot_conn(conn);
    split_central_process_connection(conn);
}

static void split_central_disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Disconnected: %s (reason %d)", addr, reason);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
    struct zmk_peripheral_battery_state_changed ev = {
        .source = peripheral_slot_index_for_conn(conn), .state_of_charge = 0};
    k_msgq_put(&peripheral_batt_lvl_msgq, &ev, K_NO_WAIT);
    k_work_submit(&peripheral_batt_lvl_work);
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)

#if IS_ENABLED(CONFIG_ZMK_INPUT_SPLIT)
    release_peripheral_input_subs(conn);
#endif

    err = release_peripheral_slot_for_conn(conn);

    if (err < 0) {
        return;
    }

    start_scanning();
}

static void split_central_security_changed(struct bt_conn *conn, bt_security_t level,
                                           enum bt_security_err err) {
    struct peripheral_slot *slot = peripheral_slot_for_conn(conn);
    if (!slot || !slot->selected_physical_layout_handle) {
        return;
    }

    if (err > 0) {
        LOG_DBG("Skipping updating the physical layout for peripheral with security error");
        return;
    }

    if (level < BT_SECURITY_L2) {
        LOG_DBG("Skipping updating the physical layout for peripheral with insufficient security");
        return;
    }

    k_work_submit(&update_peripherals_selected_layouts_work);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = split_central_connected,
    .disconnected = split_central_disconnected,
    .security_changed = split_central_security_changed,
};

K_THREAD_STACK_DEFINE(split_central_split_run_q_stack,
                      CONFIG_ZMK_SPLIT_BLE_CENTRAL_SPLIT_RUN_STACK_SIZE);

struct k_work_q split_central_split_run_q;

struct zmk_split_run_behavior_payload_wrapper {
    uint8_t source;
    struct zmk_split_run_behavior_payload payload;
};

K_MSGQ_DEFINE(zmk_split_central_split_run_msgq,
              sizeof(struct zmk_split_run_behavior_payload_wrapper),
              CONFIG_ZMK_SPLIT_BLE_CENTRAL_SPLIT_RUN_QUEUE_SIZE, 4);

void split_central_split_run_callback(struct k_work *work) {
    struct zmk_split_run_behavior_payload_wrapper payload_wrapper;

    LOG_DBG("");

    while (k_msgq_get(&zmk_split_central_split_run_msgq, &payload_wrapper, K_NO_WAIT) == 0) {
        if (peripherals[payload_wrapper.source].state != PERIPHERAL_SLOT_STATE_CONNECTED) {
            LOG_ERR("Source not connected");
            continue;
        }
        if (!peripherals[payload_wrapper.source].run_behavior_handle) {
            LOG_ERR("Run behavior handle not found");
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
                                                         .source = event.source,
                                                         .state = state ? 1 : 0,
                                                     }};
    const size_t payload_dev_size = sizeof(payload.behavior_dev);
    if (strlcpy(payload.behavior_dev, binding->behavior_dev, payload_dev_size) >=
        payload_dev_size) {
        LOG_ERR("Truncated behavior label %s to %s before invoking peripheral behavior",
                binding->behavior_dev, payload.behavior_dev);
    }

    struct zmk_split_run_behavior_payload_wrapper wrapper = {.source = source, .payload = payload};
    return split_bt_invoke_behavior_payload(wrapper);
}

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

static zmk_hid_indicators_t hid_indicators = 0;

static void split_central_update_indicators_callback(struct k_work *work) {
    zmk_hid_indicators_t indicators = hid_indicators;
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        if (peripherals[i].state != PERIPHERAL_SLOT_STATE_CONNECTED) {
            continue;
        }

        if (peripherals[i].update_hid_indicators == 0) {
            // It appears that sometimes the peripheral is considered connected
            // before the GATT characteristics have been discovered. If this is
            // the case, the update_hid_indicators handle will not yet be set.
            continue;
        }

        int err = bt_gatt_write_without_response(peripherals[i].conn,
                                                 peripherals[i].update_hid_indicators, &indicators,
                                                 sizeof(indicators), true);

        if (err) {
            LOG_ERR("Failed to write HID indicator characteristic (err %d)", err);
        }
    }
}

static K_WORK_DEFINE(split_central_update_indicators, split_central_update_indicators_callback);

int zmk_split_bt_update_hid_indicator(zmk_hid_indicators_t indicators) {
    hid_indicators = indicators;
    return k_work_submit_to_queue(&split_central_split_run_q, &split_central_update_indicators);
}

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

static int finish_init() {
    return IS_ENABLED(CONFIG_ZMK_BLE_CLEAR_BONDS_ON_START) ? 0 : start_scanning();
}

#if IS_ENABLED(CONFIG_SETTINGS)

static int central_ble_handle_set(const char *name, size_t len, settings_read_cb read_cb,
                                  void *cb_arg) {
    return 0;
}

static struct settings_handler ble_central_settings_handler = {
    .name = "ble_central", .h_set = central_ble_handle_set, .h_commit = finish_init};

#endif // IS_ENABLED(CONFIG_SETTINGS)

static int zmk_split_bt_central_init(void) {
    k_work_queue_start(&split_central_split_run_q, split_central_split_run_q_stack,
                       K_THREAD_STACK_SIZEOF(split_central_split_run_q_stack),
                       CONFIG_ZMK_BLE_THREAD_PRIORITY, NULL);
    bt_conn_cb_register(&conn_callbacks);

#if IS_ENABLED(CONFIG_SETTINGS)
    settings_register(&ble_central_settings_handler);
    return 0;
#else
    return finish_init();
#endif // IS_ENABLED(CONFIG_SETTINGS)
}

SYS_INIT(zmk_split_bt_central_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);

static int zmk_split_bt_central_listener_cb(const zmk_event_t *eh) {
    if (as_zmk_physical_layout_selection_changed(eh)) {
        k_work_submit(&update_peripherals_selected_layouts_work);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(zmk_split_bt_central, zmk_split_bt_central_listener_cb);
ZMK_SUBSCRIPTION(zmk_split_bt_central, zmk_physical_layout_selection_changed);