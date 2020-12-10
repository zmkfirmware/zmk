/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_hold_tap

#include <device.h>
#include <drivers/behavior.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <logging/log.h>
#include <zmk/behavior.h>
#include <zmk/matrix.h>
#include <zmk/endpoints.h>
#include <zmk/event-manager.h>
#include <zmk/events/position-state-changed.h>
#include <zmk/events/keycode-state-changed.h>
#include <zmk/events/modifiers-state-changed.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_NODE_EXISTS(DT_DRV_INST(0))

#define ZMK_BHV_HOLD_TAP_MAX_HELD 10
#define ZMK_BHV_HOLD_TAP_MAX_CAPTURED_EVENTS 40

// increase if you have keyboard with more keys.
#define ZMK_BHV_HOLD_TAP_POSITION_NOT_USED 9999

enum flavor {
    ZMK_BHV_HOLD_TAP_FLAVOR_HOLD_PREFERRED = 0,
    ZMK_BHV_HOLD_TAP_FLAVOR_BALANCED = 1,
    ZMK_BHV_HOLD_TAP_FLAVOR_TAP_PREFERRED = 2,
};

struct behavior_hold_tap_behaviors {
    struct zmk_behavior_binding tap;
    struct zmk_behavior_binding hold;
};

struct behavior_hold_tap_config {
    int tapping_term_ms;
    struct behavior_hold_tap_behaviors *behaviors;
    enum flavor flavor;
};

// this data is specific for each hold-tap
struct active_hold_tap {
    int32_t position;
    // todo: move these params into the config->behaviors->tap and
    uint32_t param_hold;
    uint32_t param_tap;
    int64_t timestamp;
    bool is_decided;
    bool is_hold;
    const struct behavior_hold_tap_config *config;
    struct k_delayed_work work;
    bool work_is_cancelled;
};

// The undecided hold tap is the hold tap that needs to be decided before
// other keypress events can be released. While the undecided_hold_tap is
// not NULL, most events are captured in captured_events.
// After the hold_tap is decided, it will stay in the active_hold_taps until
// its key-up has been processed and the delayed work is cleaned up.
struct active_hold_tap *undecided_hold_tap = NULL;
struct active_hold_tap active_hold_taps[ZMK_BHV_HOLD_TAP_MAX_HELD] = {};
// We capture most position_state_changed events and some modifiers_state_changed events.
const struct zmk_event_header *captured_events[ZMK_BHV_HOLD_TAP_MAX_CAPTURED_EVENTS] = {};

static int capture_event(const struct zmk_event_header *event) {
    for (int i = 0; i < ZMK_BHV_HOLD_TAP_MAX_CAPTURED_EVENTS; i++) {
        if (captured_events[i] == NULL) {
            captured_events[i] = event;
            return 0;
        }
    }
    return -ENOMEM;
}

