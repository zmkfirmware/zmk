/*
 * Copyright (c) 2020 Cody McGinnis, Okke Formsma
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_tap_hold

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/behavior.h>

#include <zmk/matrix.h>
#include <zmk/endpoints.h>
#include <zmk/event-manager.h>
#include <zmk/events/position-state-changed.h>
#include <zmk/events/keycode-state-changed.h>
#include <zmk/events/modifiers-state-changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_NODE_EXISTS(DT_DRV_INST(0))

/************************************************************  DATA SETUP */
#define ZMK_BHV_TAP_HOLD_MAX_HELD 10
#define ZMK_BHV_TAP_HOLD_MAX_CAPTURED_KC 40

#define TH_TYPE_MOD_PREFERRED 0
#define TH_TYPE_BALANCED 1
#define TH_TYPE_TAP_PREFERRED 2


// increase if you have keyboard with more keys.
#define TH_POSITION_NOT_USED 9999 

struct behavior_tap_hold_behaviors {
  struct zmk_behavior_binding tap;
  struct zmk_behavior_binding hold;
};

typedef k_timeout_t (*timer_func)();

struct behavior_tap_hold_config {
  timer_func tapping_term_ms;
  struct behavior_tap_hold_behaviors* behaviors;
  char* flavor;
};

// this data is specific for each tap-hold
struct active_tap_hold {
  s32_t position;
  bool is_decided;
  bool is_hold;
  const struct behavior_tap_hold_config *config;
  struct k_delayed_work work;
  bool work_is_cancelled;
};

struct active_tap_hold* undecided_tap_hold = NULL;
struct active_tap_hold active_tap_holds[ZMK_BHV_TAP_HOLD_MAX_HELD] = {};
struct position_state_changed* captured_position_events[ZMK_BHV_TAP_HOLD_MAX_CAPTURED_KC] = {};

/************************************************************  CAPTURED POSITION HELPER FUNCTIONS */
int capture_position_event(struct position_state_changed* event) {
  for (int i = 0; i < ZMK_BHV_TAP_HOLD_MAX_CAPTURED_KC; i++) {
    if (captured_position_events[i] == NULL) {
      captured_position_events[i] = event;
      return 0;
    }
  }
  return -ENOMEM;
}

struct position_state_changed* find_captured_keydown_event(u32_t position)
{
  struct position_state_changed *last_match = NULL;
  for (int i = 0; i < ZMK_BHV_TAP_HOLD_MAX_CAPTURED_KC; i++) {
    struct position_state_changed* event = captured_position_events[i];
    if (event == NULL) {
      return last_match;
    }

    if (captured_position_events[i]->position == position && captured_position_events[i]->state) {
      last_match = event;
    }
  }

  return last_match;
}

int behavior_tap_hold_listener(const struct zmk_event_header *eh);
void release_captured_positions() {
  if (undecided_tap_hold != NULL) {
    return;
  }

  // We use a trick to prevent copying the captured_position_events array.
  //
  // Events for different mod-tap instances are separated by a NULL pointer.
  //
  // The first event popped will never be catched by the next active tap-hold
  // because to start capturing a mod-tap-key-down event must first completely
  // go through the events queue.
  // 
  // Example of this release process;
  // [mt2_down, k1_down, k1_up, mt2_up, null, ...]
  //  ^
  // mt2_down position event isn't captured because no tap-hold is active.
  // mt2_down behavior event is handled, now we have an undecided tap-hold
  // [null, k1_down, k1_up, mt2_up, null, ...]
  //        ^
  // k1_down  is captured by the mt2 mod-tap
  // !note that searches for find_captured_position_event by the mt2 behavior will stop at the first null encountered
  // [mt1_down, null, k1_up, mt2_up, null, ...]
  //                  ^
  // k1_up event is captured by the new tap-hold:
  // [k1_down, k1_up, null, mt2_up, null, ...]
  //                        ^
  // mt2_up event is not captured but causes release of mt2 behavior
  // [k1_down, k1_up, null, null, null, ...]
  // now mt2 will start releasing it's own captured positions.
  for(int i = 0; i < ZMK_BHV_TAP_HOLD_MAX_CAPTURED_KC; i++) {
    struct position_state_changed* captured_position = captured_position_events[i];
    if(captured_position == NULL) {
      return;
    }
    captured_position_events[i] = NULL;
    if(undecided_tap_hold != NULL) {
      k_msleep(10);
    }
    LOG_DBG("Releasing key position event for position %d %s", captured_position->position, (captured_position->state ? "pressed" : "released"));
    ZMK_EVENT_RELEASE_AGAIN(captured_position);
  }
}


/************************************************************  ACTIVE TAP HOLD HELPER FUNCTIONS */

struct active_tap_hold* find_tap_hold(u32_t position) 
{
  for (int i = 0; i < ZMK_BHV_TAP_HOLD_MAX_HELD; i++) {
    if (active_tap_holds[i].position == position) {
      return &active_tap_holds[i];
    }
  }
  return NULL;
}

