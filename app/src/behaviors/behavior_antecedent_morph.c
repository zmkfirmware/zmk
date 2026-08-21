/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_antecedent_morph

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

// configuration struct per instance
struct behavior_antecedent_morph_config {
  int                          serial;          // serial number of the instance of this behavior
  uint32_t                     max_delay_ms;    // maximum delay between key release and successive key press for the adaptive behavior
  size_t                       defaults_len;    // length of the array of default behaviors (must be 1)
  struct zmk_behavior_binding *defaults;        // array of default behaviors
  size_t                       bindings_len;    // length of the array of morphed behaviors
  struct zmk_behavior_binding *bindings;        // array of morphed behaviors
  int32_t                      antecedents_len; // length of the array of antecedents (key codes)
  int32_t                      antecedents[];   // array of antecedents (key codes)
};

// data struct per instance
struct behavior_antecedent_morph_data {
  struct zmk_behavior_binding *pressed_binding; // the actual behavior that was pressed by the adaptive behavior
};

// data shared by all instances
static int32_t code_pressed; // most recently pressed key code (with implicit mods, usage page and keycode)
static int64_t time_pressed; // time stamp in milli-seconds of that key press

static int antecedent_morph_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_antecedent_morph, antecedent_morph_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_antecedent_morph,zmk_keycode_state_changed);

// Capture all key press and release events in order to record the most recently released key code.
//
// Note that the event structure gives us the keycode (16 bit), the usage page (8 bit) and the implicit modifiers (8
// bit), but not the explicit modifiers. If the keymap contains the binding "&kp RA(Y)", for example, then right-alt is
// an implicit modifier so that instead of the Y, the special character Ü is sent (US International layout).
//
// Whether the user is holding down a shift key at that moment, however, i.e. the explicit modifiers, is not known. We
// could reconstruct this information by tracking the press and release events of the modifier keys (keycodes higher
// than 0xe0) though, but in the present version, the potential antecedents are recorded without modifiers.
//
// We here record all key release events of non-modifier keys (keycodes less than 0xe0).
//
// If someone somewhere triggers a key down event with an illegal key code (beyond 0xff), this key code is recorded as a
// potential antecedent, but is then discarded. This way, it is possible to trigger 'silent antecedents', e.g. in order
// to create new dead keys.

static int antecedent_morph_keycode_state_changed_listener(const zmk_event_t *eh) {

  struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

  int32_t code = ((ev->implicit_modifiers & 0xff) << 24) | ((ev->usage_page & 0xff) << 16) | (ev->keycode & 0xffff);

  LOG_DBG("%s keycode %d; page %d; implicit mods %d; explicit mods %d; key code 0x%08x",ev->state ? "down" : "up",ev->keycode,ev->usage_page,ev->implicit_modifiers,ev->explicit_modifiers,code);
  if ((ev->state) && ((ev->keycode < 0xe0) || (ev->keycode > 0xff))) {
    LOG_DBG("global <code_pressed> variable changes from 0x%08x to 0x%08x",code_pressed,code);
    code_pressed = code;
    time_pressed = ev->timestamp;
  }

  if (ev->keycode > 0xff) {
    LOG_DBG("event dropped");
    return(ZMK_EV_EVENT_HANDLED);
  } else {
    return(ZMK_EV_EVENT_BUBBLE);
  }
}

// When an antecedent morph binding is pressed, we test whether the most recently released key code is among the
// configured antecedents and whether the corresponding key release event was no more than the configured delay time
// ago.

static int on_antecedent_morph_binding_pressed(struct zmk_behavior_binding *binding,
					       struct zmk_behavior_binding_event event) {
  const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
  const struct behavior_antecedent_morph_config *cfg = dev->config;
  struct behavior_antecedent_morph_data *data = dev->data;
  int morph = -1;

  LOG_DBG("press zmk,behavior-antecedent-morph serial no. %d when <code_pressed> is 0x%08x; delay %dms; and explicit_mods 0x%02x",
	  cfg->serial,code_pressed,(int32_t)(event.timestamp-time_pressed),zmk_hid_get_explicit_mods());

  if (data->pressed_binding != NULL) {
    LOG_ERR("Can't press the same antecedent-morph twice");
    return -ENOTSUP;
  }

  for (int i=0;i<cfg->antecedents_len;i++) {
    if (code_pressed == cfg->antecedents[i]) {
      morph = i;
    }
  }

  if ((morph >= 0) && ((int32_t)(event.timestamp-time_pressed)) < cfg->max_delay_ms) {

    // If the the delay between the most recent key release and the pressing of the current behavior is less than the
    // configured maximum delay and if the most recently released key is among the recorded antecedents, issue the
    // behavior among the 'bindings' that is at the corresponding position to the antecedent among the 'antecedents'.
    //
    // Note that should one of the arrays 'bindings' or 'defaults' are too short, an error is triggered and the behavior
    // never pressed.

    LOG_DBG("morph condition satisfied");

    if (morph < cfg->bindings_len) {
      data->pressed_binding = (struct zmk_behavior_binding *)&cfg->bindings[morph];
    } else {
      LOG_ERR("Property 'bindings' must be an array at least of length %d.",morph+1);
      return -ENOTSUP;
    }

  } else {

    // Otherwise, issue the first behavior of the 'defaults' array.

    if (0 < cfg->defaults_len) {
      data->pressed_binding = (struct zmk_behavior_binding *)&cfg->defaults[0];
    } else {
      LOG_ERR("Property 'defaults' must be an array at least of length 1.");
      return -ENOTSUP;
    }
  }

  return zmk_behavior_invoke_binding(data->pressed_binding,event,true);
}

