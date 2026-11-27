/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT zmk_behavior_numpad_calculator
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include "include/dt-bindings/numpad-calculator.h"
#if IS_ENABLED(CONFIG_NUMPAD_CALCULATOR)
#include "calculator.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static int pressed(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
#if IS_ENABLED(CONFIG_NUMPAD_CALCULATOR)
    if (binding->param1 == CALC_TOGGLE) {
        const uint8_t layer = DT_PROP(DT_DRV_INST(0), calculator_layer);
        const uint8_t fn = DT_PROP(DT_DRV_INST(0), function_layer);
        if (zmk_keymap_layer_active(layer)) {
            zmk_keymap_layer_deactivate(fn, false);
            return zmk_keymap_layer_deactivate(layer, false);
        }
        return zmk_keymap_layer_activate(layer, false);
    }
    numpad_calculator_key(binding->param1);
#endif
    return ZMK_BEHAVIOR_OPAQUE;
}
static int released(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}
static const struct behavior_driver_api api = {.binding_pressed = pressed,
                                               .binding_released = released};
BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &api);

#endif
