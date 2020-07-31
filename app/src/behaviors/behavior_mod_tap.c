/*
 * Copyright (c) 2020 Peter Johanson <peter@peterjohanson.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_mod_tap

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/matrix.h>
#include <zmk/endpoints.h>
#include <zmk/event-manager.h>
#include <zmk/events/keycode-state-changed.h>
#include <zmk/events/modifiers-state-changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define ZMK_BHV_MOD_TAP_MAX_HELD 4
#define ZMK_BHV_MOD_TAP_MAX_PENDING_KC 4

#define TOGGLE_FIRST(arr, len, match, val) \
    for (int idx = 0; idx < len; idx++)    \
    {                                      \
        if (arr[idx] != match)             \
        {                                  \
            continue;                      \
        }                                  \
        arr[idx] = val;                    \
        break;                             \
    }

struct pending_mod_tap_item {
  u32_t keycode;
  u8_t mods;
};

struct behavior_mod_tap_config { };
struct behavior_mod_tap_data {
  struct pending_mod_tap_item pending_mod_taps[ZMK_BHV_MOD_TAP_MAX_HELD];
  struct pending_mod_tap_item triggered_mod_taps[ZMK_BHV_MOD_TAP_MAX_HELD];
  struct keycode_state_changed* pending_key_presses[ZMK_BHV_MOD_TAP_MAX_PENDING_KC];
};

bool have_pending_mods(char *label) {
  struct device *dev = device_get_binding(label);
  struct behavior_mod_tap_data *data = dev->driver_data;

  for (int i = 0; i < ZMK_BHV_MOD_TAP_MAX_HELD; i++) {
    if (data->pending_mod_taps[i].mods) {
      LOG_DBG("Found pending mods for %d and keycode 0x%02X", data->pending_mod_taps[i].mods, data->pending_mod_taps[i].keycode);
      return true;
    }
  }

  return false;
}

bool have_pending_keycode(struct behavior_mod_tap_data *data, u32_t keycode)
{
  for (int i = 0; i < ZMK_BHV_MOD_TAP_MAX_PENDING_KC; i++) {
    if (data->pending_key_presses[i] == NULL) {
      continue;
    }

    if (data->pending_key_presses[i]->keycode == keycode) {
      return true;
    }
  }

  return false;
}

// How to pass context to subscription?!
int behavior_mod_tap_listener(const struct zmk_event_header *eh)
{
  if (is_keycode_state_changed(eh) && have_pending_mods(DT_INST_LABEL(0))) {
    struct device *dev = device_get_binding(DT_INST_LABEL(0));
    struct keycode_state_changed *ev = cast_keycode_state_changed(eh);
    struct behavior_mod_tap_data *data = dev->driver_data;
    if (ev->state) {
      LOG_DBG("Have pending mods, capturing keycode 0x%02X event to ressend later", ev->keycode);
      TOGGLE_FIRST(data->pending_key_presses, ZMK_BHV_MOD_TAP_MAX_PENDING_KC, NULL, ev);
      return ZMK_EV_EVENT_CAPTURED;
    } else if (have_pending_keycode(data, ev->keycode)) {
      zmk_mod_flags mods = 0;

      LOG_DBG("Key released, going to activate mods then send pending key presses");
      for (int i = 0; i < ZMK_BHV_MOD_TAP_MAX_HELD; i++) {
        memcpy(&data->triggered_mod_taps[i], &data->pending_mod_taps[i], sizeof(struct pending_mod_tap_item));
        data->pending_mod_taps[i].mods = 0;
        data->pending_mod_taps[i].keycode = 0;
      }
      LOG_DBG("After swapping, do I have pending mods? %s", (have_pending_mods(DT_INST_LABEL(0)) ? "true" : "false"));

      for (int i = 0; i < ZMK_BHV_MOD_TAP_MAX_HELD; i++) {
        mods |= data->triggered_mod_taps[i].mods;
      }

      LOG_DBG("Setting mods: %d", mods);

      zmk_hid_register_mods(mods);

      for (int i = 0; i < ZMK_BHV_MOD_TAP_MAX_PENDING_KC; i++) {
        if (data->pending_key_presses[i] == NULL) {
          continue;
        }

        struct keycode_state_changed *ev = data->pending_key_presses[i];
        data->pending_key_presses[i] = NULL;
        ZMK_EVENT_RAISE(ev);
        k_msleep(10);
      }
    }
  }
  return 0;
}

ZMK_LISTENER(behavior_mod_tap, behavior_mod_tap_listener);
ZMK_SUBSCRIPTION(behavior_mod_tap, keycode_state_changed);

static int behavior_mod_tap_init(struct device *dev)
{
	return 0;
};


static int on_keymap_binding_pressed(struct device *dev, u32_t position, u32_t mods, u32_t keycode)
{
  struct behavior_mod_tap_data *data = dev->driver_data;
  LOG_DBG("mods: %d, keycode: 0x%02X", mods, keycode);
  for (int i = 0; i < ZMK_BHV_MOD_TAP_MAX_HELD; i++) {
    if (data->pending_mod_taps[i].mods != 0) {
      continue;
    }

    data->pending_mod_taps[i].mods = mods;
    data->pending_mod_taps[i].keycode = keycode;

    return 0;
  }

  return -ENOMEM;
}

static int on_keymap_binding_released(struct device *dev, u32_t position, u32_t mods, u32_t keycode)
{
  struct behavior_mod_tap_data *data = dev->driver_data;
  bool sending_keycode = false;
  bool sent_pending_key_presses = false;
  struct keycode_state_changed *pending_key_presses[ZMK_BHV_MOD_TAP_MAX_PENDING_KC];
  LOG_DBG("mods: %d, keycode: %d", mods, keycode);
  
  for (int i = 0; i < ZMK_BHV_MOD_TAP_MAX_HELD; i++) {
    if (data->triggered_mod_taps[i].mods == mods && data->triggered_mod_taps[i].keycode == keycode) {
      LOG_DBG("Releasing triggered mods: %d", mods);
      zmk_hid_unregister_mods(mods);
      data->triggered_mod_taps[i].mods = 0;
      data->triggered_mod_taps[i].keycode = 0;
      break;
    }
  }

  for (int i = 0; i < ZMK_BHV_MOD_TAP_MAX_HELD; i++) {
    if (data->pending_mod_taps[i].mods == mods && data->pending_mod_taps[i].keycode == keycode) {
      sending_keycode = true;
      data->pending_mod_taps[i].mods = 0;
      data->pending_mod_taps[i].keycode = 0;
      break;
    }
  }

  for (int i = 0; i < ZMK_BHV_MOD_TAP_MAX_PENDING_KC; i++) {
    pending_key_presses[i] = data->pending_key_presses[i];
    data->pending_key_presses[i] = NULL;
  }


  if (sending_keycode) {
      struct keycode_state_changed *key_press = create_keycode_state_changed(USAGE_KEYPAD, keycode, true);
      LOG_DBG("Sending un-triggered mod-tap for keycode: 0x%02X", keycode);
      ZMK_EVENT_RAISE(key_press);
      k_msleep(10);
  }
  

  for (int i = 0; i < ZMK_BHV_MOD_TAP_MAX_PENDING_KC; i++) {
    if (pending_key_presses[i] == NULL) {
      continue;
    }

    struct keycode_state_changed *ev = pending_key_presses[i];
    sent_pending_key_presses = true;
    LOG_DBG("Re-sending latched key press for usage page 0x%02X keycode 0x%02X state %s", ev->usage_page, ev->keycode, (ev->state ? "pressed" : "released"));
    ZMK_EVENT_RELEASE(ev);
    k_msleep(10);
  }

  if (sending_keycode) {
      struct keycode_state_changed *key_release = create_keycode_state_changed(USAGE_KEYPAD, keycode, false);
      LOG_DBG("Sending un-triggered mod-tap release for keycode: 0x%02X", keycode);
      ZMK_EVENT_RAISE(key_release);
  }

  if (!sending_keycode && !sent_pending_key_presses) {
    // Need to ensure the mod release is propagated.
    zmk_endpoints_send_report(USAGE_KEYPAD);

  }

  return 0;
}

static const struct behavior_driver_api behavior_mod_tap_driver_api = {
  .binding_pressed = on_keymap_binding_pressed,
  .binding_released = on_keymap_binding_released,
};

static const struct behavior_mod_tap_config behavior_mod_tap_config = {};

static struct behavior_mod_tap_data behavior_mod_tap_data;

DEVICE_AND_API_INIT(behavior_mod_tap, DT_INST_LABEL(0), behavior_mod_tap_init,
                    &behavior_mod_tap_data,
                    &behavior_mod_tap_config,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &behavior_mod_tap_driver_api);
