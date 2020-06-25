/*
 * Copyright (c) 2020 Peter Johanson <peter@peterjohanson.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_mod_tap

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/events.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_mod_tap_config { };
struct behavior_mod_tap_data {
  u16_t pending_press_positions;
};

static int behavior_mod_tap_init(struct device *dev)
{
	return 0;
};


static int on_keymap_binding_pressed(struct device *dev, u32_t position, u32_t mods, u32_t keycode)
{
  struct behavior_mod_tap_data *data = dev->driver_data;
  LOG_DBG("mods: %d, keycode: %d", mods, keycode);
  WRITE_BIT(data->pending_press_positions, position, true);
  return zmk_events_modifiers_pressed(mods);
}


// They keycode is passed by the "keymap" based on the parameter created as part of the assignment.
static int on_keymap_binding_released(struct device *dev, u32_t position, u32_t mods, u32_t keycode)
{
  struct behavior_mod_tap_data *data = dev->driver_data;
  LOG_DBG("mods: %d, keycode: %d", mods, keycode);
  
zmk_events_modifiers_released(mods);
  if (data->pending_press_positions & BIT(position)) {
    zmk_events_keycode_pressed(USAGE_KEYPAD, keycode);
    k_msleep(10);
    zmk_events_keycode_released(USAGE_KEYPAD, keycode);
  }

  return 0;
}

static int on_keycode_pressed(struct device *dev, u8_t usage_page, u32_t keycode)
{
  struct behavior_mod_tap_data *data = dev->driver_data;
  data->pending_press_positions = 0;
  LOG_DBG("pressing: %d", keycode);
  return 0;
}

static int on_keycode_released(struct device *dev, u8_t usage_page, u32_t keycode)
{
  LOG_DBG("releasing: %d", keycode);
  return 0;
}

static const struct behavior_driver_api behavior_mod_tap_driver_api = {
  // These callbacks are all optional, and define which kinds of events the behavior can handle.
  // They can reference local functions defined here, or shared event handlers.
  .binding_pressed = on_keymap_binding_pressed,
  .binding_released = on_keymap_binding_released,
  .keycode_pressed = on_keycode_pressed,
  .keycode_released = on_keycode_released
  // Other optional callbacks a behavior can implement
  // .on_mouse_moved
  // .on_sensor_data - Any behaviour that wants to be linked to a censor can implement this behavior
};


static const struct behavior_mod_tap_config behavior_mod_tap_config = {};

static struct behavior_mod_tap_data behavior_mod_tap_data;

DEVICE_AND_API_INIT(behavior_key_press, DT_INST_LABEL(0), behavior_mod_tap_init,
                    &behavior_mod_tap_data,
                    &behavior_mod_tap_config,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_mod_tap_driver_api);
