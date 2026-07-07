/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/ble.h>
#include <zmk/endpoints_types.h>

/**
 * Recommended length of string buffer for printing endpoint identifiers.
 */
#define ZMK_ENDPOINT_STR_LEN 10

#define ZMK_ENDPOINT_NONE_COUNT 1

#ifdef CONFIG_ZMK_USB
#define ZMK_ENDPOINT_USB_COUNT 1
#else
#define ZMK_ENDPOINT_USB_COUNT 0
#endif

#ifdef CONFIG_ZMK_BLE
#define ZMK_ENDPOINT_BLE_COUNT ZMK_BLE_PROFILE_COUNT
#else
#define ZMK_ENDPOINT_BLE_COUNT 0
#endif

/**
 * The total number of different (struct zmk_endpoint_instance) values that can
 * be selected.
 *
 * Note that this value may change between firmware versions, so it should not
 * be used in any persistent storage.
 */
#define ZMK_ENDPOINT_COUNT                                                                         \
    (ZMK_ENDPOINT_NONE_COUNT + ZMK_ENDPOINT_USB_COUNT + ZMK_ENDPOINT_BLE_COUNT)

bool zmk_endpoint_instance_eq(struct zmk_endpoint_instance a, struct zmk_endpoint_instance b);

/**
 * Writes a string identifying an endpoint instance.
 *
 * @param str Address of output string buffer
 * @param len Length of string buffer. See ZMK_ENDPOINT_STR_LEN for recommended length.
 *
 * @returns Number of characters written.
 */
int zmk_endpoint_instance_to_str(struct zmk_endpoint_instance endpoint, char *str, size_t len);

/**
 * Gets a unique index for an endpoint instance. This can be used together with
 * ZMK_ENDPOINT_COUNT to manage separate state for each endpoint instance.
 *
 * Note that the index for a specific instance may change between firmware versions,
 * so it should not be used in any persistent storage.
 */
int zmk_endpoint_instance_to_index(struct zmk_endpoint_instance endpoint);

/**
 * Sets the preferred endpoint transport to use.
 *
 * If the preferred endpoint is not available, zmk_endpoint_get_selected() may
 * automatically fall back to another transport.
 */
int zmk_endpoint_set_preferred_transport(enum zmk_transport transport);

enum zmk_transport zmk_endpoint_get_preferred_transport(void);

/**
 * If the preferred endpoint transport is USB, sets it to BLE, else sets it to USB.
 */
int zmk_endpoint_toggle_preferred_transport(void);

/**
 * Gets the endpoint instance that will be preferred if it is connected.
 */
struct zmk_endpoint_instance zmk_endpoint_get_preferred(void);

/**
 * Gets the endpoint instance that is currently in use.
 *
 * This may differ from zmk_endpoint_get_preferred(), for example if the preferred
 * endpoint is not connected, then this will return an instance for ZMK_TRANSPORT_NONE.
 */
struct zmk_endpoint_instance zmk_endpoint_get_selected(void);

/**
 * Returns whether the keyboard is connected to an endpoint.
 *
 * This is equivalent to zmk_endpoint_get_selected().transport != ZMK_TRANSPORT_NONE
 */
bool zmk_endpoint_is_connected(void);

/**
 * Sends the HID report for the given usage page to the selected endpoint.
 */
int zmk_endpoint_send_report(uint16_t usage_page);

#if IS_ENABLED(CONFIG_ZMK_POINTING)
/**
 * Sends the HID mouse report to the selected endpoint.
 */
int zmk_endpoint_send_mouse_report();
#endif // IS_ENABLED(CONFIG_ZMK_POINTING)

/**
 * Clears all HID reports for the selected endpoint.
 */
void zmk_endpoint_clear_reports(void);
