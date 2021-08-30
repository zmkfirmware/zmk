/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <settings/settings.h>
#include <init.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include <zmk/ble.h>
#include <zmk/hog.h>
#include <zmk/hid.h>

enum {
    HIDS_REMOTE_WAKE = BIT(0),
    HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info {
    uint16_t version; /* version number of base USB HID Specification */
    uint8_t code;     /* country HID Device hardware is localized for. */
    uint8_t flags;
} __packed;

struct hids_report {
    uint8_t id;   /* report id */
    uint8_t type; /* report type */
} __packed;

static struct hids_info info = {
    .version = 0x0000,
    .code = 0x00,
    .flags = HIDS_NORMALLY_CONNECTABLE | HIDS_REMOTE_WAKE,
};

enum {
    HIDS_INPUT = 0x01,
    HIDS_OUTPUT = 0x02,
    HIDS_FEATURE = 0x03,
};

static struct hids_report input = {
    .id = 0x01,
    .type = HIDS_INPUT,
};

static struct hids_report consumer_input = {
    .id = 0x02,
    .type = HIDS_INPUT,
};

static struct hids_report mouse_input = {
    .id = 0x04,
    .type = HIDS_INPUT,
};

static bool host_requests_notification = false;
static uint8_t ctrl_point;
// static uint8_t proto_mode;

static ssize_t read_hids_info(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                              uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
                             sizeof(struct hids_info));
}

static ssize_t read_hids_report_ref(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                    void *buf, uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
                             sizeof(struct hids_report));
}

static ssize_t read_hids_report_map(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                    void *buf, uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, zmk_hid_report_desc,
                             sizeof(zmk_hid_report_desc));
}

static ssize_t read_hids_input_report(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                      void *buf, uint16_t len, uint16_t offset) {
    struct zmk_hid_keyboard_report_body *report_body = &zmk_hid_get_keyboard_report()->body;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, report_body,
                             sizeof(struct zmk_hid_keyboard_report_body));
}

static ssize_t read_hids_consumer_input_report(struct bt_conn *conn,
                                               const struct bt_gatt_attr *attr, void *buf,
                                               uint16_t len, uint16_t offset) {
    struct zmk_hid_consumer_report_body *report_body = &zmk_hid_get_consumer_report()->body;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, report_body,
                             sizeof(struct zmk_hid_consumer_report_body));
}

static ssize_t read_hids_mouse_input_report(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                            void *buf, uint16_t len, uint16_t offset) {
    struct zmk_hid_mouse_report_body *report_body = &zmk_hid_get_mouse_report()->body;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, report_body,
                             sizeof(struct zmk_hid_mouse_report_body));
}

// static ssize_t write_proto_mode(struct bt_conn *conn,
//                                 const struct bt_gatt_attr *attr,
//                                 const void *buf, uint16_t len, uint16_t offset,
//                                 uint8_t flags)
// {
//     printk("PROTO CHANGED\n");
//     return 0;
// }

static void input_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    host_requests_notification = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t write_ctrl_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
    uint8_t *value = attr->user_data;

    if (offset + len > sizeof(ctrl_point)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    return len;
}

/* HID Service Declaration */
BT_GATT_SERVICE_DEFINE(
    hog_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
    //    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_PROTOCOL_MODE, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
    //                           BT_GATT_PERM_WRITE, NULL, write_proto_mode, &proto_mode),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_hids_info,
                           NULL, &info),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
                           read_hids_report_map, NULL, NULL),

    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ_ENCRYPT, read_hids_input_report, NULL, NULL),
    BT_GATT_CCC(input_ccc_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT, read_hids_report_ref,
                       NULL, &input),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ_ENCRYPT, read_hids_consumer_input_report, NULL, NULL),
    BT_GATT_CCC(input_ccc_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT, read_hids_report_ref,
                       NULL, &consumer_input),

    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ_ENCRYPT, read_hids_mouse_input_report, NULL, NULL),
    BT_GATT_CCC(input_ccc_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT, read_hids_report_ref,
                       NULL, &mouse_input),

    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, write_ctrl_point, &ctrl_point));

struct bt_conn *destination_connection() {
    struct bt_conn *conn;
    bt_addr_le_t *addr = zmk_ble_active_profile_addr();
    LOG_DBG("Address pointer %p", addr);
    if (!bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
        LOG_WRN("Not sending, no active address for current profile");
        return NULL;
    } else if ((conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr)) == NULL) {
        LOG_WRN("Not sending, not connected to active profile");
        return NULL;
    }

    return conn;
}

K_THREAD_STACK_DEFINE(hog_q_stack, CONFIG_ZMK_BLE_THREAD_STACK_SIZE);

struct k_work_q hog_work_q;