// The release of the antecedent morph behavior considers the behavior that was recorded in the instance data and
// releases it.

static int on_antecedent_morph_binding_released(struct zmk_behavior_binding *binding,
						struct zmk_behavior_binding_event event) {
  const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
  const struct behavior_antecedent_morph_config *cfg = dev->config;
  struct behavior_antecedent_morph_data *data = dev->data;

  if (data->pressed_binding == NULL) {
    LOG_ERR("Antecedent-morph already released");
    return -ENOTSUP;
  }

  LOG_DBG("release zmk,behavior-antecedent-morph serial no. %d",cfg->serial);

  struct zmk_behavior_binding *pressed_binding = data->pressed_binding;
  data->pressed_binding = NULL;
  return zmk_behavior_invoke_binding(pressed_binding,event,false);
}

static const struct behavior_driver_api behavior_antecedent_morph_driver_api = {
  .binding_pressed = on_antecedent_morph_binding_pressed,
  .binding_released = on_antecedent_morph_binding_released,
};

static int behavior_antecedent_morph_init(const struct device *dev) {

  const struct behavior_antecedent_morph_config *cfg = dev->config;
  struct behavior_antecedent_morph_data *data = dev->data;

  LOG_DBG("zmk,behavior-antecedent-morph serial no. %d defined with %d defaults, %d bindings and %d antecedents.",cfg->serial,cfg->defaults_len,cfg->bindings_len,cfg->antecedents_len);
  for (int i=0; i<cfg->antecedents_len;i++) {
    LOG_DBG("antedecent no. %d is 0x%08x.",i,cfg->antecedents[i]);
  }

  data->pressed_binding = NULL;
  code_pressed = 0;
  return 0;
}

#define ZMK_KEYMAP_EXTRACT_DEFAULT(idx, drv_inst)                                                  \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_PHANDLE_BY_IDX(drv_inst, defaults, idx)),                \
        .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(drv_inst, defaults, idx, param1), (0),        \
                              (DT_PHA_BY_IDX(drv_inst, defaults, idx, param1))),                   \
        .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(drv_inst, defaults, idx, param2), (0),        \
                              (DT_PHA_BY_IDX(drv_inst, defaults, idx, param2))),                   \
    }

#define _TRANSFORM_ENTRY_DEFAULT(idx, node) ZMK_KEYMAP_EXTRACT_DEFAULT(idx, node)

#define TRANSFORMED_DEFAULTS(node)					                                              \
  { LISTIFY(DT_INST_PROP_LEN(node, defaults), _TRANSFORM_ENTRY_DEFAULT, (, ), DT_DRV_INST(node)) }

#define _TRANSFORM_ENTRY_BINDING(idx, node) ZMK_KEYMAP_EXTRACT_BINDING(idx, node)

#define TRANSFORMED_BINDINGS(node)					                                              \
  { LISTIFY(DT_INST_PROP_LEN(node, bindings), _TRANSFORM_ENTRY_BINDING, (, ), DT_DRV_INST(node)) }

#define KP_INST(n)							                                              \
  static struct zmk_behavior_binding behavior_antecedent_morph_config_##n##_defaults[DT_INST_PROP_LEN(n, defaults)] = \
    TRANSFORMED_DEFAULTS(n);						                                              \
  static struct zmk_behavior_binding behavior_antecedent_morph_config_##n##_bindings[DT_INST_PROP_LEN(n, bindings)] = \
    TRANSFORMED_BINDINGS(n);						                                              \
  static struct behavior_antecedent_morph_config behavior_antecedent_morph_config_##n = {                             \
    .serial = n,							                                              \
    .max_delay_ms = DT_INST_PROP(n, max_delay_ms),			                                              \
    .defaults = behavior_antecedent_morph_config_##n##_defaults,	                                              \
    .defaults_len = DT_INST_PROP_LEN(n, defaults),			                                              \
    .bindings = behavior_antecedent_morph_config_##n##_bindings,                                                      \
    .bindings_len = DT_INST_PROP_LEN(n, bindings),			                                              \
    .antecedents = DT_INST_PROP(n, antecedents),			                                              \
    .antecedents_len = DT_INST_PROP_LEN(n, antecedents)                                                               \
  };									                                              \
  static struct behavior_antecedent_morph_data behavior_antecedent_morph_data_##n = {                                 \
  };									                                              \
  BEHAVIOR_DT_INST_DEFINE(n,behavior_antecedent_morph_init,NULL,&behavior_antecedent_morph_data_##n,   		      \
			&behavior_antecedent_morph_config_##n,		                                              \
			POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                             \
			&behavior_antecedent_morph_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif
