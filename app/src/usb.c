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

static const struct hid_ops ops = {
    .int_in_ready = in_ready_cb,
};

int zmk_usb_hid_send_report(const uint8_t *report, size_t len) {
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
