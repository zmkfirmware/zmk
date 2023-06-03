/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_hold_tap

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/keys.h>
#include <dt-bindings/zmk/keys.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/matrix.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ZMK_BHV_HOLD_TAP_MAX_HELD 10
#define ZMK_BHV_HOLD_TAP_MAX_CAPTURED_EVENTS 40

// increase if you have keyboard with more keys.
#define ZMK_BHV_HOLD_TAP_POSITION_NOT_USED 9999

enum flavor {
    FLAVOR_HOLD_PREFERRED,
    FLAVOR_BALANCED,
    FLAVOR_TAP_PREFERRED,
    FLAVOR_TAP_UNLESS_INTERRUPTED,
};

enum status {
    STATUS_UNDECIDED,
    STATUS_TAP,
    STATUS_HOLD_INTERRUPT,
    STATUS_HOLD_TIMER,
};

enum decision_moment {
    HT_KEY_UP,
    HT_OTHER_KEY_DOWN,
    HT_OTHER_KEY_UP,
    HT_TIMER_EVENT,
    HT_QUICK_TAP,
};

struct behavior_hold_tap_config {
    int tapping_term_ms;
    char *hold_behavior_dev;
    char *tap_behavior_dev;
    int quick_tap_ms;
    bool global_quick_tap;
    enum flavor flavor;
    bool hold_while_undecided;
    bool retro_tap;
    bool hold_trigger_on_release;
    int32_t hold_trigger_key_positions_len;
    int32_t hold_trigger_key_positions[];
};

// this data is specific for each hold-tap
struct active_hold_tap {
    bool first_press;
    int32_t position;
    uint32_t param_hold;
    uint32_t param_tap;
    int64_t timestamp;
    bool hold_released;
    enum status status;
    const struct behavior_hold_tap_config *config;
    struct k_work_delayable work;
    bool work_is_cancelled;

    // initialized to -1, which is to be interpreted as "no other key has been pressed yet"
    int32_t position_of_first_other_key_pressed;
};

// The undecided hold tap is the hold tap that needs to be decided before
// other keypress events can be released. While the undecided_hold_tap is
// not NULL, most events are captured in captured_events.
// After the hold_tap is decided, it will stay in the active_hold_taps until
// its key-up has been processed and the delayed work is cleaned up.
struct active_hold_tap *undecided_hold_tap = NULL;
struct active_hold_tap active_hold_taps[ZMK_BHV_HOLD_TAP_MAX_HELD] = {};
// We capture most position_state_changed events and some modifiers_state_changed events.
const zmk_event_t *captured_events[ZMK_BHV_HOLD_TAP_MAX_CAPTURED_EVENTS] = {};

// Keep track of which key was tapped most recently for the standard, if it is a hold-tap
// a position, will be given, if not it will just be INT32_MIN
struct last_tapped {
    int32_t position;
    int64_t timestamp;
};

struct last_tapped last_tapped = {INT32_MIN, INT64_MIN};

static void store_last_tapped(int64_t timestamp) {
    if (timestamp > last_tapped.timestamp) {
        last_tapped.position = INT32_MIN;
        last_tapped.timestamp = timestamp;
    }
}

static void store_last_hold_tapped(struct active_hold_tap *hold_tap) {
    last_tapped.position = hold_tap->position;
    last_tapped.timestamp = hold_tap->timestamp;
}

static bool is_quick_tap(struct active_hold_tap *hold_tap) {
    if (hold_tap->config->global_quick_tap || last_tapped.position == hold_tap->position) {
        return (last_tapped.timestamp + hold_tap->config->quick_tap_ms) > hold_tap->timestamp;
    } else {
        return false;
    }
}

static int capture_event(const zmk_event_t *event) {
    for (int i = 0; i < ZMK_BHV_HOLD_TAP_MAX_CAPTURED_EVENTS; i++) {
        if (captured_events[i] == NULL) {
            captured_events[i] = event;
            return 0;
        }
    }
    return -ENOMEM;
}