struct active_tap_hold* store_tap_hold(u32_t position, const struct behavior_tap_hold_config* config) 
{
  for (int i = 0; i < ZMK_BHV_TAP_HOLD_MAX_HELD; i++) {
    if (active_tap_holds[i].position != TH_POSITION_NOT_USED) {
      continue;
    }
    active_tap_holds[i].position = position;
    active_tap_holds[i].is_decided = false;
    active_tap_holds[i].is_hold = false;
    active_tap_holds[i].config = config;
    return &active_tap_holds[i];
  }
  return NULL;
}

void clear_tap_hold(struct active_tap_hold * tap_hold) 
{
  tap_hold->position = TH_POSITION_NOT_USED;
  tap_hold->is_decided = false;
  tap_hold->is_hold = false;
  tap_hold->work_is_cancelled= false;
}

enum decision_moment{
  TH_KEY_UP = 0,
  TH_OTHER_KEY_DOWN = 1,
  TH_OTHER_KEY_UP = 2,
  TH_TIMER_EVENT = 3,
};

static void decide_balanced(struct active_tap_hold * tap_hold, enum decision_moment event) {
    switch(event) {
    case TH_KEY_UP:
      tap_hold->is_hold = 0;
      tap_hold->is_decided = true;
      break;
    case TH_OTHER_KEY_UP:
      tap_hold->is_hold = 1;
      tap_hold->is_decided = true;
      break;
    case TH_TIMER_EVENT:
      tap_hold->is_hold = 1;
      tap_hold->is_decided = true;
      break;
    default: return;
  }
}

static void decide_tap_preferred(struct active_tap_hold * tap_hold, enum decision_moment event) {
    switch(event) {
    case TH_KEY_UP:
      tap_hold->is_hold = 0;
      tap_hold->is_decided = true;
      break;
    case TH_TIMER_EVENT:
      tap_hold->is_hold = 1;
      tap_hold->is_decided = true;
      break;
    default: return;
  }
}

static void decide_hold_preferred(struct active_tap_hold * tap_hold, enum decision_moment event) {
    switch(event) {
    case TH_KEY_UP: 
      tap_hold->is_hold = 0;
      tap_hold->is_decided = true;
      break;
    case TH_OTHER_KEY_DOWN:
      tap_hold->is_hold = 1;
      tap_hold->is_decided = true;
      break;
    case TH_TIMER_EVENT:
      tap_hold->is_hold = 1;
      tap_hold->is_decided = true;
      break;
    default: return;
  }
}


void decide_tap_hold(struct active_tap_hold * tap_hold, enum decision_moment event)
{
  if (tap_hold->is_decided) {
    return;
  }

  if(tap_hold != undecided_tap_hold) {
    LOG_DBG("ERROR found undecided tap hold that is not the active tap hold");
    return;
  }

  char* flavor = tap_hold->config->flavor;
  if(strcmp(flavor, "balanced") == 0) {
    decide_balanced(tap_hold, event);
  } else if(strcmp(flavor, "tap-preferred") == 0) {
    decide_tap_preferred(tap_hold, event);
  } else if(strcmp(flavor, "hold-preferred") == 0) {
    decide_hold_preferred(tap_hold, event);
  }

  if(!tap_hold->is_decided) {
    return;
  }

  LOG_DBG("%d decided %s (%s event %d)", tap_hold->position, tap_hold->is_hold?"hold":"tap", flavor, event);

  undecided_tap_hold = NULL;

  struct zmk_behavior_binding *behavior;
  if (tap_hold->is_hold) {
    behavior = &tap_hold->config->behaviors->hold;
  } else {
    behavior = &tap_hold->config->behaviors->tap;
  }
  struct device *behavior_device = device_get_binding(behavior->behavior_dev);
  behavior_keymap_binding_pressed(behavior_device,  tap_hold->position, behavior->param1, behavior->param2);
  release_captured_positions();
}

/************************************************************  tap_hold_binding and key handlers */
static int on_tap_hold_binding_pressed(struct device *dev, u32_t position, u32_t _, u32_t __)
{
  const struct behavior_tap_hold_config *cfg = dev->config_info;

  if(undecided_tap_hold != NULL) {
    LOG_DBG("ERROR another tap-hold behavior is undecided.");
    // if this happens, make sure the behavior events occur AFTER other position events.
    return 0;
  }

  struct active_tap_hold *tap_hold = store_tap_hold(position, cfg);
  if (tap_hold == NULL) {
    LOG_ERR("unable to store tap-hold info, did you press more than %d tap-holds?", ZMK_BHV_TAP_HOLD_MAX_HELD);
    return 0;
  }

  LOG_DBG("%d new undecided tap_hold", position);
  undecided_tap_hold = tap_hold;
  k_delayed_work_submit(&tap_hold->work, cfg->tapping_term_ms());

  //todo: once we get timing info for keypresses, start the timer relative to the original keypress
  // don't forget to simulate a timer-event before the event after that time was handled.

  return 0;
}

