/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_indicator_leds

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/pm/device.h>

#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/hid_indicators.h>
#include <zmk/usb.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/usb_conn_state_changed.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct indicator_led_child_config {
    size_t leds_len;
    const struct led_dt_spec *leds;

    zmk_hid_indicators_t indicator;
    uint8_t active_brightness;
    uint8_t inactive_brightness;
    uint8_t disconnected_brightness;
    bool on_while_idle;
};

struct indicator_led_config {
    size_t indicators_len;
    const struct indicator_led_child_config *indicators;
};

struct indicator_led_data {
    enum zmk_activity_state activity_state;
    zmk_hid_indicators_t indicators;
    bool usb_powered;
    bool pm_suspended;
    bool endpoint_connected;
};

static bool is_led_disabled(const struct indicator_led_child_config *config,
                            const struct indicator_led_data *data) {
    // LEDs should always be off if the device is suspended.
    if (data->pm_suspended) {
        return true;
    }

    // If the keyboard is powered, LEDs don't need to be disabled to save power.
    if (data->usb_powered) {
        return false;
    }

    switch (data->activity_state) {
    case ZMK_ACTIVITY_ACTIVE:
        return false;

    case ZMK_ACTIVITY_IDLE:
        return !config->on_while_idle;

    case ZMK_ACTIVITY_SLEEP:
        return true;
    }

    LOG_ERR("Unhandled activity state %d", data->activity_state);
    return false;
}

static uint8_t get_brightness(const struct indicator_led_child_config *config,
                              const struct indicator_led_data *data) {
    if (is_led_disabled(config, data)) {
        return 0;
    }

    if (!data->endpoint_connected) {
        return config->disconnected_brightness;
    }

    const bool active = (data->indicators & config->indicator) == config->indicator;
    return active ? config->active_brightness : config->inactive_brightness;
}

static int update_indicator(const struct indicator_led_child_config *config,
                            const struct indicator_led_data *data) {
    const uint8_t value = get_brightness(config, data);

    for (int i = 0; i < config->leds_len; i++) {
        const struct led_dt_spec *spec = &config->leds[i];
        const int err = led_set_brightness_dt(spec, value);
        if (err) {
            LOG_ERR("Failed to set %s %u to %u%%: %d", spec->dev->name, spec->index, value, err);
            return err;
        }

        LOG_DBG("Set %s %u to %u%%", spec->dev->name, spec->index, value);
    }

    return 0;
}

static int update_device(const struct device *dev) {
    const struct indicator_led_config *config = dev->config;
    struct indicator_led_data *data = dev->data;

    data->activity_state = zmk_activity_get_state();
    data->indicators = zmk_hid_indicators_get_current_profile();
    data->usb_powered = zmk_usb_is_powered();
    data->endpoint_connected = zmk_endpoint_is_connected();

    for (int i = 0; i < config->indicators_len; i++) {
        const int err = update_indicator(&config->indicators[i], data);
        if (err) {
            return err;
        }
    }

    return 0;
}

#define INST_DEV(n) DEVICE_DT_GET(DT_DRV_INST(n)),
static const struct device *all_instances[] = {DT_INST_FOREACH_STATUS_OKAY(INST_DEV)};

static void update_all_indicators(struct k_work *work) {
    LOG_DBG("Updating indicator LEDs");

    for (int i = 0; i < ARRAY_SIZE(all_instances); i++) {
        if (device_is_ready(all_instances[i])) {
            update_device(all_instances[i]);
        }
    }
}

// We may get multiple events at the same time (e.g. endpoint changed will
// also trigger HID indicators changed), but we only need to update the LEDs
// once per batch of events, so defer the updates with a work item.
static K_WORK_DEFINE(update_all_indicators_work, update_all_indicators);

static int indicator_led_event_listener(const zmk_event_t *eh) {
    k_work_submit(&update_all_indicators_work);
    return ZMK_EV_EVENT_BUBBLE;
}

static int indicator_led_init(const struct device *dev) { return update_device(dev); }

ZMK_LISTENER(indicator_led, indicator_led_event_listener);
ZMK_SUBSCRIPTION(indicator_led, zmk_activity_state_changed);
ZMK_SUBSCRIPTION(indicator_led, zmk_hid_indicators_changed);
ZMK_SUBSCRIPTION(indicator_led, zmk_usb_conn_state_changed);
ZMK_SUBSCRIPTION(indicator_led, zmk_endpoint_changed);

#if IS_ENABLED(CONFIG_PM_DEVICE)

static int indicator_led_init_pm_action(const struct device *dev, enum pm_device_action action) {
    struct indicator_led_data *data = dev->data;

    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        data->pm_suspended = true;
        return update_device(dev);

    case PM_DEVICE_ACTION_RESUME:
        data->pm_suspended = false;
        return update_device(dev);

    default:
        return -ENOTSUP;
    }
}

#endif // IS_ENABLED(CONFIG_PM_DEVICE)

#define LED_DT_SPEC_GET_BY_IDX(node_id, prop, idx)                                                 \
    LED_DT_SPEC_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define CHILD_LEDS_ARRAY(inst) DT_CAT(indicator_led_dt_spec_, inst)

#define DEFINE_CHILD_LEDS(inst)                                                                    \
    static const struct led_dt_spec CHILD_LEDS_ARRAY(inst)[] = {                                   \
        DT_FOREACH_PROP_ELEM_SEP(inst, leds, LED_DT_SPEC_GET_BY_IDX, (, )),                        \
    };

#define CHILD_CONFIG(inst)                                                                         \
    {                                                                                              \
        .leds_len = ARRAY_SIZE(CHILD_LEDS_ARRAY(inst)),                                            \
        .leds = CHILD_LEDS_ARRAY(inst),                                                            \
        .indicator = DT_PROP(inst, indicator),                                                     \
        .active_brightness = DT_PROP_OR(inst, active_brightness, 100),                             \
        .inactive_brightness = DT_PROP_OR(inst, inactive_brightness, 0),                           \
        .disconnected_brightness = DT_PROP_OR(inst, disconnected_brightness, 0),                   \
        .on_while_idle = DT_PROP_OR(inst, on_while_idle, false),                                   \
    },

#define INDICATOR_LED_DEVICE(n)                                                                    \
    DT_INST_FOREACH_CHILD(n, DEFINE_CHILD_LEDS)                                                    \
                                                                                                   \
    static const struct indicator_led_child_config indicator_led_children_##n[] = {                \
        DT_INST_FOREACH_CHILD(n, CHILD_CONFIG)};                                                   \
                                                                                                   \
    static const struct indicator_led_config indicator_led_config_##n = {                          \
        .indicators_len = ARRAY_SIZE(indicator_led_children_##n),                                  \
        .indicators = indicator_led_children_##n,                                                  \
    };                                                                                             \
                                                                                                   \
    static struct indicator_led_data indicator_led_data_##n = {                                    \
        .activity_state = ZMK_ACTIVITY_ACTIVE,                                                     \
        .indicators = 0,                                                                           \
        .usb_powered = true,                                                                       \
        .pm_suspended = false,                                                                     \
    };                                                                                             \
                                                                                                   \
    PM_DEVICE_DT_INST_DEFINE(n, indicator_led_init_pm_action);                                     \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(n, &indicator_led_init, PM_DEVICE_DT_INST_GET(n),                        \
                          &indicator_led_data_##n, &indicator_led_config_##n, POST_KERNEL,         \
                          CONFIG_ZMK_INDICATOR_LEDS_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INDICATOR_LED_DEVICE);
