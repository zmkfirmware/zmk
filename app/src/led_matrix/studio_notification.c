// SPDX-License-Identifier: MIT

#include <zmk/events/led_matrix_state_changed.h>
#include <zmk/studio/rpc.h>

static int lighting_event_mapper(const zmk_event_t *eh, zmk_studio_Notification *n) {
    const struct zmk_led_matrix_state_changed *event = as_zmk_led_matrix_state_changed(eh);
    const uint8_t settings =
        ZMK_LED_MATRIX_STATE_CHANGE_ENABLED | ZMK_LED_MATRIX_STATE_CHANGE_BRIGHTNESS |
        ZMK_LED_MATRIX_STATE_CHANGE_PROFILE | ZMK_LED_MATRIX_STATE_CHANGE_SPEED |
        ZMK_LED_MATRIX_STATE_CHANGE_COLOR | ZMK_LED_MATRIX_STATE_CHANGE_CHILD_PROFILE;
    if (!event || !(event->reason_mask & settings) ||
        zmk_studio_core_get_lock_state() != ZMK_STUDIO_CORE_LOCK_STATE_UNLOCKED) {
        return -ENOTSUP;
    }
    *n = ZMK_RPC_NOTIFICATION(lighting, state_changed, true);
    return 0;
}
ZMK_RPC_EVENT_MAPPER(lighting, lighting_event_mapper, zmk_led_matrix_state_changed);
