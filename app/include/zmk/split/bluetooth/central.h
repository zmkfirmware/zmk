
#pragma once

#include <bluetooth/addr.h>
#include <zmk/behavior.h>

int zmk_split_bt_invoke_behavior(const bt_addr_le_t *source, struct zmk_behavior_binding *binding,
                                 struct zmk_behavior_binding_event event, bool state);