static struct zmk_position_state_changed *find_captured_keydown_event(uint32_t position) {
    struct zmk_position_state_changed *last_match = NULL;
    for (int i = 0; i < ZMK_BHV_HOLD_TAP_MAX_CAPTURED_EVENTS; i++) {
        const zmk_event_t *eh = captured_events[i];
        if (eh == NULL) {
            return last_match;
        }
        struct zmk_position_state_changed *position_event = as_zmk_position_state_changed(eh);
        if (position_event == NULL) {
            continue;
        }

        if (position_event->position == position && position_event->state) {
            last_match = position_event;
        }
    }
    return last_match;
}

const struct zmk_listener zmk_listener_behavior_hold_tap;

static void release_captured_events() {
    if (undecided_hold_tap != NULL) {
        return;
    }

    // We use a trick to prevent copying the captured_events array.
    //
    // Events for different mod-tap instances are separated by a NULL pointer.
    //
    // The first event popped will never be catched by the next active hold-tap
    // because to start capturing a mod-tap-key-down event must first completely
    // go through the events queue.
    //
    // Example of this release process;
    // [mt2_down, k1_down, k1_up, mt2_up, null, ...]
    //  ^
    // mt2_down position event isn't captured because no hold-tap is active.
    // mt2_down behavior event is handled, now we have an undecided hold-tap
    // [null, k1_down, k1_up, mt2_up, null, ...]
    //        ^
    // k1_down  is captured by the mt2 mod-tap
    // !note that searches for find_captured_keydown_event by the mt2 behavior will stop at the
    // first null encountered [mt1_down, null, k1_up, mt2_up, null, ...]
    //                  ^
    // k1_up event is captured by the new hold-tap:
    // [k1_down, k1_up, null, mt2_up, null, ...]
    //                        ^
    // mt2_up event is not captured but causes release of mt2 behavior
    // [k1_down, k1_up, null, null, null, ...]
    // now mt2 will start releasing it's own captured positions.
    for (int i = 0; i < ZMK_BHV_HOLD_TAP_MAX_CAPTURED_EVENTS; i++) {
        const zmk_event_t *captured_event = captured_events[i];
        if (captured_event == NULL) {
            return;
        }
        captured_events[i] = NULL;
        if (undecided_hold_tap != NULL) {
            k_msleep(10);
        }

        struct zmk_position_state_changed *position_event;
        struct zmk_keycode_state_changed *modifier_event;
        if ((position_event = as_zmk_position_state_changed(captured_event)) != NULL) {
            LOG_DBG("Releasing key position event for position %d %s", position_event->position,
                    (position_event->state ? "pressed" : "released"));
        } else if ((modifier_event = as_zmk_keycode_state_changed(captured_event)) != NULL) {
            LOG_DBG("Releasing mods changed event 0x%02X %s", modifier_event->keycode,
                    (modifier_event->state ? "pressed" : "released"));
        }
        ZMK_EVENT_RAISE_AT(captured_event, behavior_hold_tap);
    }
}

static struct active_hold_tap *find_hold_tap(uint32_t position) {
    for (int i = 0; i < ZMK_BHV_HOLD_TAP_MAX_HELD; i++) {
        if (active_hold_taps[i].position == position) {
            return &active_hold_taps[i];
        }
    }
    return NULL;
}

static struct active_hold_tap *store_hold_tap(uint32_t position, uint32_t param_hold,
                                              uint32_t param_tap, int64_t timestamp,
                                              const struct behavior_hold_tap_config *config) {
    for (int i = 0; i < ZMK_BHV_HOLD_TAP_MAX_HELD; i++) {
        if (active_hold_taps[i].position != ZMK_BHV_HOLD_TAP_POSITION_NOT_USED) {
            continue;
        }
        active_hold_taps[i].first_press = true;
        active_hold_taps[i].position = position;
        active_hold_taps[i].status = STATUS_UNDECIDED;
        active_hold_taps[i].config = config;
        active_hold_taps[i].param_hold = param_hold;
        active_hold_taps[i].param_tap = param_tap;
        active_hold_taps[i].hold_released = false;
        active_hold_taps[i].timestamp = timestamp;
        active_hold_taps[i].position_of_first_other_key_pressed = -1;
        return &active_hold_taps[i];
    }
    return NULL;
}

