/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <init.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static enum usb_dc_status_code usb_status = USB_DC_UNKNOWN;

static void raise_usb_status_changed_event(struct k_work *_work) {
    ZMK_EVENT_RAISE(new_zmk_usb_conn_state_changed(
        (struct zmk_usb_conn_state_changed){.conn_state = zmk_usb_get_conn_state()}));
}

K_WORK_DEFINE(usb_status_notifier_work, raise_usb_status_changed_event);

#ifdef CONFIG_ZMK_USB

static const struct device *hid_dev;

static K_SEM_DEFINE(hid_sem, 1, 1);

static void in_ready_cb(const struct device *dev) { k_sem_give(&hid_sem); }

#define HID_GET_REPORT_TYPE_MASK 0xff00
#define HID_GET_REPORT_ID_MASK   0x00ff

#define HID_REPORT_TYPE_INPUT    0x100
#define HID_REPORT_TYPE_OUTPUT   0x200
#define HID_REPORT_TYPE_FEATURE  0x300

static int zmk_usb_get_report(const struct device *dev,
                             struct usb_setup_packet *setup, int32_t *len,
                             uint8_t **data) {

    /*
     * 7.2.1 of the HID v1.11 spec is unclear about handling requests for reports that do not exist
     * For requested reports that aren't input reports, return -ENOTSUP like the Zephyr subsys does
     */
    if ((setup->wValue & HID_GET_REPORT_TYPE_MASK) != HID_REPORT_TYPE_INPUT) {
        LOG_ERR("Unsupported report type %d requested", (setup->wValue & HID_GET_REPORT_TYPE_MASK) << 8);
        return -ENOTSUP;
    }

    uint8_t *report = NULL;
    switch (setup->wValue & HID_GET_REPORT_ID_MASK) {
        case HID_REPORT_ID_KEYBOARD:
            report = zmk_hid_get_keyboard_report(HID_REPORT_FULL, HID_PROTOCOL_REPORT);
            *len = sizeof(struct zmk_hid_keyboard_report);
            break;
        case HID_REPORT_ID_CONSUMER:
            report = zmk_hid_get_consumer_report(HID_REPORT_FULL);
            *len = sizeof(struct zmk_hid_consumer_report);
            break;
        default:
            LOG_ERR("Invalid report ID %d requested", setup->wValue & HID_GET_REPORT_ID_MASK);
            return -EINVAL;
    }

    *data = report;
    return 0;
}

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
static uint8_t hid_protocol = HID_PROTOCOL_REPORT;
void zmk_usb_set_proto_cb(const struct device *dev, uint8_t protocol) {
    hid_protocol = protocol;
}
#endif

static const struct hid_ops ops = {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    .protocol_change = zmk_usb_set_proto_cb,
#endif
    .int_in_ready = in_ready_cb,
    .get_report = zmk_usb_get_report
};

int zmk_usb_hid_send_report(uint8_t report_id) {
    switch (usb_status) {
    case USB_DC_SUSPEND:
        return usb_wakeup_request();
    case USB_DC_ERROR:
    case USB_DC_RESET:
    case USB_DC_DISCONNECTED:
    case USB_DC_UNKNOWN:
        return -ENODEV;
    default:
        k_sem_take(&hid_sem, K_MSEC(30));
        uint8_t *report;
        size_t len;

        if (report_id == HID_REPORT_ID_KEYBOARD) {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
            report = zmk_hid_get_keyboard_report(HID_REPORT_FULL, hid_protocol);
            len = hid_protocol == HID_PROTOCOL_BOOT ? sizeof(zmk_hid_boot_report_t) : sizeof(struct zmk_hid_keyboard_report);
#else
            report = zmk_hid_get_keyboard_report(HID_REPORT_FULL, HID_PROTOCOL_REPORT);
            len = sizeof(struct zmk_hid_keyboard_report);
#endif
        } else {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
            if (hid_protocol == HID_PROTOCOL_BOOT) {
                return -ENOTSUP;
            }
#endif
            report = zmk_hid_get_consumer_report(HID_REPORT_FULL);
            len = sizeof(struct zmk_hid_consumer_report);
        }

        int err = hid_int_ep_write(hid_dev, report, len, NULL);

        if (err) {
            k_sem_give(&hid_sem);
        }

        return err;
    }
}

#endif /* CONFIG_ZMK_USB */

enum usb_dc_status_code zmk_usb_get_status() { return usb_status; }

enum zmk_usb_conn_state zmk_usb_get_conn_state() {
    LOG_DBG("state: %d", usb_status);
    switch (usb_status) {
    case USB_DC_DISCONNECTED:
    case USB_DC_UNKNOWN:
        return ZMK_USB_CONN_NONE;

    case USB_DC_ERROR:
    case USB_DC_RESET:
        return ZMK_USB_CONN_POWERED;

    default:
        return ZMK_USB_CONN_HID;
    }
}

void usb_status_cb(enum usb_dc_status_code status, const uint8_t *params) {
#ifdef CONFIG_ZMK_USB_BOOT
    if (status == USB_DC_RESET) {
        hid_protocol = HID_PROTOCOL_REPORT;
    }
#endif
    usb_status = status;
    k_work_submit(&usb_status_notifier_work);
};

static int zmk_usb_init(const struct device *_arg) {
    int usb_enable_ret;

#ifdef CONFIG_ZMK_USB
    hid_dev = device_get_binding("HID_0");
    if (hid_dev == NULL) {
        LOG_ERR("Unable to locate HID device");
        return -EINVAL;
    }

    usb_hid_register_device(hid_dev, zmk_hid_report_desc, sizeof(zmk_hid_report_desc), &ops);

    usb_hid_init(hid_dev);

#endif /* CONFIG_ZMK_USB */

    usb_enable_ret = usb_enable(usb_status_cb);

    if (usb_enable_ret != 0) {
        LOG_ERR("Unable to enable USB");
        return -EINVAL;
    }

    return 0;
}

SYS_INIT(zmk_usb_init, APPLICATION, CONFIG_ZMK_USB_INIT_PRIORITY);