static int on_tap_hold_binding_released(struct device *dev, u32_t position, u32_t _, u32_t __)
{
  struct active_tap_hold *tap_hold = find_tap_hold(position);
  if(tap_hold == NULL) {
    LOG_ERR("ACTIVE_TAP_HOLD_CLEANED_UP_TOO_EARLY");
    return 0;
  }

  int work_cancel_result = k_delayed_work_cancel(&tap_hold->work);
  decide_tap_hold(tap_hold, TH_KEY_UP);

  struct zmk_behavior_binding *behavior;
  if (tap_hold->is_hold) {
    behavior = &tap_hold->config->behaviors->hold;
  } else {
    behavior = &tap_hold->config->behaviors->tap;
  }

  struct device *behavior_device = device_get_binding(behavior->behavior_dev);
  behavior_keymap_binding_released(behavior_device,  tap_hold->position, behavior->param1, behavior->param2);

  if(work_cancel_result == -EINPROGRESS) {
    // let the timer handler clean up
    // if we'd clear now, the timer may call back for an uninitialized active_tap_hold.
    LOG_DBG("%d tap-hold timer work in event queue", position);
    tap_hold->work_is_cancelled = true;
  } else {
    LOG_DBG("%d cleaning up tap-hold", position);
    clear_tap_hold(tap_hold);
  }

  return 0;
}

static const struct behavior_driver_api behavior_tap_hold_driver_api = {
  .binding_pressed = on_tap_hold_binding_pressed,
  .binding_released = on_tap_hold_binding_released,
};

int behavior_tap_hold_listener(const struct zmk_event_header *eh)
{
  if (!is_position_state_changed(eh)) {
    return 0;
  }
  struct position_state_changed* ev = cast_position_state_changed(eh);
  if(undecided_tap_hold == NULL) {
    LOG_DBG("%d bubble (no undecided tap_hold active)", ev->position);
    return 0;
  }

  if(undecided_tap_hold->position == ev->position) {
    if(ev->state) {
      LOG_ERR("this listener should be called before before the behavior listeners!");
      return 0;
    } else {
      LOG_DBG("%d bubble undecided tap-hold keyrelease event", undecided_tap_hold->position);
      return 0;
    }
  }

  LOG_DBG("%d capturing %d %s event", undecided_tap_hold->position, ev->position, ev->state?"down":"up");

  capture_position_event(ev);
  if (ev->state) { 
    decide_tap_hold(undecided_tap_hold, TH_OTHER_KEY_DOWN);
  } else {
    struct position_state_changed* captured_key_down = find_captured_keydown_event(ev->position); 
    if(captured_key_down == NULL) {
      // todo: allow key-up events for non-mod keys pressed before the TH was pressed.
      // see scenario 3c/3d vs 3a/3b. 
    } else {
      decide_tap_hold(undecided_tap_hold, TH_OTHER_KEY_UP); 
    }
  }
  return ZMK_EV_EVENT_CAPTURED;
}

ZMK_LISTENER(behavior_tap_hold, behavior_tap_hold_listener);
ZMK_SUBSCRIPTION(behavior_tap_hold, position_state_changed);

/************************************************************  TIMER FUNCTIONS */
void behavior_tap_hold_timer_work_handler(struct k_work *item)
{
  struct active_tap_hold *tap_hold = CONTAINER_OF(item, struct active_tap_hold, work);
  if(tap_hold->work_is_cancelled) {
    clear_tap_hold(tap_hold);
  } else {
    decide_tap_hold(tap_hold, TH_TIMER_EVENT);
  }
}

static int behavior_tap_hold_init(struct device *dev)
{ 
  static bool init_first_run = true;
  if(init_first_run) {
    for (int i = 0; i < ZMK_BHV_TAP_HOLD_MAX_HELD; i++) {
        k_delayed_work_init(&active_tap_holds[i].work, behavior_tap_hold_timer_work_handler);
        active_tap_holds[i].position = TH_POSITION_NOT_USED;
      }
  }
  init_first_run = false;
  return 0;
}

struct behavior_tap_hold_data {};
static struct behavior_tap_hold_data behavior_tap_hold_data;

/************************************************************ NODE CONFIG */
#define _TRANSFORM_ENTRY(idx, node) \
  { .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)), \
    .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0), (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))), \
    .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0), (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))), \
  },

#define KP_INST(n) \
  static k_timeout_t behavior_tap_hold_config_##n##_gettime() { return K_MSEC(DT_INST_PROP(n, tapping_term_ms)); } \
  static struct behavior_tap_hold_behaviors behavior_tap_hold_behaviors_##n = { \
    .tap = _TRANSFORM_ENTRY(0, n) \
    .hold = _TRANSFORM_ENTRY(1, n) \
  }; \
  static struct behavior_tap_hold_config behavior_tap_hold_config_##n = { \
    .behaviors = &behavior_tap_hold_behaviors_##n, \
    .tapping_term_ms = &behavior_tap_hold_config_##n##_gettime, \
    .flavor = DT_INST_PROP(n, flavor), \
  }; \
  DEVICE_AND_API_INIT(behavior_tap_hold_##n, DT_INST_LABEL(n), behavior_tap_hold_init, \
                      &behavior_tap_hold_data, \
                      &behavior_tap_hold_config_##n, \
                      APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                      &behavior_tap_hold_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)


#endif