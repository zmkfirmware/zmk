
#pragma once

#include <zephyr/bluetooth/addr.h>
#include <zmk/behavior.h>
#include <zmk/hid_indicators_types.h>

int zmk_split_bt_invoke_behavior(uint8_t source, struct zmk_behavior_binding *binding,
                                 struct zmk_behavior_binding_event event, bool state);

int zmk_split_bt_update_hid_indicator(zmk_hid_indicators indicators);