static void clear_hold_tap(struct active_hold_tap *hold_tap) {
    hold_tap->position = ZMK_BHV_HOLD_TAP_POSITION_NOT_USED;
    hold_tap->status = STATUS_UNDECIDED;
    hold_tap->work_is_cancelled = false;
}

static void decide_balanced(struct active_hold_tap *hold_tap, enum decision_moment event) {
    switch (event) {
    case HT_KEY_UP:
        hold_tap->status = STATUS_TAP;
        return;
    case HT_OTHER_KEY_UP:
        hold_tap->status = STATUS_HOLD_INTERRUPT;
        return;
    case HT_TIMER_EVENT:
        hold_tap->status = STATUS_HOLD_TIMER;
        return;
    case HT_QUICK_TAP:
        hold_tap->status = STATUS_TAP;
        return;
    default:
        return;
    }
}

static void decide_tap_preferred(struct active_hold_tap *hold_tap, enum decision_moment event) {
    switch (event) {
    case HT_KEY_UP:
        hold_tap->status = STATUS_TAP;
        return;
    case HT_TIMER_EVENT:
        hold_tap->status = STATUS_HOLD_TIMER;
        return;
    case HT_QUICK_TAP:
        hold_tap->status = STATUS_TAP;
        return;
    default:
        return;
    }
}

static void decide_tap_unless_interrupted(struct active_hold_tap *hold_tap,
                                          enum decision_moment event) {
    switch (event) {
    case HT_KEY_UP:
        hold_tap->status = STATUS_TAP;
        return;
    case HT_OTHER_KEY_DOWN:
        hold_tap->status = STATUS_HOLD_INTERRUPT;
        return;
    case HT_TIMER_EVENT:
        hold_tap->status = STATUS_TAP;
        return;
    case HT_QUICK_TAP:
        hold_tap->status = STATUS_TAP;
        return;
    default:
        return;
    }
}

static void decide_hold_preferred(struct active_hold_tap *hold_tap, enum decision_moment event) {
    switch (event) {
    case HT_KEY_UP:
        hold_tap->status = STATUS_TAP;
        return;
    case HT_OTHER_KEY_DOWN:
        hold_tap->status = STATUS_HOLD_INTERRUPT;
        return;
    case HT_TIMER_EVENT:
        hold_tap->status = STATUS_HOLD_TIMER;
        return;
    case HT_QUICK_TAP:
        hold_tap->status = STATUS_TAP;
        return;
    default:
        return;
    }
}

static inline const char *flavor_str(enum flavor flavor) {
    switch (flavor) {
    case FLAVOR_HOLD_PREFERRED:
        return "hold-preferred";
    case FLAVOR_BALANCED:
        return "balanced";
    case FLAVOR_TAP_PREFERRED:
        return "tap-preferred";
    case FLAVOR_TAP_UNLESS_INTERRUPTED:
        return "tap-unless-interrupted";
    default:
        return "UNKNOWN FLAVOR";
    }
}

static inline const char *status_str(enum status status) {
    switch (status) {
    case STATUS_UNDECIDED:
        return "undecided";
    case STATUS_HOLD_TIMER:
        return "hold-timer";
    case STATUS_HOLD_INTERRUPT:
        return "hold-interrupt";
    case STATUS_TAP:
        return "tap";
    default:
        return "UNKNOWN STATUS";
    }
}

static inline const char *decision_moment_str(enum decision_moment decision_moment) {
    switch (decision_moment) {
    case HT_KEY_UP:
        return "key-up";
    case HT_OTHER_KEY_DOWN:
        return "other-key-down";
    case HT_OTHER_KEY_UP:
        return "other-key-up";
    case HT_QUICK_TAP:
        return "quick-tap";
    case HT_TIMER_EVENT:
        return "timer";
    default:
        return "UNKNOWN STATUS";
    }
}

