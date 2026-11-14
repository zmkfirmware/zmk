/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include <zmk/usb.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
#include <zmk/pointing/resolution_multipliers.h>
#endif // IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
#include <zmk/hid_indicators.h>
#endif // IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)

#include <zmk/event_manager.h>

#if IS_ENABLED(CONFIG_ZMK_USB_HID_REPLAY_ON_READY)
#include <zmk/events/usb_conn_state_changed.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *hid_dev;

static K_SEM_DEFINE(hid_sem, 1, 1);

static void in_ready_cb(const struct device *dev) { k_sem_give(&hid_sem); }

#define HID_GET_REPORT_TYPE_MASK 0xff00
#define HID_GET_REPORT_ID_MASK 0x00ff

#define HID_REPORT_TYPE_INPUT 0x100
#define HID_REPORT_TYPE_OUTPUT 0x200
#define HID_REPORT_TYPE_FEATURE 0x300

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
static uint8_t hid_protocol = HID_PROTOCOL_REPORT;

static void set_proto_cb(const struct device *dev, uint8_t protocol) { hid_protocol = protocol; }

void zmk_usb_hid_set_protocol(uint8_t protocol) { hid_protocol = protocol; }
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

static uint8_t *get_keyboard_report(size_t *len) {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    if (hid_protocol != HID_PROTOCOL_REPORT) {
        zmk_hid_boot_report_t *boot_report = zmk_hid_get_boot_report();
        *len = sizeof(*boot_report);
        return (uint8_t *)boot_report;
    }
#endif
    struct zmk_hid_keyboard_report *report = zmk_hid_get_keyboard_report();
    *len = sizeof(*report);
    return (uint8_t *)report;
}

static int get_report_cb(const struct device *dev, struct usb_setup_packet *setup, int32_t *len,
                         uint8_t **data) {
    switch (setup->wValue & HID_GET_REPORT_TYPE_MASK) {
    case HID_REPORT_TYPE_FEATURE:
        switch (setup->wValue & HID_GET_REPORT_ID_MASK) {
#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
        case ZMK_HID_REPORT_ID_MOUSE:
            static struct zmk_hid_mouse_resolution_feature_report res_feature_report;

            struct zmk_endpoint_instance endpoint = {
                .transport = ZMK_TRANSPORT_USB,
            };

            *len = sizeof(struct zmk_hid_mouse_resolution_feature_report);
            struct zmk_pointing_resolution_multipliers mult =
                zmk_pointing_resolution_multipliers_get_profile(endpoint);

            res_feature_report.body.wheel_res = mult.wheel;
            res_feature_report.body.hwheel_res = mult.hor_wheel;
            *data = (uint8_t *)&res_feature_report;
            break;
#endif // IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
        default:
            return -ENOTSUP;
        }
        break;
    case HID_REPORT_TYPE_INPUT:
        switch (setup->wValue & HID_GET_REPORT_ID_MASK) {
        case ZMK_HID_REPORT_ID_KEYBOARD: {
            size_t size;
            *data = get_keyboard_report(&size);
            *len = (int32_t)size;
            break;
        }
        case ZMK_HID_REPORT_ID_CONSUMER: {
            struct zmk_hid_consumer_report *report = zmk_hid_get_consumer_report();
            *data = (uint8_t *)report;
            *len = sizeof(*report);
            break;
        }
        default:
            LOG_ERR("Invalid report ID %d requested", setup->wValue & HID_GET_REPORT_ID_MASK);
            return -EINVAL;
        }
        break;
    default:
        /*
         * 7.2.1 of the HID v1.11 spec is unclear about handling requests for reports that do not
         * exist For requested reports that aren't input reports, return -ENOTSUP like the Zephyr
         * subsys does
         */
        LOG_ERR("Unsupported report type %d requested", (setup->wValue & HID_GET_REPORT_TYPE_MASK)
                                                            << 8);
        return -ENOTSUP;
    }

    return 0;
}

static int set_report_cb(const struct device *dev, struct usb_setup_packet *setup, int32_t *len,
                         uint8_t **data) {
    switch (setup->wValue & HID_GET_REPORT_TYPE_MASK) {
    case HID_REPORT_TYPE_FEATURE:
        switch (setup->wValue & HID_GET_REPORT_ID_MASK) {
#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
        case ZMK_HID_REPORT_ID_MOUSE:
            if (*len != sizeof(struct zmk_hid_mouse_resolution_feature_report)) {
                return -EINVAL;
            }

            struct zmk_hid_mouse_resolution_feature_report *report =
                (struct zmk_hid_mouse_resolution_feature_report *)*data;
            struct zmk_endpoint_instance endpoint = {
                .transport = ZMK_TRANSPORT_USB,
            };

            zmk_pointing_resolution_multipliers_process_report(&report->body, endpoint);

            break;
#endif // IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
        default:
            return -ENOTSUP;
        }
        break;

    case HID_REPORT_TYPE_OUTPUT:
        switch (setup->wValue & HID_GET_REPORT_ID_MASK) {
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
        case ZMK_HID_REPORT_ID_LEDS:
            if (*len != sizeof(struct zmk_hid_led_report)) {
                LOG_ERR("LED set report is malformed: length=%d", *len);
                return -EINVAL;
            } else {
                struct zmk_hid_led_report *report = (struct zmk_hid_led_report *)*data;
                struct zmk_endpoint_instance endpoint = {
                    .transport = ZMK_TRANSPORT_USB,
                };
                zmk_hid_indicators_process_report(&report->body, endpoint);
            }
            break;
#endif // IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
        default:
            LOG_ERR("Invalid report ID %d requested", setup->wValue & HID_GET_REPORT_ID_MASK);
            return -EINVAL;
        }
        break;
    default:
        LOG_ERR("Unsupported report type %d requested",
                (setup->wValue & HID_GET_REPORT_TYPE_MASK) >> 8);
        return -ENOTSUP;
    }

    return 0;
}

