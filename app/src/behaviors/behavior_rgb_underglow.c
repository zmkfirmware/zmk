/*
 * Copyright (c) 2020 Nick Winans <nick@winans.codes>
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_rgb_underglow

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_rgb_underglow_config { };
struct behavior_rgb_underglow_data { };

static int behavior_rgb_underglow_init(struct device *dev)
{
  return 0;
}