static int press_binding(struct active_hold_tap *hold_tap) {
    if (hold_tap->config->retro_tap && hold_tap->status == STATUS_HOLD_TIMER) {
        return 0;
    }

    struct zmk_behavior_binding_event event = {
        .position = hold_tap->position,
        .timestamp = hold_tap->timestamp,
    };

    struct zmk_behavior_binding binding = {0};
    if (hold_tap->status == STATUS_HOLD_TIMER || hold_tap->status == STATUS_HOLD_INTERRUPT) {
        binding.behavior_dev = hold_tap->config->hold_behavior_dev;
        binding.param1 = hold_tap->param_hold;
    } else {
        binding.behavior_dev = hold_tap->config->tap_behavior_dev;
        binding.param1 = hold_tap->param_tap;
        store_last_hold_tapped(hold_tap);
    }
    return behavior_keymap_binding_pressed(&binding, event);
}

static int release_binding(struct active_hold_tap *hold_tap) {
    if (hold_tap->config->retro_tap && hold_tap->status == STATUS_HOLD_TIMER) {
        return 0;
    }

    struct zmk_behavior_binding_event event = {
        .position = hold_tap->position,
        .timestamp = hold_tap->timestamp,
    };

    struct zmk_behavior_binding binding = {0};
    if (hold_tap->status == STATUS_HOLD_TIMER || hold_tap->status == STATUS_HOLD_INTERRUPT) {
        binding.behavior_dev = hold_tap->config->hold_behavior_dev;
        binding.param1 = hold_tap->param_hold;
    } else {
        binding.behavior_dev = hold_tap->config->tap_behavior_dev;
        binding.param1 = hold_tap->param_tap;
    }
    return behavior_keymap_binding_released(&binding, event);
}

static bool is_first_other_key_pressed_trigger_key(struct active_hold_tap *hold_tap) {
    for (int i = 0; i < hold_tap->config->hold_trigger_key_positions_len; i++) {
        if (hold_tap->config->hold_trigger_key_positions[i] ==
            hold_tap->position_of_first_other_key_pressed) {
            return true;
        }
    }
    return false;
}

// Force a tap decision if the positional conditions for a hold decision are not met.
static void decide_positional_hold(struct active_hold_tap *hold_tap) {
    // Only force a tap decision if the positional hold/tap feature is enabled.
    if (!(hold_tap->config->hold_trigger_key_positions_len > 0)) {
        return;
    }

    // Only force a tap decision if another key was pressed after
    // the hold/tap key.
    if (hold_tap->position_of_first_other_key_pressed == -1) {
        return;
    }

    // Only force a tap decision if the first other key to be pressed
    // (after the hold/tap key) is not one of the trigger keys.
    if (is_first_other_key_pressed_trigger_key(hold_tap)) {
        return;
    }

    // Since the positional key conditions have failed, force a TAP decision.
    hold_tap->status = STATUS_TAP;
}

static void release_hold_binding(struct active_hold_tap *hold_tap,
                                 enum decision_moment decision_moment) {
    bool keyTap =
        !(hold_tap->status == STATUS_HOLD_TIMER || hold_tap->status == STATUS_HOLD_INTERRUPT);
    bool holdRelease =
        (hold_tap->status == STATUS_HOLD_TIMER || hold_tap->status == STATUS_HOLD_INTERRUPT) &&
        decision_moment == HT_KEY_UP;

    if (holdRelease || keyTap) {
        LOG_DBG("Releasing hold behavior");
        struct zmk_behavior_binding_event event = {
            .position = hold_tap->position,
            .timestamp = hold_tap->timestamp,
        };
        struct zmk_behavior_binding binding = {0};
        binding.behavior_dev = hold_tap->config->hold_behavior_dev;
        binding.param1 = hold_tap->param_hold;
        behavior_keymap_binding_released(&binding, event);
        hold_tap->hold_released = true;
    }
}

