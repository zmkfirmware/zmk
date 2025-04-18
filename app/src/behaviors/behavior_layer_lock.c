#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/behavior.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static bool layer_locked = false;
static uint8_t locked_layer = 0xFF;

static int layer_lock_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    uint8_t layer_index = binding->param1;

    if (layer_locked && locked_layer == layer_index) {
        layer_locked = false;
        zmk_keymap_layer_deactivate(layer_index);
        locked_layer = 0xFF;
        LOG_INF("LayerLock: Unlocked layer %d", layer_index);
    } else {
        if (layer_locked) {
            zmk_keymap_layer_deactivate(locked_layer);
        }

        layer_locked = true;
        locked_layer = layer_index;
        zmk_keymap_layer_activate(layer_index);
        LOG_INF("LayerLock: Locked layer %d", layer_index);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int layer_lock_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int layer_lock_listener(const zmk_event_t *eh) {
    if (!layer_locked) {
        return ZMK_EV_EVENT_BUBBLE;
    }

#if CONFIG_ZMK_BEHAVIOR_LAYER_LOCK_UNLOCK_ON_ESC
    const struct zmk_position_state_changed *pos_ev = as_zmk_position_state_changed(eh);
    if (pos_ev && pos_ev->state) {
        struct zmk_behavior_binding pressed_binding;
        int err = zmk_keymap_binding_for_key(pos_ev->position, 0, &pressed_binding);
        if (err) {
            return ZMK_EV_EVENT_BUBBLE;
        }

        if (strcmp(binding.behavior_dev, "ZMK_BEHAVIOR_MOMENTARY_LAYER") == 0) {
            zmk_keymap_layer_deactivate(locked_layer);
            layer_locked = false;
            locked_layer = 0xFF;
        }
    }
#endif

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_lock, layer_lock_listener);
ZMK_SUBSCRIPTION(layer_lock, zmk_position_state_changed);

static const struct behavior_driver_api layer_lock_driver_api = {
    .binding_pressed = layer_lock_pressed,
    .binding_released = layer_lock_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &layer_lock_driver_api);

#endif
