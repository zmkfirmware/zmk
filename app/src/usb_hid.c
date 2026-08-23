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

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define ZMK_USB_HID_DEVICE_NAME(index) "HID_" STRINGIFY(index)
#define ZMK_USB_KEYBOARD_HID_DEVICE                                                                \
    ZMK_USB_HID_DEVICE_NAME(CONFIG_ZMK_USB_KEYBOARD_HID_DEVICE_INDEX)
#define ZMK_USB_CONSUMER_HID_DEVICE                                                                \
    ZMK_USB_HID_DEVICE_NAME(CONFIG_ZMK_USB_CONSUMER_HID_DEVICE_INDEX)
#if IS_ENABLED(CONFIG_ZMK_POINTING)
#define ZMK_USB_POINTING_HID_DEVICE                                                                \
    ZMK_USB_HID_DEVICE_NAME(CONFIG_ZMK_USB_POINTING_HID_DEVICE_INDEX)
#endif

BUILD_ASSERT(CONFIG_ZMK_USB_KEYBOARD_HID_DEVICE_INDEX >= 0 &&
                 CONFIG_ZMK_USB_KEYBOARD_HID_DEVICE_INDEX < CONFIG_USB_HID_DEVICE_COUNT,
             "Keyboard HID device index must be between 0 and USB_HID_DEVICE_COUNT - 1");
BUILD_ASSERT(CONFIG_ZMK_USB_CONSUMER_HID_DEVICE_INDEX >= 0 &&
                 CONFIG_ZMK_USB_CONSUMER_HID_DEVICE_INDEX < CONFIG_USB_HID_DEVICE_COUNT,
             "Consumer HID device index must be between 0 and USB_HID_DEVICE_COUNT - 1");
#if IS_ENABLED(CONFIG_ZMK_POINTING)
BUILD_ASSERT(CONFIG_ZMK_USB_POINTING_HID_DEVICE_INDEX >= 0 &&
                 CONFIG_ZMK_USB_POINTING_HID_DEVICE_INDEX < CONFIG_USB_HID_DEVICE_COUNT,
             "Pointing HID device index must be between 0 and USB_HID_DEVICE_COUNT - 1");
#endif

#define ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED                                                       \
    (CONFIG_ZMK_USB_KEYBOARD_HID_DEVICE_INDEX == CONFIG_ZMK_USB_CONSUMER_HID_DEVICE_INDEX)
#if IS_ENABLED(CONFIG_ZMK_POINTING)
#define ZMK_USB_KEYBOARD_POINTING_HID_SHARED                                                       \
    (CONFIG_ZMK_USB_KEYBOARD_HID_DEVICE_INDEX == CONFIG_ZMK_USB_POINTING_HID_DEVICE_INDEX)
#define ZMK_USB_CONSUMER_POINTING_HID_SHARED                                                       \
    (CONFIG_ZMK_USB_CONSUMER_HID_DEVICE_INDEX == CONFIG_ZMK_USB_POINTING_HID_DEVICE_INDEX)
#define ZMK_USB_KEYBOARD_HID_USES_REPORT_IDS                                                       \
    (ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED || ZMK_USB_KEYBOARD_POINTING_HID_SHARED)
#else
#define ZMK_USB_KEYBOARD_HID_USES_REPORT_IDS ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED
#endif

static const struct device *keyboard_hid_dev;
static const struct device *consumer_hid_dev;
#if IS_ENABLED(CONFIG_ZMK_POINTING)
static const struct device *mouse_hid_dev;
#endif

static K_SEM_DEFINE(keyboard_hid_sem, 1, 1);
static K_SEM_DEFINE(consumer_hid_sem, 1, 1);
#if IS_ENABLED(CONFIG_ZMK_POINTING)
static K_SEM_DEFINE(mouse_hid_sem, 1, 1);
#endif

static struct k_sem *keyboard_hid_sem_ptr;
static struct k_sem *consumer_hid_sem_ptr;
#if IS_ENABLED(CONFIG_ZMK_POINTING)
static struct k_sem *mouse_hid_sem_ptr;
#endif

static struct k_sem *sem_for_hid_dev(const struct device *dev) {
    if (dev == keyboard_hid_dev) {
        return keyboard_hid_sem_ptr;
    }

#if !ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED
    if (dev == consumer_hid_dev) {
        return consumer_hid_sem_ptr;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_POINTING) && !ZMK_USB_KEYBOARD_POINTING_HID_SHARED &&                    \
    !ZMK_USB_CONSUMER_POINTING_HID_SHARED
    if (dev == mouse_hid_dev) {
        return mouse_hid_sem_ptr;
    }
#endif