static void decide_hold_tap(struct active_hold_tap *hold_tap,
                            enum decision_moment decision_moment) {
    if (hold_tap->status != STATUS_UNDECIDED) {
        return;
    }

    if (hold_tap != undecided_hold_tap) {
        LOG_DBG("ERROR found undecided tap hold that is not the active tap hold");
        return;
    }

    // If the hold-tap behavior is still undecided, attempt to decide it.
    switch (hold_tap->config->flavor) {
    case FLAVOR_HOLD_PREFERRED:
        decide_hold_preferred(hold_tap, decision_moment);
        break;
    case FLAVOR_BALANCED:
        decide_balanced(hold_tap, decision_moment);
        break;
    case FLAVOR_TAP_PREFERRED:
        decide_tap_preferred(hold_tap, decision_moment);
        break;
    case FLAVOR_TAP_UNLESS_INTERRUPTED:
        decide_tap_unless_interrupted(hold_tap, decision_moment);
        break;
    }

    if (hold_tap->status == STATUS_UNDECIDED) {
        return;
    }

    decide_positional_hold(hold_tap);

    // Since the hold-tap has been decided, clean up undecided_hold_tap and
    // execute the decided behavior.
    LOG_DBG("%d decided %s (%s decision moment %s)", hold_tap->position,
            status_str(hold_tap->status), flavor_str(hold_tap->config->flavor),
            decision_moment_str(decision_moment));
    undecided_hold_tap = NULL;

    if (hold_tap->config->hold_while_undecided && !hold_tap->hold_released) {
        release_hold_binding(hold_tap, decision_moment);
    }
    press_binding(hold_tap);
    release_captured_events();
}

static void decide_retro_tap(struct active_hold_tap *hold_tap) {
    if (!hold_tap->config->retro_tap) {
        return;
    }
    if (hold_tap->status == STATUS_HOLD_TIMER) {
        release_binding(hold_tap);
        LOG_DBG("%d retro tap", hold_tap->position);
        hold_tap->status = STATUS_TAP;
        press_binding(hold_tap);
        return;
    }
}

static void update_hold_status_for_retro_tap(uint32_t ignore_position) {
    for (int i = 0; i < ZMK_BHV_HOLD_TAP_MAX_HELD; i++) {
        struct active_hold_tap *hold_tap = &active_hold_taps[i];
        if (hold_tap->position == ignore_position ||
            hold_tap->position == ZMK_BHV_HOLD_TAP_POSITION_NOT_USED ||
            hold_tap->config->retro_tap == false) {
            continue;
        }
        if (hold_tap->status == STATUS_HOLD_TIMER) {
            LOG_DBG("Update hold tap %d status to hold-interrupt", hold_tap->position);
            hold_tap->status = STATUS_HOLD_INTERRUPT;
            press_binding(hold_tap);
        }
    }
}

