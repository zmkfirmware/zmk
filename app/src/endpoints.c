/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/usb.h>
#include <zmk/hog.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

int zmk_endpoints_send_report(u8_t usage_page) {
    int err;
    struct zmk_hid_keypad_report *keypad_report;
    struct zmk_hid_consumer_report *consumer_report;
    LOG_DBG("usage page 0x%02X", usage_page);
    switch (usage_page) {
    case USAGE_KEYPAD:
        keypad_report = zmk_hid_get_keypad_report();
#ifdef CONFIG_ZMK_USB
        if (zmk_usb_hid_send_report((u8_t *)keypad_report, sizeof(struct zmk_hid_keypad_report)) !=
            0) {
            LOG_DBG("USB Send Failed");
        }
#endif /* CONFIG_ZMK_USB */

#ifdef CONFIG_ZMK_BLE
        err = zmk_hog_send_keypad_report(&keypad_report->body);
        if (err) {
            LOG_ERR("FAILED TO SEND OVER HOG: %d", err);
        }
#endif /* CONFIG_ZMK_BLE */

        break;
    case USAGE_CONSUMER:
        consumer_report = zmk_hid_get_consumer_report();
#ifdef CONFIG_ZMK_USB
        if (zmk_usb_hid_send_report((u8_t *)consumer_report,
                                    sizeof(struct zmk_hid_consumer_report)) != 0) {
            LOG_DBG("USB Send Failed");
        }
#endif /* CONFIG_ZMK_USB */

#ifdef CONFIG_ZMK_BLE
        err = zmk_hog_send_consumer_report(&consumer_report->body);
        if (err) {
            LOG_ERR("FAILED TO SEND OVER HOG: %d", err);
        }
#endif /* CONFIG_ZMK_BLE */

        break;
    default:
        LOG_ERR("Unsupported usage page %d", usage_page);
        return -ENOTSUP;
    }

    return 0;
}