static struct position_state_changed *find_captured_keydown_event(uint32_t position) {
    struct position_state_changed *last_match = NULL;
    for (int i = 0; i < ZMK_BHV_HOLD_TAP_MAX_CAPTURED_EVENTS; i++) {
        const struct zmk_event_header *eh = captured_events[i];
        if (eh == NULL) {
            return last_match;
        }
        if (!is_position_state_changed(eh)) {
            continue;
        }
        struct position_state_changed *position_event = cast_position_state_changed(eh);
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
        const struct zmk_event_header *captured_event = captured_events[i];
        if (captured_event == NULL) {
            return;
        }
        captured_events[i] = NULL;
        if (undecided_hold_tap != NULL) {
            k_msleep(10);
        }
        if (is_position_state_changed(captured_event)) {
            struct position_state_changed *position_event =
                cast_position_state_changed(captured_event);
            LOG_DBG("Releasing key position event for position %d %s", position_event->position,
                    (position_event->state ? "pressed" : "released"));
        } else {
            struct keycode_state_changed *modifier_event =
                cast_keycode_state_changed(captured_event);
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
        active_hold_taps[i].position = position;
        active_hold_taps[i].is_decided = false;
        active_hold_taps[i].is_hold = false;
        active_hold_taps[i].config = config;
        active_hold_taps[i].param_hold = param_hold;
        active_hold_taps[i].param_tap = param_tap;
        active_hold_taps[i].timestamp = timestamp;
        return &active_hold_taps[i];
    }
    return NULL;
}

static void clear_hold_tap(struct active_hold_tap *hold_tap) {
    hold_tap->position = ZMK_BHV_HOLD_TAP_POSITION_NOT_USED;
    hold_tap->is_decided = false;
    hold_tap->is_hold = false;
    hold_tap->work_is_cancelled = false;
}

enum decision_moment {
    HT_KEY_UP = 0,
    HT_OTHER_KEY_DOWN = 1,
    HT_OTHER_KEY_UP = 2,
    HT_TIMER_EVENT = 3,
};

static void decide_balanced(struct active_hold_tap *hold_tap, enum decision_moment event) {
    switch (event) {
    case HT_KEY_UP:
        hold_tap->is_hold = 0;
        hold_tap->is_decided = true;
        break;
    case HT_OTHER_KEY_UP:
    case HT_TIMER_EVENT:
        hold_tap->is_hold = 1;
        hold_tap->is_decided = true;
        break;
    default:
        return;
    }
}

static void decide_tap_preferred(struct active_hold_tap *hold_tap, enum decision_moment event) {
    switch (event) {
    case HT_KEY_UP:
        hold_tap->is_hold = 0;
        hold_tap->is_decided = true;
        break;
    case HT_TIMER_EVENT:
        hold_tap->is_hold = 1;
        hold_tap->is_decided = true;
        break;
    default:
        return;
    }
}

static void decide_hold_preferred(struct active_hold_tap *hold_tap, enum decision_moment event) {
    switch (event) {
    case HT_KEY_UP:
        hold_tap->is_hold = 0;
        hold_tap->is_decided = true;
        break;
    case HT_OTHER_KEY_DOWN:
    case HT_TIMER_EVENT:
        hold_tap->is_hold = 1;
        hold_tap->is_decided = true;
        break;
    default:
        return;
    }
}

static inline char *flavor_str(enum flavor flavor) {
    switch (flavor) {
    case ZMK_BHV_HOLD_TAP_FLAVOR_HOLD_PREFERRED:
        return "hold-preferred";
    case ZMK_BHV_HOLD_TAP_FLAVOR_BALANCED:
        return "balanced";
    case ZMK_BHV_HOLD_TAP_FLAVOR_TAP_PREFERRED:
        return "tap-preferred";
    }
    return "UNKNOWN FLAVOR";
}

static void decide_hold_tap(struct active_hold_tap *hold_tap, enum decision_moment event_type) {
    if (hold_tap->is_decided) {
        return;
    }

    if (hold_tap != undecided_hold_tap) {
        LOG_DBG("ERROR found undecided tap hold that is not the active tap hold");
        return;
    }

    switch (hold_tap->config->flavor) {
    case ZMK_BHV_HOLD_TAP_FLAVOR_HOLD_PREFERRED:
        decide_hold_preferred(hold_tap, event_type);
    case ZMK_BHV_HOLD_TAP_FLAVOR_BALANCED:
        decide_balanced(hold_tap, event_type);
    case ZMK_BHV_HOLD_TAP_FLAVOR_TAP_PREFERRED:
        decide_tap_preferred(hold_tap, event_type);
    }

    if (!hold_tap->is_decided) {
        return;
    }

    LOG_DBG("%d decided %s (%s event %d)", hold_tap->position, hold_tap->is_hold ? "hold" : "tap",
            flavor_str(hold_tap->config->flavor), event_type);
    undecided_hold_tap = NULL;

    struct zmk_behavior_binding_event event = {
        .position = hold_tap->position,
        .timestamp = hold_tap->timestamp,
    };

    struct zmk_behavior_binding binding;
    if (hold_tap->is_hold) {
        binding.behavior_dev = hold_tap->config->behaviors->hold.behavior_dev;
        binding.param1 = hold_tap->param_hold;
        binding.param2 = 0;
    } else {
        binding.behavior_dev = hold_tap->config->behaviors->tap.behavior_dev;
        binding.param1 = hold_tap->param_tap;
        binding.param2 = 0;
    }
    behavior_keymap_binding_pressed(&binding, event);
    release_captured_events();
}

static int on_hold_tap_binding_pressed(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_hold_tap_config *cfg = dev->config;

    if (undecided_hold_tap != NULL) {
        LOG_DBG("ERROR another hold-tap behavior is undecided.");
        // if this happens, make sure the behavior events occur AFTER other position events.
        return 0;
    }

    struct active_hold_tap *hold_tap =
        store_hold_tap(event.position, binding->param1, binding->param2, event.timestamp, cfg);
    if (hold_tap == NULL) {
        LOG_ERR("unable to store hold-tap info, did you press more than %d hold-taps?",
                ZMK_BHV_HOLD_TAP_MAX_HELD);
        return 0;
    }

    LOG_DBG("%d new undecided hold_tap", event.position);
    undecided_hold_tap = hold_tap;

    // if this behavior was queued we have to adjust the timer to only
    // wait for the remaining time.
    int32_t tapping_term_ms_left = (hold_tap->timestamp + cfg->tapping_term_ms) - k_uptime_get();
    if (tapping_term_ms_left > 0) {
        k_delayed_work_submit(&hold_tap->work, K_MSEC(tapping_term_ms_left));
    }

    return 0;
}

static int on_hold_tap_binding_released(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    struct active_hold_tap *hold_tap = find_hold_tap(event.position);
    if (hold_tap == NULL) {
        LOG_ERR("ACTIVE_HOLD_TAP_CLEANED_UP_TOO_EARLY");
        return 0;
    }

    // If these events were queued, the timer event may be queued too late or not at all.
    // We insert a timer event before the TH_KEY_UP event to verify.
    int work_cancel_result = k_delayed_work_cancel(&hold_tap->work);
    if (event.timestamp > (hold_tap->timestamp + hold_tap->config->tapping_term_ms)) {
        decide_hold_tap(hold_tap, HT_TIMER_EVENT);
    }

    decide_hold_tap(hold_tap, HT_KEY_UP);

    // todo: set up the binding and data items inside of the active_hold_tap struct
    struct zmk_behavior_binding_event sub_behavior_data = {
        .position = hold_tap->position,
        .timestamp = hold_tap->timestamp,
    };

    struct zmk_behavior_binding sub_behavior_binding;
    if (hold_tap->is_hold) {
        sub_behavior_binding.behavior_dev = hold_tap->config->behaviors->hold.behavior_dev;
        sub_behavior_binding.param1 = hold_tap->param_hold;
        sub_behavior_binding.param2 = 0;
    } else {
        sub_behavior_binding.behavior_dev = hold_tap->config->behaviors->tap.behavior_dev;
        sub_behavior_binding.param1 = hold_tap->param_tap;
        sub_behavior_binding.param2 = 0;
    }
    behavior_keymap_binding_released(&sub_behavior_binding, sub_behavior_data);

    if (work_cancel_result == -EINPROGRESS) {
        // let the timer handler clean up
        // if we'd clear now, the timer may call back for an uninitialized active_hold_tap.
        LOG_DBG("%d hold-tap timer work in event queue", event.position);
        hold_tap->work_is_cancelled = true;
    } else {
        LOG_DBG("%d cleaning up hold-tap", event.position);
        clear_hold_tap(hold_tap);
    }

    return 0;
}

static const struct behavior_driver_api behavior_hold_tap_driver_api = {
    .binding_pressed = on_hold_tap_binding_pressed,
    .binding_released = on_hold_tap_binding_released,
};

static int position_state_changed_listener(const struct zmk_event_header *eh) {
    struct position_state_changed *ev = cast_position_state_changed(eh);

    if (undecided_hold_tap == NULL) {
        LOG_DBG("%d bubble (no undecided hold_tap active)", ev->position);
        return 0;
    }

    if (undecided_hold_tap->position == ev->position) {
        if (ev->state) { // keydown
            LOG_ERR("hold-tap listener should be called before before most other listeners!");
            return 0;
        } else { // keyup
            LOG_DBG("%d bubble undecided hold-tap keyrelease event", undecided_hold_tap->position);
            return 0;
        }
    }

    // If these events were queued, the timer event may be queued too late or not at all.
    // We make a timer decision before the other key events are handled if the timer would
    // have run out.
    if (ev->timestamp >
        (undecided_hold_tap->timestamp + undecided_hold_tap->config->tapping_term_ms)) {
        decide_hold_tap(undecided_hold_tap, HT_TIMER_EVENT);
    }

    if (!ev->state && find_captured_keydown_event(ev->position) == NULL) {
        // no keydown event has been captured, let it bubble.
        // we'll catch modifiers later in modifier_state_changed_listener
        LOG_DBG("%d bubbling %d %s event", undecided_hold_tap->position, ev->position,
                ev->state ? "down" : "up");
        return 0;
    }

    LOG_DBG("%d capturing %d %s event", undecided_hold_tap->position, ev->position,
            ev->state ? "down" : "up");
    capture_event(eh);
    decide_hold_tap(undecided_hold_tap, ev->state ? HT_OTHER_KEY_DOWN : HT_OTHER_KEY_UP);
    return ZMK_EV_EVENT_CAPTURED;
}

static inline bool only_mods(struct keycode_state_changed *ev) {
    return ev->usage_page == HID_USAGE_KEY && ev->keycode >= HID_USAGE_KEY_KEYBOARD_LEFTCONTROL &&
           ev->keycode <= HID_USAGE_KEY_KEYBOARD_RIGHT_GUI;
}

static int keycode_state_changed_listener(const struct zmk_event_header *eh) {
    // we want to catch layer-up events too... how?
    struct keycode_state_changed *ev = cast_keycode_state_changed(eh);

    if (undecided_hold_tap == NULL) {
        // LOG_DBG("0x%02X bubble (no undecided hold_tap active)", ev->keycode);
        return 0;
    }

    if (!only_mods(ev)) {
        // LOG_DBG("0x%02X bubble (not a mod)", ev->keycode);
        return 0;
    }

    // only key-up events will bubble through position_state_changed_listener
    // if a undecided_hold_tap is active.
    LOG_DBG("%d capturing 0x%02X %s event", undecided_hold_tap->position, ev->keycode,
            ev->state ? "down" : "up");
    capture_event(eh);
    return ZMK_EV_EVENT_CAPTURED;
}

int behavior_hold_tap_listener(const struct zmk_event_header *eh) {
    if (is_position_state_changed(eh)) {
        return position_state_changed_listener(eh);
    } else if (is_keycode_state_changed(eh)) {
        return keycode_state_changed_listener(eh);
    }
    return 0;
}

ZMK_LISTENER(behavior_hold_tap, behavior_hold_tap_listener);
ZMK_SUBSCRIPTION(behavior_hold_tap, position_state_changed);
// this should be modifiers_state_changed, but unfrotunately that's not implemented yet.
ZMK_SUBSCRIPTION(behavior_hold_tap, keycode_state_changed);

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
            k_delayed_work_init(&active_hold_taps[i].work, behavior_hold_tap_timer_work_handler);
            active_hold_taps[i].position = ZMK_BHV_HOLD_TAP_POSITION_NOT_USED;
        }
    }
    init_first_run = false;
    return 0;
}

struct behavior_hold_tap_data {};
static struct behavior_hold_tap_data behavior_hold_tap_data;

/* todo: get rid of unused param1 and param2. */
#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),                     \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    },

#define KP_INST(n)                                                                                 \
    static struct behavior_hold_tap_behaviors behavior_hold_tap_behaviors_##n = {                  \
        .hold = _TRANSFORM_ENTRY(0, n).tap = _TRANSFORM_ENTRY(1, n)};                              \
    static struct behavior_hold_tap_config behavior_hold_tap_config_##n = {                        \
        .behaviors = &behavior_hold_tap_behaviors_##n,                                             \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                       \
        .flavor = DT_ENUM_IDX(DT_DRV_INST(n), flavor),                                             \
    };                                                                                             \
    DEVICE_AND_API_INIT(behavior_hold_tap_##n, DT_INST_LABEL(n), behavior_hold_tap_init,           \
                        &behavior_hold_tap_data, &behavior_hold_tap_config_##n, APPLICATION,       \
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_hold_tap_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif