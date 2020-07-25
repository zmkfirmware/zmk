/*
 * Copyright (c) 2020 Nick Winans <nick@winans.codes>
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_rgb_underglow

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <dt-bindings/zmk/rgb.h>
#include <zmk/rgb_underglow.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_rgb_underglow_config { };
struct behavior_rgb_underglow_data { };

static int behavior_rgb_underglow_init(struct device *dev)
{
  return 0;
}

static int on_keymap_binding_pressed(struct device *dev, u32_t position, u32_t action, u32_t _)
{
  switch (action)
  {
    case RGB_TOG:
      zmk_rgb_underglow_toggle();
      break;
    case RGB_HUI:
      zmk_rgb_underglow_change_hue(1);
      break;
    case RGB_HUD:
      zmk_rgb_underglow_change_hue(-1);
      break;
    case RGB_SAI:
      zmk_rgb_underglow_change_sat(1);
      break;
    case RGB_SAD:
      zmk_rgb_underglow_change_sat(-1);
      break;
    case RGB_BRI:
      zmk_rgb_underglow_change_brt(1);
      break;
    case RGB_BRD:
      zmk_rgb_underglow_change_brt(-1);
      break;
    case RGB_SPI:
      zmk_rgb_underglow_change_spd(1);
      break;
    case RGB_SPD:
      zmk_rgb_underglow_change_spd(-1);
      break;
    case RGB_EFF:
      zmk_rgb_underglow_cycle_effect(1);
      break;
    case RGB_EFR:
      zmk_rgb_underglow_cycle_effect(-1);
      break;
  }

  return 0;
}

static const struct behavior_driver_api behavior_rgb_underglow_driver_api = {
  .binding_pressed = on_keymap_binding_pressed,
};

static const struct behavior_rgb_underglow_config behavior_rgb_underglow_config = {};

static struct behavior_rgb_underglow_data behavior_rgb_underglow_data;

DEVICE_AND_API_INIT(behavior_rgb_underglow, DT_INST_LABEL(0), behavior_rgb_underglow_init,
                    &behavior_rgb_underglow_data,
                    &behavior_rgb_underglow_config,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_rgb_underglow_driver_api);