    return keyboard_hid_sem_ptr;
}

static void in_ready_cb(const struct device *dev) { k_sem_give(sem_for_hid_dev(dev)); }

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

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
static int process_led_report(int32_t len, size_t expected_len,
                              struct zmk_hid_led_report_body *body) {
    if (len != expected_len) {
        LOG_ERR("LED set report is malformed: length=%d", len);
        return -EINVAL;
    }

    struct zmk_endpoint_instance endpoint = {
        .transport = ZMK_TRANSPORT_USB,
    };
    zmk_hid_indicators_process_report(body, endpoint);
    return 0;
}
#endif // IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)

static uint8_t *get_keyboard_report(size_t *len) {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    if (hid_protocol != HID_PROTOCOL_REPORT) {
        zmk_hid_boot_report_t *boot_report = zmk_hid_get_boot_report();
        *len = sizeof(*boot_report);
        return (uint8_t *)boot_report;
    }
#endif
    struct zmk_hid_keyboard_report *report = zmk_hid_get_keyboard_report();
    if (!ZMK_USB_KEYBOARD_HID_USES_REPORT_IDS) {
        *len = sizeof(report->body);
        return (uint8_t *)&report->body;
    } else {
        *len = sizeof(*report);
        return (uint8_t *)report;
    }
}

