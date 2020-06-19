/*
 * Copyright (c) 2020 Peter Johanson <peter@peterjohanson.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_reset

#include <device.h>
#include <power/reboot.h>
#include <drivers/behavior.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_reset_config { };
struct behavior_reset_data { };

static int behavior_reset_init(struct device *dev)
{
	return 0;
};

static int on_position_pressed(struct device *dev, u32_t _param1, u32_t _param2)
{
  // TODO: Correct magic code for going into DFU?
  // See https://github.com/adafruit/Adafruit_nRF52_Bootloader/blob/d6b28e66053eea467166f44875e3c7ec741cb471/src/main.c#L107
  sys_reboot(0);
  return 0;
}

static int on_position_released(struct device *dev, u32_t _param1, u32_t _param2)
{
  return 0;
}

static const struct behavior_driver_api behavior_reset_driver_api = {
  .position_pressed = on_position_pressed,
  .position_released = on_position_released
};


static const struct behavior_reset_config behavior_reset_config = {};

static struct behavior_reset_data behavior_reset_data;

DEVICE_AND_API_INIT(behavior_reset, DT_INST_LABEL(0), behavior_reset_init,
                    &behavior_reset_data,
                    &behavior_reset_config,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_reset_driver_api);