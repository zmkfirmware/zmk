/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/hid_indicators_types.h>

zmk_hid_indicators_t zmk_hid_indicators_get_current_profile(void);

#if ZMK_ENDPOINTS_LOCAL

zmk_hid_indicators_t zmk_hid_indicators_get_profile(struct zmk_endpoint_instance endpoint);
void zmk_hid_indicators_set_profile(zmk_hid_indicators_t indicators,
                                    struct zmk_endpoint_instance endpoint);

void zmk_hid_indicators_process_report(struct zmk_hid_led_report_body *report,
                                       struct zmk_endpoint_instance endpoint);

#elif IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

/**
 * Replaces the indicator state mirrored from the central.
 *
 * Only exists on a split peripheral, where it is called by the split transport.
 * The per endpoint functions above are absent there, since a peripheral has no
 * endpoints to key the state on. Raises zmk_hid_indicators_changed if the state
 * changed.
 */
int zmk_hid_indicators_set_mirrored_state(zmk_hid_indicators_t indicators);

#endif // ZMK_ENDPOINTS_LOCAL
