/*
 * Copyright (c) 2020 Cody McGinnis
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>
#include <zmk/behavior.h>

#include <zmk/matrix.h>
#include <zmk/endpoints.h>
#include <zmk/event-manager.h>
#include <zmk/events/keycode-state-changed.h>
#include <zmk/events/modifiers-state-changed.h>
#include <zmk/hid.h>

#define DT_DRV_COMPAT zmk_behavior_tap_hold

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define _TRANSFORM_ENTRY(idx, node) \
  { .behavior_dev = DT_LABEL(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)), \
    .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0), (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))), \
    .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0), (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))), \
  },

struct behavior_tap_hold_behaviors {
  struct zmk_behavior_binding tap;
  struct zmk_behavior_binding hold;
};

typedef k_timeout_t (*timer_func)();

struct behavior_tap_hold_config {
  timer_func hold_ms;
  struct behavior_tap_hold_behaviors* behaviors;
};

struct behavior_tap_hold_data {
  struct k_timer timer;
};

int behavior_tap_hold_listener(const struct zmk_event_header *eh)
{
  return 0;
}

ZMK_LISTENER(behavior_tap_hold, behavior_tap_hold_listener);
ZMK_SUBSCRIPTION(behavior_tap_hold, keycode_state_changed);

static void timer_handler(struct k_timer *timer)
{
  const struct behavior_tap_hold_config *cfg = k_timer_user_data_get(timer);
  const struct behavior_tap_hold_behaviors *behaviors = cfg->behaviors;

  if (k_timer_status_get(timer) == 0) {
    LOG_DBG("timer %p up: tap binding name: %s", timer, log_strdup(behaviors->tap.behavior_dev));
    struct device *behavior = device_get_binding(behaviors->tap.behavior_dev);
    if (behavior) {
      behavior_keymap_binding_pressed(behavior, 0, behaviors->tap.param1, behaviors->tap.param2);
    }
  } else {
    LOG_DBG("timer %p up: hold binding name: %s", timer, log_strdup(behaviors->hold.behavior_dev));
    struct device *behavior = device_get_binding(behaviors->hold.behavior_dev);
    if (behavior) {
      behavior_keymap_binding_pressed(behavior, 0, behaviors->hold.param1, behaviors->hold.param2);
    }
  }
}

static int behavior_tap_hold_init(struct device *dev)
{
  struct behavior_tap_hold_data *data = dev->driver_data;

  k_timer_init(&data->timer, timer_handler, timer_handler);
  k_timer_user_data_set(&data->timer, (void*)dev->config_info);

  return 0;
}

static int on_keymap_binding_pressed(struct device *dev, u32_t position, u32_t _, u32_t __)
{
  struct behavior_tap_hold_data *data = dev->driver_data;
  const struct behavior_tap_hold_config *cfg = dev->config_info;

  LOG_DBG("timer %p started", &data->timer);
  k_timer_start(&data->timer, cfg->hold_ms(), K_NO_WAIT);

  return 0;
}

static int on_keymap_binding_released(struct device *dev, u32_t position, u32_t _, u32_t __)
{
  struct behavior_tap_hold_data *data = dev->driver_data;
  const struct behavior_tap_hold_config *cfg = dev->config_info;
  const struct behavior_tap_hold_behaviors *behaviors = cfg->behaviors;

  uint32_t ticks_left = k_timer_remaining_ticks(&data->timer);
  
  k_timer_stop(&data->timer);

  if (ticks_left > 0) {
    LOG_DBG("key release: tap binding name: %s", log_strdup(behaviors->tap.behavior_dev));
    struct device *behavior = device_get_binding(behaviors->tap.behavior_dev);
    if (behavior) {
      return behavior_keymap_binding_released(behavior, position, behaviors->tap.param1, behaviors->tap.param2);
    }
  } else {
    LOG_DBG("key release: hold binding name: %s", log_strdup(behaviors->hold.behavior_dev));
    struct device *behavior = device_get_binding(behaviors->hold.behavior_dev);
    if (behavior) {
      return behavior_keymap_binding_released(behavior, position, behaviors->hold.param1, behaviors->hold.param2);
    }
  }

  return 0;
}

static const struct behavior_driver_api behavior_tap_hold_driver_api = {
  .binding_pressed = on_keymap_binding_pressed,
  .binding_released = on_keymap_binding_released,
};

#define KP_INST(n) \
  static k_timeout_t behavior_tap_hold_config_##n##_gettime() { return K_MSEC(DT_INST_PROP(n, hold_ms)); } \
  static struct behavior_tap_hold_behaviors behavior_tap_hold_behaviors_##n = { \
    .tap = _TRANSFORM_ENTRY(0, n) \
    .hold = _TRANSFORM_ENTRY(1, n) \
  }; \
  static struct behavior_tap_hold_config behavior_tap_hold_config_##n = { \
    .behaviors = &behavior_tap_hold_behaviors_##n, \
    .hold_ms = &behavior_tap_hold_config_##n##_gettime, \
  }; \
  static struct behavior_tap_hold_data behavior_tap_hold_data_##n; \
  DEVICE_AND_API_INIT(behavior_tap_hold_##n, DT_INST_LABEL(n), behavior_tap_hold_init, \
                      &behavior_tap_hold_data_##n, \
                      &behavior_tap_hold_config_##n, \
                      APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                      &behavior_tap_hold_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)