static const struct hid_ops ops = {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    .protocol_change = set_proto_cb,
#endif
    .int_in_ready = in_ready_cb,
    .get_report = get_report_cb,
    .set_report = set_report_cb,
};

#if IS_ENABLED(CONFIG_ZMK_USB_HID_REPLAY_ON_READY)

// Queue HID reports that arrive while the USB endpoint is not ready, then
// flush them shortly after USB transitions back to a ready state. Without
// queueing, USB_DC_SUSPEND drops every report (returns usb_wakeup_request)
// and the host HID stack additionally drops the first input report from a
// freshly bound interface as a binding-race quirk. The flush delay covers
// the host-side binding race by letting the host attach before we resend.

#define ZMK_USB_HID_REPLAY_QUEUE_DEPTH       CONFIG_ZMK_USB_HID_REPLAY_QUEUE_DEPTH
#define ZMK_USB_HID_REPLAY_REPORT_MAX_LEN    CONFIG_ZMK_USB_HID_REPLAY_REPORT_MAX_LEN

struct pending_report {
    uint8_t data[ZMK_USB_HID_REPLAY_REPORT_MAX_LEN];
    uint8_t len;
};

static struct pending_report pending_queue[ZMK_USB_HID_REPLAY_QUEUE_DEPTH];
static uint8_t pending_head;
static uint8_t pending_tail;
static K_MUTEX_DEFINE(pending_mutex);

static void enqueue_pending_report(const uint8_t *report, size_t len) {
    if (len > ZMK_USB_HID_REPLAY_REPORT_MAX_LEN) {
        LOG_WRN("HID report too large to queue (%zu B), dropped", len);
        return;
    }
    k_mutex_lock(&pending_mutex, K_FOREVER);
    uint8_t next = (pending_tail + 1) % ZMK_USB_HID_REPLAY_QUEUE_DEPTH;
    if (next == pending_head) {
        // queue full: advance head to drop oldest
        pending_head = (pending_head + 1) % ZMK_USB_HID_REPLAY_QUEUE_DEPTH;
    }
    memcpy(pending_queue[pending_tail].data, report, len);
    pending_queue[pending_tail].len = (uint8_t)len;
    pending_tail = next;
    k_mutex_unlock(&pending_mutex);
}

static void flush_pending_reports(struct k_work *_work);
K_WORK_DELAYABLE_DEFINE(pending_flush_dwork, flush_pending_reports);

static void flush_pending_reports(struct k_work *_work) {
    bool retry = false;
    k_mutex_lock(&pending_mutex, K_FOREVER);
    while (pending_head != pending_tail) {
        struct pending_report *slot = &pending_queue[pending_head];
        // Advisory take, mirroring the normal send path (which ignores the
        // take result). A bus reset aborts an armed IN transfer without
        // raising the write-complete callback (usb_dc_nrfx EP_ABORTED is
        // log-only), stranding the semaphore at 0; blocking on it here
        // would abandon the queue for the whole wake cycle. Proceeding
        // lets the next completed write re-give the semaphore.
        int took = k_sem_take(&hid_sem, K_MSEC(50));
        int err = hid_int_ep_write(hid_dev, slot->data, slot->len, NULL);
        if (err) {
            if (took == 0) {
                k_sem_give(&hid_sem);
            }
            retry = true;
            break;
        }
        pending_head = (pending_head + 1) % ZMK_USB_HID_REPLAY_QUEUE_DEPTH;
    }
    k_mutex_unlock(&pending_mutex);

    // -EAGAIN here usually means mid-enumeration (reset seen, not yet
    // configured) or a transfer still armed. Retry on a short timer while
    // the bus is active instead of waiting for a status change that may
    // never come; during suspend the next wake reschedules us anyway.
    if (retry) {
        switch (zmk_usb_get_status()) {
        case USB_DC_CONFIGURED:
        case USB_DC_RESUME:
        case USB_DC_CLEAR_HALT:
            k_work_reschedule(&pending_flush_dwork,
                              K_MSEC(CONFIG_ZMK_USB_HID_REPLAY_FLUSH_DELAY_MS));
            break;
        default:
            break;
        }
    }
}