static int get_report_cb(const struct device *dev, struct usb_setup_packet *setup, int32_t *len,
                         uint8_t **data) {
    switch (setup->wValue & HID_GET_REPORT_TYPE_MASK) {
    case HID_REPORT_TYPE_FEATURE:
        switch (setup->wValue & HID_GET_REPORT_ID_MASK) {
#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
        case ZMK_HID_REPORT_ID_MOUSE:
            if (dev != mouse_hid_dev) {
                return -EINVAL;
            }
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
        case 0: {
            if (ZMK_USB_KEYBOARD_HID_USES_REPORT_IDS || dev != keyboard_hid_dev) {
                return -EINVAL;
            }
            size_t size;
            *data = get_keyboard_report(&size);
            *len = (int32_t)size;
            break;
        }
        case ZMK_HID_REPORT_ID_KEYBOARD: {
            if (!ZMK_USB_KEYBOARD_HID_USES_REPORT_IDS || dev != keyboard_hid_dev) {
                return -EINVAL;
            }
            size_t size;
            *data = get_keyboard_report(&size);
            *len = (int32_t)size;
            break;
        }
        case ZMK_HID_REPORT_ID_CONSUMER: {
            if (dev != consumer_hid_dev) {
                return -EINVAL;
            }
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
            if (dev != mouse_hid_dev) {
                return -EINVAL;
            }
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
        case 0: {
            if (ZMK_USB_KEYBOARD_HID_USES_REPORT_IDS || dev != keyboard_hid_dev) {
                return -EINVAL;
            }
            return process_led_report(*len, sizeof(struct zmk_hid_led_report_body),
                                      (struct zmk_hid_led_report_body *)*data);
        }
        case ZMK_HID_REPORT_ID_LEDS: {
            if (!ZMK_USB_KEYBOARD_HID_USES_REPORT_IDS || dev != keyboard_hid_dev) {
                return -EINVAL;
            }
            struct zmk_hid_led_report *report = (struct zmk_hid_led_report *)*data;
            return process_led_report(*len, sizeof(*report), &report->body);
        }
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

#define ZMK_USB_HID_DESC __attribute__((unused)) static const uint8_t

/*
 * Keyboard-only HID interfaces intentionally omit the report ID for a simpler
 * boot-keyboard-like wire format. Consumer and pointing reports currently keep
 * report IDs even when split onto their own interfaces, but this isn't strictly
 * necessary so may be changed in the future.
 */
#if !ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED &&                                                       \
    (!IS_ENABLED(CONFIG_ZMK_POINTING) || !ZMK_USB_KEYBOARD_POINTING_HID_SHARED)
ZMK_USB_HID_DESC zmk_hid_keyboard_report_desc[] = {
    ZMK_HID_KEYBOARD_DESC(ZMK_HID_REPORT_ID_NONE),
};
#endif

#if !ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED &&                                                       \
    (!IS_ENABLED(CONFIG_ZMK_POINTING) || !ZMK_USB_CONSUMER_POINTING_HID_SHARED)
ZMK_USB_HID_DESC zmk_hid_consumer_report_desc[] = {
    ZMK_HID_CONSUMER_DESC(ZMK_HID_CONSUMER_REPORT_ID_DESC),
};
#endif

#if ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED &&                                                        \
    (!IS_ENABLED(CONFIG_ZMK_POINTING) || !ZMK_USB_KEYBOARD_POINTING_HID_SHARED)
ZMK_USB_HID_DESC zmk_hid_keyboard_consumer_report_desc[] = {
    ZMK_HID_KEYBOARD_DESC(ZMK_HID_KEYBOARD_REPORT_ID_DESC),
    ZMK_HID_CONSUMER_DESC(ZMK_HID_CONSUMER_REPORT_ID_DESC),
};
#endif

#if IS_ENABLED(CONFIG_ZMK_POINTING)
#if !ZMK_USB_KEYBOARD_POINTING_HID_SHARED && !ZMK_USB_CONSUMER_POINTING_HID_SHARED
ZMK_USB_HID_DESC zmk_hid_mouse_report_desc[] = {
    ZMK_HID_MOUSE_DESC(ZMK_HID_MOUSE_REPORT_ID_DESC),
};
#endif

#if ZMK_USB_KEYBOARD_POINTING_HID_SHARED && !ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED
ZMK_USB_HID_DESC zmk_hid_keyboard_mouse_report_desc[] = {
    ZMK_HID_KEYBOARD_DESC(ZMK_HID_KEYBOARD_REPORT_ID_DESC),
    ZMK_HID_MOUSE_DESC(ZMK_HID_MOUSE_REPORT_ID_DESC),
};
#endif

#if ZMK_USB_CONSUMER_POINTING_HID_SHARED && !ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED
ZMK_USB_HID_DESC zmk_hid_consumer_mouse_report_desc[] = {
    ZMK_HID_CONSUMER_DESC(ZMK_HID_CONSUMER_REPORT_ID_DESC),
    ZMK_HID_MOUSE_DESC(ZMK_HID_MOUSE_REPORT_ID_DESC),
};
#endif
#endif // IS_ENABLED(CONFIG_ZMK_POINTING)

#define REGISTER_HID_DEVICE(dev, desc) usb_hid_register_device(dev, desc, sizeof(desc), &ops)

static int zmk_usb_hid_send_report(const struct device *dev, struct k_sem *sem,
                                   const uint8_t *report, size_t len) {
    switch (zmk_usb_get_status()) {
    case USB_DC_SUSPEND:
        return usb_wakeup_request();
    case USB_DC_ERROR:
    case USB_DC_RESET:
    case USB_DC_DISCONNECTED:
    case USB_DC_UNKNOWN:
        return -ENODEV;
    default:
        k_sem_take(sem, K_MSEC(30));
        int err = hid_int_ep_write(dev, report, len, NULL);

        if (err) {
            k_sem_give(sem);
        }

        return err;
    }
}

int zmk_usb_hid_send_keyboard_report(void) {
    size_t len;
    uint8_t *report = get_keyboard_report(&len);
    return zmk_usb_hid_send_report(keyboard_hid_dev, keyboard_hid_sem_ptr, report, len);
}

int zmk_usb_hid_send_consumer_report(void) {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    if (ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED && hid_protocol == HID_PROTOCOL_BOOT) {
        return -ENOTSUP;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

    struct zmk_hid_consumer_report *report = zmk_hid_get_consumer_report();
    return zmk_usb_hid_send_report(consumer_hid_dev, consumer_hid_sem_ptr, (uint8_t *)report,
                                   sizeof(*report));
}

#if IS_ENABLED(CONFIG_ZMK_POINTING)
int zmk_usb_hid_send_mouse_report() {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    if (ZMK_USB_KEYBOARD_POINTING_HID_SHARED && hid_protocol == HID_PROTOCOL_BOOT) {
        return -ENOTSUP;
    }
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

    struct zmk_hid_mouse_report *report = zmk_hid_get_mouse_report();
    return zmk_usb_hid_send_report(mouse_hid_dev, mouse_hid_sem_ptr, (uint8_t *)report,
                                   sizeof(*report));
}
#endif // IS_ENABLED(CONFIG_ZMK_POINTING)

static void assign_hid_semaphores(void) {
    keyboard_hid_sem_ptr = &keyboard_hid_sem;
    consumer_hid_sem_ptr =
        ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED ? keyboard_hid_sem_ptr : &consumer_hid_sem;
#if IS_ENABLED(CONFIG_ZMK_POINTING)
    if (ZMK_USB_KEYBOARD_POINTING_HID_SHARED) {
        mouse_hid_sem_ptr = keyboard_hid_sem_ptr;
    } else if (ZMK_USB_CONSUMER_POINTING_HID_SHARED) {
        mouse_hid_sem_ptr = consumer_hid_sem_ptr;
    } else {
        mouse_hid_sem_ptr = &mouse_hid_sem;
    }
#endif
}

static int zmk_usb_hid_init(void) {
    keyboard_hid_dev = device_get_binding(ZMK_USB_KEYBOARD_HID_DEVICE);
    consumer_hid_dev = device_get_binding(ZMK_USB_CONSUMER_HID_DEVICE);
#if IS_ENABLED(CONFIG_ZMK_POINTING)
    mouse_hid_dev = device_get_binding(ZMK_USB_POINTING_HID_DEVICE);
#endif
    if (keyboard_hid_dev == NULL) {
        LOG_ERR("Unable to locate keyboard HID device %s", ZMK_USB_KEYBOARD_HID_DEVICE);
        return -EINVAL;
    }

    if (consumer_hid_dev == NULL) {
        LOG_ERR("Unable to locate consumer HID device %s", ZMK_USB_CONSUMER_HID_DEVICE);
        return -EINVAL;
    }

#if IS_ENABLED(CONFIG_ZMK_POINTING)
    if (mouse_hid_dev == NULL) {
        LOG_ERR("Unable to locate pointing HID device %s", ZMK_USB_POINTING_HID_DEVICE);
        return -EINVAL;
    }
#endif

    assign_hid_semaphores();

#if ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED &&                                                        \
    (!IS_ENABLED(CONFIG_ZMK_POINTING) || ZMK_USB_KEYBOARD_POINTING_HID_SHARED)
    REGISTER_HID_DEVICE(keyboard_hid_dev, zmk_hid_report_desc);
#else
#if IS_ENABLED(CONFIG_ZMK_POINTING) && ZMK_USB_KEYBOARD_POINTING_HID_SHARED
    REGISTER_HID_DEVICE(keyboard_hid_dev, zmk_hid_keyboard_mouse_report_desc);
#elif ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED
    REGISTER_HID_DEVICE(keyboard_hid_dev, zmk_hid_keyboard_consumer_report_desc);
#else
    REGISTER_HID_DEVICE(keyboard_hid_dev, zmk_hid_keyboard_report_desc);
#endif
#if !ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED
#if IS_ENABLED(CONFIG_ZMK_POINTING)
    if (ZMK_USB_CONSUMER_POINTING_HID_SHARED) {
        REGISTER_HID_DEVICE(consumer_hid_dev, zmk_hid_consumer_mouse_report_desc);
    } else {
        REGISTER_HID_DEVICE(consumer_hid_dev, zmk_hid_consumer_report_desc);
    }
#else
    REGISTER_HID_DEVICE(consumer_hid_dev, zmk_hid_consumer_report_desc);
#endif
#endif
#if IS_ENABLED(CONFIG_ZMK_POINTING)
    if (!ZMK_USB_KEYBOARD_POINTING_HID_SHARED && !ZMK_USB_CONSUMER_POINTING_HID_SHARED) {
        REGISTER_HID_DEVICE(mouse_hid_dev, zmk_hid_mouse_report_desc);
    }
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    usb_hid_set_proto_code(keyboard_hid_dev, HID_BOOT_IFACE_CODE_KEYBOARD);
#endif /* IS_ENABLED(CONFIG_ZMK_USB_BOOT) */

    usb_hid_init(keyboard_hid_dev);
    if (!ZMK_USB_KEYBOARD_CONSUMER_HID_SHARED) {
        usb_hid_init(consumer_hid_dev);
    }
#if IS_ENABLED(CONFIG_ZMK_POINTING)
    if (!ZMK_USB_KEYBOARD_POINTING_HID_SHARED && !ZMK_USB_CONSUMER_POINTING_HID_SHARED) {
        usb_hid_init(mouse_hid_dev);
    }
#endif

    return 0;
}

SYS_INIT(zmk_usb_hid_init, APPLICATION, CONFIG_ZMK_USB_HID_INIT_PRIORITY);
