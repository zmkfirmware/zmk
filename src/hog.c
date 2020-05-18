#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include <zmk/ble.h>
#include <zmk/hog.h>
#include <zmk/hid.h>

int zmk_hog_init()
{
    return zmk_ble_init();
}

enum
{
    HIDS_REMOTE_WAKE = BIT(0),
    HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info
{
    u16_t version; /* version number of base USB HID Specification */
    u8_t code;     /* country HID Device hardware is localized for. */
    u8_t flags;
} __packed;

struct hids_report
{
    u8_t id;   /* report id */
    u8_t type; /* report type */
} __packed;

static struct hids_info info = {
    .version = 0x0000,
    .code = 0x00,
    .flags = HIDS_NORMALLY_CONNECTABLE & HIDS_REMOTE_WAKE,
};

enum
{
    HIDS_INPUT = 0x01,
    HIDS_OUTPUT = 0x02,
    HIDS_FEATURE = 0x03,
};

static struct hids_report input = {
    .id = 0x01,
    .type = HIDS_INPUT,
};

static bool host_requests_notification = false;
static u8_t ctrl_point;
// static u8_t proto_mode;

static ssize_t read_hids_info(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(struct hids_info));
}

static ssize_t read_hids_report_ref(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(struct hids_report));
}

static ssize_t read_hids_report_map(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, zmk_hid_report_desc, sizeof(zmk_hid_report_desc));
}

static ssize_t read_hids_input_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset)
{
    struct zmk_hid_report *report = zmk_hid_get_report();
    return bt_gatt_attr_read(conn, attr, buf, len, offset, report, sizeof(struct zmk_hid_report));
}

// static ssize_t write_proto_mode(struct bt_conn *conn,
//                                 const struct bt_gatt_attr *attr,
//                                 const void *buf, u16_t len, u16_t offset,
//                                 u8_t flags)
// {
//     printk("PROTO CHANGED\n");
//     return 0;
// }

static void input_ccc_changed(const struct bt_gatt_attr *attr, u16_t value)
{
    host_requests_notification = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
                                const struct bt_gatt_attr *attr,
                                const void *buf, u16_t len, u16_t offset,
                                u8_t flags)
{
    u8_t *value = attr->user_data;

    if (offset + len > sizeof(ctrl_point))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    return len;
}

/* HID Service Declaration */
BT_GATT_SERVICE_DEFINE(hog_svc,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
                       //    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_PROTOCOL_MODE, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                       //                           BT_GATT_PERM_WRITE, NULL, write_proto_mode, &proto_mode),
                       BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ,
                                              BT_GATT_PERM_READ, read_hids_info, NULL, &info),
                       BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ,
                                              BT_GATT_PERM_READ, read_hids_report_map, NULL, NULL),

                       BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
                                              BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                                              BT_GATT_PERM_READ_ENCRYPT,
                                              read_hids_input_report, NULL, NULL),
                       BT_GATT_CCC(input_ccc_changed,
                                   BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
                       BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
                                          read_hids_report_ref, NULL, &input),
                       BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT,
                                              BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                                              BT_GATT_PERM_WRITE,
                                              NULL, write_ctrl_point, &ctrl_point));

int zmk_hog_send_report(struct zmk_hid_report *report)
{
    return bt_gatt_notify(NULL, &hog_svc.attrs[5], report, sizeof(struct zmk_hid_report));
};