static int on_hold_tap_binding_pressed(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_hold_tap_config *cfg = dev->config;

    if (undecided_hold_tap != NULL) {
        LOG_DBG("ERROR another hold-tap behavior is undecided.");
        // if this happens, make sure the behavior events occur AFTER other position events.
        return ZMK_BEHAVIOR_OPAQUE;
    }

    struct active_hold_tap *hold_tap =
        store_hold_tap(event.position, binding->param1, binding->param2, event.timestamp, cfg);
    if (hold_tap == NULL) {
        LOG_ERR("unable to store hold-tap info, did you press more than %d hold-taps?",
                ZMK_BHV_HOLD_TAP_MAX_HELD);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    LOG_DBG("%d new undecided hold_tap", event.position);
    undecided_hold_tap = hold_tap;

    if (is_quick_tap(hold_tap)) {
        decide_hold_tap(hold_tap, HT_QUICK_TAP);
    } else if (cfg->hold_while_undecided) {
        struct zmk_behavior_binding hold_binding = {0};
        hold_binding.behavior_dev = cfg->hold_behavior_dev;
        hold_binding.param1 = binding->param1;
        LOG_DBG("%d hold behavior pressed while undecided", event.position);
        behavior_keymap_binding_pressed(&hold_binding, event);
    }

    // if this behavior was queued we have to adjust the timer to only
    // wait for the remaining time.
    int32_t tapping_term_ms_left = (hold_tap->timestamp + cfg->tapping_term_ms) - k_uptime_get();
    k_work_schedule(&hold_tap->work, K_MSEC(tapping_term_ms_left));

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_hold_tap_binding_released(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    struct active_hold_tap *hold_tap = find_hold_tap(event.position);
    if (hold_tap == NULL) {
        LOG_ERR("ACTIVE_HOLD_TAP_CLEANED_UP_TOO_EARLY");
        return ZMK_BEHAVIOR_OPAQUE;
    }

    // If these events were queued, the timer event may be queued too late or not at all.
    // We insert a timer event before the TH_KEY_UP event to verify.
    int work_cancel_result = k_work_cancel_delayable(&hold_tap->work);
    if (event.timestamp > (hold_tap->timestamp + hold_tap->config->tapping_term_ms)) {
        decide_hold_tap(hold_tap, HT_TIMER_EVENT);
    }

    decide_hold_tap(hold_tap, HT_KEY_UP);
    if (hold_tap->config->hold_while_undecided && !hold_tap->hold_released) {
        release_hold_binding(hold_tap, HT_KEY_UP);
    }
    decide_retro_tap(hold_tap);
    release_binding(hold_tap);

    if (work_cancel_result == -EINPROGRESS) {
        // let the timer handler clean up
        // if we'd clear now, the timer may call back for an uninitialized active_hold_tap.
        LOG_DBG("%d hold-tap timer work in event queue", event.position);
        hold_tap->work_is_cancelled = true;
    } else {
        LOG_DBG("%d cleaning up hold-tap", event.position);
        clear_hold_tap(hold_tap);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_hold_tap_driver_api = {
    .binding_pressed = on_hold_tap_binding_pressed,
    .binding_released = on_hold_tap_binding_released,
};

static int position_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

    update_hold_status_for_retro_tap(ev->position);

    if (undecided_hold_tap == NULL) {
        LOG_DBG("%d bubble (no undecided hold_tap active)", ev->position);
        return ZMK_EV_EVENT_BUBBLE;
    }

    // Store the position of pressed key for positional hold-tap purposes.
    if ((undecided_hold_tap->config->hold_trigger_on_release !=
         ev->state) // key has been pressed and hold_trigger_on_release is not set, or key
                    // has been released and hold_trigger_on_release is set
        && (undecided_hold_tap->position_of_first_other_key_pressed ==
            -1) // no other key has been pressed yet
    ) {
        undecided_hold_tap->position_of_first_other_key_pressed = ev->position;
    }

    if (undecided_hold_tap->position == ev->position) {
        if (ev->state) { // keydown
            LOG_ERR("hold-tap listener should be called before before most other listeners!");
            return ZMK_EV_EVENT_BUBBLE;
        } else { // keyup
            LOG_DBG("%d bubble undecided hold-tap keyrelease event", undecided_hold_tap->position);
            return ZMK_EV_EVENT_BUBBLE;
        }
    }

    // If these events were queued, the timer event may be queued too late or not at all.
    // We make a timer decision before the other key events are handled if the timer would
    // have run out.
    if (ev->timestamp >
        (undecided_hold_tap->timestamp + undecided_hold_tap->config->tapping_term_ms)) {
        decide_hold_tap(undecided_hold_tap, HT_TIMER_EVENT);
    }

    if (undecided_hold_tap == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (!ev->state && find_captured_keydown_event(ev->position) == NULL) {
        // no keydown event has been captured, let it bubble.
        // we'll catch modifiers later in modifier_state_changed_listener
        LOG_DBG("%d bubbling %d %s event", undecided_hold_tap->position, ev->position,
                ev->state ? "down" : "up");
        return ZMK_EV_EVENT_BUBBLE;
    }

    LOG_DBG("%d capturing %d %s event", undecided_hold_tap->position, ev->position,
            ev->state ? "down" : "up");
    capture_event(eh);
    decide_hold_tap(undecided_hold_tap, ev->state ? HT_OTHER_KEY_DOWN : HT_OTHER_KEY_UP);
    return ZMK_EV_EVENT_CAPTURED;
}

static int keycode_state_changed_listener(const zmk_event_t *eh) {
    // we want to catch layer-up events too... how?
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    if (ev->state && !is_mod(ev->usage_page, ev->keycode)) {
        store_last_tapped(ev->timestamp);
    }

    if (undecided_hold_tap == NULL) {
        // LOG_DBG("0x%02X bubble (no undecided hold_tap active)", ev->keycode);
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (!is_mod(ev->usage_page, ev->keycode)) {
        // LOG_DBG("0x%02X bubble (not a mod)", ev->keycode);
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (undecided_hold_tap->first_press && undecided_hold_tap->config->hold_while_undecided) {
        undecided_hold_tap->first_press = false;
        return ZMK_EV_EVENT_BUBBLE;
    }

    // only key-up events will bubble through position_state_changed_listener
    // if a undecided_hold_tap is active.
    LOG_DBG("%d capturing 0x%02X %s event", undecided_hold_tap->position, ev->keycode,
            ev->state ? "down" : "up");
    capture_event(eh);
    return ZMK_EV_EVENT_CAPTURED;
}

int behavior_hold_tap_listener(const zmk_event_t *eh) {
    if (as_zmk_position_state_changed(eh) != NULL) {
        return position_state_changed_listener(eh);
    } else if (as_zmk_keycode_state_changed(eh) != NULL) {
        return keycode_state_changed_listener(eh);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(behavior_hold_tap, behavior_hold_tap_listener);
ZMK_SUBSCRIPTION(behavior_hold_tap, zmk_position_state_changed);
// this should be modifiers_state_changed, but unfrotunately that's not implemented yet.
ZMK_SUBSCRIPTION(behavior_hold_tap, zmk_keycode_state_changed);

void behavior_hold_tap_timer_work_handler(struct k_work *item) {
    struct active_hold_tap *hold_tap = CONTAINER_OF(item, struct active_hold_tap, work);

    if (hold_tap->work_is_cancelled) {
        clear_hold_tap(hold_tap);
    } else {
        decide_hold_tap(hold_tap, HT_TIMER_EVENT);
    }
}

static int behavior_hold_tap_init(const struct device *dev) {
    static bool init_first_run = true;

    if (init_first_run) {
        for (int i = 0; i < ZMK_BHV_HOLD_TAP_MAX_HELD; i++) {
            k_work_init_delayable(&active_hold_taps[i].work, behavior_hold_tap_timer_work_handler);
            active_hold_taps[i].position = ZMK_BHV_HOLD_TAP_POSITION_NOT_USED;
        }
    }
    init_first_run = false;
    return 0;
}

#define KP_INST(n)                                                                                 \
    static struct behavior_hold_tap_config behavior_hold_tap_config_##n = {                        \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                       \
        .hold_behavior_dev = DT_PROP(DT_INST_PHANDLE_BY_IDX(n, bindings, 0), label),               \
        .tap_behavior_dev = DT_PROP(DT_INST_PHANDLE_BY_IDX(n, bindings, 1), label),                \
        .quick_tap_ms = DT_INST_PROP(n, quick_tap_ms),                                             \
        .global_quick_tap = DT_INST_PROP(n, global_quick_tap),                                     \
        .flavor = DT_ENUM_IDX(DT_DRV_INST(n), flavor),                                             \
        .hold_while_undecided = DT_INST_PROP(n, hold_while_undecided),                             \
        .retro_tap = DT_INST_PROP(n, retro_tap),                                                   \
        .hold_trigger_on_release = DT_INST_PROP(n, hold_trigger_on_release),                       \
        .hold_trigger_key_positions = DT_INST_PROP(n, hold_trigger_key_positions),                 \
        .hold_trigger_key_positions_len = DT_INST_PROP_LEN(n, hold_trigger_key_positions),         \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, behavior_hold_tap_init, NULL, NULL, &behavior_hold_tap_config_##n,    \
                          APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                        \
                          &behavior_hold_tap_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
