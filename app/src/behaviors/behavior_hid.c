/*
 * Copyright (c) 2020 Peter Johanson <peter@peterjohanson.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_hid

#include <device.h>
#include <power/reboot.h>
#include <drivers/behavior.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/hid.h>
#include <zmk/endpoints.h>

struct behavior_hid_config { };
struct behavior_hid_data { };

static int behavior_hid_init(struct device *dev)
{
	return 0;
};

static int on_keycode_pressed(struct device *dev, u32_t keycode)
{
  enum zmk_hid_report_changes changes;
  LOG_DBG("keycode %d", keycode);
  
  changes = zmk_hid_press_key(keycode);
  return zmk_endpoints_send_report(changes);
}

static int on_keycode_released(struct device *dev, u32_t keycode)
{
  enum zmk_hid_report_changes changes;
  LOG_DBG("keycode %d", keycode);
  
  changes = zmk_hid_release_key(keycode);
  return zmk_endpoints_send_report(changes);
}

static int on_modifiers_pressed(struct device *dev, zmk_mod_flags modifiers)
{
  LOG_DBG("modifiers %d", modifiers);
  
  zmk_hid_register_mods(modifiers);
  return zmk_endpoints_send_report(Keypad);
}

static int on_modifiers_released(struct device *dev, zmk_mod_flags modifiers)
{
  LOG_DBG("modifiers %d", modifiers);
  
  zmk_hid_unregister_mods(modifiers);
  return zmk_endpoints_send_report(Keypad);
}

static const struct behavior_driver_api behavior_hid_driver_api = {
  .keycode_pressed = on_keycode_pressed,
  .keycode_released = on_keycode_released,
  .modifiers_pressed = on_modifiers_pressed,
  .modifiers_released = on_modifiers_released
};


static const struct behavior_hid_config behavior_hid_config = {};

static struct behavior_hid_data behavior_hid_data;

DEVICE_AND_API_INIT(behavior_hid, DT_INST_LABEL(0), behavior_hid_init,
                    &behavior_hid_data,
                    &behavior_hid_config,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_hid_driver_api);