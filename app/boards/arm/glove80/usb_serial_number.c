/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/hwinfo.h>
#include "usb_descriptor.h"

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(usb_descriptor);

int base16_encode(const uint8_t *data, int length, uint8_t *result, int bufSize);

uint8_t *usb_update_sn_string_descriptor(void) {
    /*
     * nrf52840 hwinfo returns a 64-bit hardware id. Glove80 uses this as a
     * serial number, encoded as base16 into the last 16 characters of the
     * CONFIG_USB_DEVICE_SN template. If insufficient template space is
     * available, instead return the static serial number string.
     */
    const uint8_t template_len = sizeof(CONFIG_USB_DEVICE_SN);
    const uint8_t sn_len = 16;

    if (template_len < sn_len + 1) {
        LOG_DBG("Serial number template too short");
        return CONFIG_USB_DEVICE_SN;
    }

    static uint8_t serial[sizeof(CONFIG_USB_DEVICE_SN)];
    strncpy(serial, CONFIG_USB_DEVICE_SN, template_len);

    uint8_t hwid[8];
    memset(hwid, 0, sizeof(hwid));
    uint8_t hwlen = hwinfo_get_device_id(hwid, sizeof(hwid));

    if (hwlen > 0) {
        const uint8_t offset = template_len - sn_len - 1;
        LOG_HEXDUMP_DBG(&hwid, sn_len, "Serial Number");
        base16_encode(hwid, hwlen, serial + offset, sn_len + 1);
    }

    return serial;
}

int base16_encode(const uint8_t *data, int length, uint8_t *result, int bufSize) {
    const char hex[] = "0123456789ABCDEF";

    int i = 0;
    while (i < bufSize && i < length * 2) {
        uint8_t nibble;
        if (i % 2 == 0) {
            nibble = data[i / 2] >> 4;
        } else {
            nibble = data[i / 2] & 0xF;
        }
        result[i] = hex[nibble];
        ++i;
    }
    if (i < bufSize) {
        result[i] = '\0';
    }
    return i;
}