static int usb_hid_conn_state_listener(const zmk_event_t *eh) {
    const struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    if (!ev) {
        return 0;
    }
    switch (zmk_usb_get_status()) {
    case USB_DC_CONFIGURED:
    case USB_DC_RESUME:
        // reschedule (not schedule): k_work_schedule on an already-pending
        // work keeps the OLD deadline, so a RESUME -> RESET -> CONFIGURED
        // cluster would fire the flush ~0ms after CONFIGURED — inside the
        // host's HID binding race the delay exists to avoid. Restart the
        // grace window from the last transition instead.
        if (zmk_usb_is_hid_ready()) {
            k_work_reschedule(&pending_flush_dwork,
                              K_MSEC(CONFIG_ZMK_USB_HID_REPLAY_FLUSH_DELAY_MS));
        }
        break;
    case USB_DC_DISCONNECTED:
        // Physically unplugged: drop pending reports. Replaying hours-old
        // keystrokes on replug is phantom input, not recovery. (RESET /
        // CONNECTED keep the queue — they are transient on the wake path.)
        k_work_cancel_delayable(&pending_flush_dwork);
        k_mutex_lock(&pending_mutex, K_FOREVER);
        pending_head = pending_tail;
        k_mutex_unlock(&pending_mutex);
        break;
    default:
        // USB_DC_SUSPEND still maps to ZMK_USB_CONN_HID with is_configured
        // true; scheduling a flush on suspend entry is useless and feeds
        // the deadline-collapse above.
        break;
    }
    return 0;
}

ZMK_LISTENER(usb_hid_conn_state_listener, usb_hid_conn_state_listener);
ZMK_SUBSCRIPTION(usb_hid_conn_state_listener, zmk_usb_conn_state_changed);

#endif // CONFIG_ZMK_USB_HID_REPLAY_ON_READY

static int zmk_usb_hid_send_report(const uint8_t *report, size_t len) {
    switch (zmk_usb_get_status()) {
    case USB_DC_SUSPEND:
#if IS_ENABLED(CONFIG_ZMK_USB_HID_REPLAY_ON_READY)
        enqueue_pending_report(report, len);
#endif
        return usb_wakeup_request();
    case USB_DC_ERROR:
    case USB_DC_RESET:
    case USB_DC_DISCONNECTED:
    case USB_DC_UNKNOWN:
    case USB_DC_CONNECTED:
#if IS_ENABLED(CONFIG_ZMK_USB_HID_REPLAY_ON_READY)
        enqueue_pending_report(report, len);
#endif
        return -ENODEV;
    default:
#if IS_ENABLED(CONFIG_ZMK_USB_HID_REPLAY_ON_READY)
        // Don't overtake reports still queued from a not-ready window: a
        // live release racing past its queued press would leave that key
        // logically held on the host. Append and let the flush deliver
        // both in order (k_work_schedule keeps an already-pending grace
        // deadline; schedules immediately when none is pending).
        if (pending_head != pending_tail) {
            enqueue_pending_report(report, len);
            k_work_schedule(&pending_flush_dwork, K_NO_WAIT);
            return 0;
        }
#endif
        k_sem_take(&hid_sem, K_MSEC(30));
        int err = hid_int_ep_write(hid_dev, report, len, NULL);

        if (err) {
            k_sem_give(&hid_sem);
        }

        return err;
    }
}

int zmk_usb_hid_send_keyboard_report(void) {
    size_t len;
    uint8_t *report = get_keyboard_report(&len);
    return zmk_usb_hid_send_report(report, len);
}

int zmk_usb_hid_send_consumer_report(void) {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    if (hid_protocol == HID_PROTOCOL_BOOT) {
        return -ENOTSUP;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

    struct zmk_hid_consumer_report *report = zmk_hid_get_consumer_report();
    return zmk_usb_hid_send_report((uint8_t *)report, sizeof(*report));
}

#if IS_ENABLED(CONFIG_ZMK_POINTING)
int zmk_usb_hid_send_mouse_report() {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    if (hid_protocol == HID_PROTOCOL_BOOT) {
        return -ENOTSUP;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

    struct zmk_hid_mouse_report *report = zmk_hid_get_mouse_report();
    return zmk_usb_hid_send_report((uint8_t *)report, sizeof(*report));
}
#endif // IS_ENABLED(CONFIG_ZMK_POINTING)

static int zmk_usb_hid_init(void) {
    hid_dev = device_get_binding("HID_0");
    if (hid_dev == NULL) {
        LOG_ERR("Unable to locate HID device");
        return -EINVAL;
    }

    usb_hid_register_device(hid_dev, zmk_hid_report_desc, sizeof(zmk_hid_report_desc), &ops);

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    usb_hid_set_proto_code(hid_dev, HID_BOOT_IFACE_CODE_KEYBOARD);
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

    usb_hid_init(hid_dev);

    return 0;
}

SYS_INIT(zmk_usb_hid_init, APPLICATION, CONFIG_ZMK_USB_HID_INIT_PRIORITY);