K_MSGQ_DEFINE(zmk_hog_keyboard_msgq, sizeof(struct zmk_hid_keyboard_report_body),
              CONFIG_ZMK_BLE_KEYBOARD_REPORT_QUEUE_SIZE, 4);

void send_keyboard_report_callback(struct k_work *work) {
    struct zmk_hid_keyboard_report_body report;

    while (k_msgq_get(&zmk_hog_keyboard_msgq, &report, K_NO_WAIT) == 0) {
        struct bt_conn *conn = destination_connection();
        if (conn == NULL) {
            return;
        }

        struct bt_gatt_notify_params notify_params = {
            .attr = &hog_svc.attrs[5],
            .data = &report,
            .len = sizeof(report),
        };

        int err = bt_gatt_notify_cb(conn, &notify_params);
        if (err) {
            LOG_ERR("Error notifying %d", err);
        }

        bt_conn_unref(conn);
    }
}

K_WORK_DEFINE(hog_keyboard_work, send_keyboard_report_callback);

int zmk_hog_send_keyboard_report(struct zmk_hid_keyboard_report_body *report) {
    int err = k_msgq_put(&zmk_hog_keyboard_msgq, report, K_MSEC(100));
    if (err) {
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Keyboard message queue full, popping first message and queueing again");
            struct zmk_hid_keyboard_report_body discarded_report;
            k_msgq_get(&zmk_hog_keyboard_msgq, &discarded_report, K_NO_WAIT);
            return zmk_hog_send_keyboard_report(report);
        }
        default:
            LOG_WRN("Failed to queue keyboard report to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&hog_work_q, &hog_keyboard_work);

    return 0;
};

K_MSGQ_DEFINE(zmk_hog_consumer_msgq, sizeof(struct zmk_hid_consumer_report_body),
              CONFIG_ZMK_BLE_CONSUMER_REPORT_QUEUE_SIZE, 4);

void send_consumer_report_callback(struct k_work *work) {
    struct zmk_hid_consumer_report_body report;

    while (k_msgq_get(&zmk_hog_consumer_msgq, &report, K_NO_WAIT) == 0) {
        struct bt_conn *conn = destination_connection();
        if (conn == NULL) {
            return;
        }

        struct bt_gatt_notify_params notify_params = {
            .attr = &hog_svc.attrs[10],
            .data = &report,
            .len = sizeof(report),
        };

        int err = bt_gatt_notify_cb(conn, &notify_params);
        if (err) {
            LOG_DBG("Error notifying %d", err);
        }

        bt_conn_unref(conn);
    }
};

K_WORK_DEFINE(hog_consumer_work, send_consumer_report_callback);

int zmk_hog_send_consumer_report(struct zmk_hid_consumer_report_body *report) {
    int err = k_msgq_put(&zmk_hog_consumer_msgq, report, K_MSEC(100));
    if (err) {
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Consumer message queue full, popping first message and queueing again");
            struct zmk_hid_consumer_report_body discarded_report;
            k_msgq_get(&zmk_hog_consumer_msgq, &discarded_report, K_NO_WAIT);
            return zmk_hog_send_consumer_report(report);
        }
        default:
            LOG_WRN("Failed to queue consumer report to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&hog_work_q, &hog_consumer_work);

    return 0;
};

K_MSGQ_DEFINE(zmk_hog_mouse_msgq, sizeof(struct zmk_hid_mouse_report_body),
              CONFIG_ZMK_BLE_MOUSE_REPORT_QUEUE_SIZE, 4);

void send_mouse_report_callback(struct k_work *work) {
    struct zmk_hid_mouse_report_body report;
    while (k_msgq_get(&zmk_hog_mouse_msgq, &report, K_NO_WAIT) == 0) {
        struct bt_conn *conn = destination_connection();
        if (conn == NULL) {
            return;
        }

        struct bt_gatt_notify_params notify_params = {
            .attr = &hog_svc.attrs[13],
            .data = &report,
            .len = sizeof(report),
        };

        int err = bt_gatt_notify_cb(conn, &notify_params);
        if (err) {
            LOG_DBG("Error notifying %d", err);
        }

        bt_conn_unref(conn);
    }
};

K_WORK_DEFINE(hog_mouse_work, send_mouse_report_callback);

int zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *report) {
    int err = k_msgq_put(&zmk_hog_mouse_msgq, report, K_NO_WAIT);
    if (err) {
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Mouse message queue full, dropping report");
            return err;
        }
        default:
            LOG_WRN("Failed to queue mouse report to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&hog_work_q, &hog_mouse_work);

    return 0;
};

int zmk_hog_init(const struct device *_arg) {
    k_work_q_start(&hog_work_q, hog_q_stack, K_THREAD_STACK_SIZEOF(hog_q_stack),
                   CONFIG_ZMK_BLE_THREAD_PRIORITY);

    return 0;
}

SYS_INIT(zmk_hog_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
