/*
 * Copyright (c) 2020 Peter Johanson <peter@peterjohanson.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_keymap

#include <device.h>
#include <power/reboot.h>
#include <drivers/behavior.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keymap.h>

struct behavior_keymap_config { };
struct behavior_keymap_data { };

static int behavior_keymap_init(struct device *dev)
{
	return 0;
};

static int on_position_pressed(struct device *dev, u32_t position)
{
  return zmk_keymap_position_state_changed(position, true);
}

static int on_position_released(struct device *dev, u32_t position)
{
  return zmk_keymap_position_state_changed(position, false);
}

static const struct behavior_driver_api behavior_keymap_driver_api = {
  .position_pressed = on_position_pressed,
  .position_released = on_position_released,
};


static const struct behavior_keymap_config behavior_keymap_config = {};

static struct behavior_keymap_data behavior_keymap_data;

DEVICE_AND_API_INIT(behavior_keymap, DT_INST_LABEL(0), behavior_keymap_init,
                    &behavior_keymap_data,
                    &behavior_keymap_config,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_keymap_driver_api);
