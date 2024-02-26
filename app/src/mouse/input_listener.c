/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_listener

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <zmk/mouse.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum input_listener_xy_data_mode {
    INPUT_LISTENER_XY_DATA_MODE_NONE,
    INPUT_LISTENER_XY_DATA_MODE_REL,
    INPUT_LISTENER_XY_DATA_MODE_ABS,
};

struct input_listener_xy_data {
    enum input_listener_xy_data_mode mode;
    int16_t x;
    int16_t y;
};

struct input_listener_config {
    bool xy_swap;
    bool x_invert;
    bool y_invert;
    uint16_t scale_multiplier;
    uint16_t scale_divisor;
    int layer_toggle;
    int layer_toggle_delay_ms;
    int layer_toggle_timeout_ms;
};

struct input_listener_data {
    const struct device *dev;

    struct input_listener_xy_data data;
    struct input_listener_xy_data wheel_data;

    uint8_t button_set;
    uint8_t button_clear;

    bool layer_toggle_layer_enabled;
    int64_t layer_toggle_last_mouse_package_time;
    struct k_work_delayable layer_toggle_activation_delay;
    struct k_work_delayable layer_toggle_deactivation_delay;
};

void zmk_input_listener_layer_toggle_input_rel_received(const struct input_listener_config *config,
                                                        struct input_listener_data *data);

static void handle_rel_code(struct input_listener_data *data, struct input_event *evt) {
    switch (evt->code) {
    case INPUT_REL_X:
        data->data.mode = INPUT_LISTENER_XY_DATA_MODE_REL;
        data->data.x += evt->value;
        break;
    case INPUT_REL_Y:
        data->data.mode = INPUT_LISTENER_XY_DATA_MODE_REL;
        data->data.y += evt->value;
        break;
    case INPUT_REL_WHEEL:
        data->wheel_data.mode = INPUT_LISTENER_XY_DATA_MODE_REL;
        data->wheel_data.y += evt->value;
        break;
    case INPUT_REL_HWHEEL:
        data->wheel_data.mode = INPUT_LISTENER_XY_DATA_MODE_REL;
        data->wheel_data.x += evt->value;
        break;
    default:
        break;
    }
}

static char *get_input_code_name(struct input_event *evt) {
    switch (evt->code) {
    case INPUT_REL_X:
        return "INPUT_REL_X";
    case INPUT_REL_Y:
        return "INPUT_REL_Y";
    case INPUT_REL_WHEEL:
        return "INPUT_REL_WHEEL";
    case INPUT_REL_HWHEEL:
        return "INPUT_REL_HWHEEL";
    case INPUT_BTN_0:
        return "INPUT_BTN_0";
    case INPUT_BTN_1:
        return "INPUT_BTN_1";
    case INPUT_BTN_2:
        return "INPUT_BTN_2";
    case INPUT_BTN_3:
        return "INPUT_BTN_3";
    case INPUT_BTN_4:
        return "INPUT_BTN_4";
    default:
        return "UNKNOWN";
    }
}

static void handle_key_code(struct input_listener_data *data, struct input_event *evt) {
    int8_t btn;

    switch (evt->code) {
    case INPUT_BTN_0:
    case INPUT_BTN_1:
    case INPUT_BTN_2:
    case INPUT_BTN_3:
    case INPUT_BTN_4:
        btn = evt->code - INPUT_BTN_0;
        if (evt->value > 0) {
            WRITE_BIT(data->button_set, btn, 1);
        } else {
            WRITE_BIT(data->button_clear, btn, 1);
        }
        break;
    default:
        break;
    }
}

static void swap_xy(struct input_event *evt) {
    switch (evt->code) {
    case INPUT_REL_X:
        evt->code = INPUT_REL_Y;
        break;
    case INPUT_REL_Y:
        evt->code = INPUT_REL_X;
        break;
    }
}

static void filter_with_input_config(const struct input_listener_config *cfg,
                                     struct input_event *evt) {
    if (!evt->dev) {
        return;
    }

    if (cfg->xy_swap) {
        swap_xy(evt);
    }

    if ((cfg->x_invert && evt->code == INPUT_REL_X) ||
        (cfg->y_invert && evt->code == INPUT_REL_Y)) {
        evt->value = -(evt->value);
    }

    evt->value = (int16_t)((evt->value * cfg->scale_multiplier) / cfg->scale_divisor);
}

static void clear_xy_data(struct input_listener_xy_data *data) {
    data->x = data->y = 0;
    data->mode = INPUT_LISTENER_XY_DATA_MODE_NONE;
}

static void input_handler(const struct input_listener_config *config,
                          struct input_listener_data *data, struct input_event *evt) {
    // First, filter to update the event data as needed.
    filter_with_input_config(config, evt);

    LOG_DBG("Got input_handler event: %s with value 0x%x", get_input_code_name(evt), evt->value);

    zmk_input_listener_layer_toggle_input_rel_received(config, data);

    switch (evt->type) {
    case INPUT_EV_REL:
        handle_rel_code(data, evt);
        break;
    case INPUT_EV_KEY:
        handle_key_code(data, evt);
        break;
    }

    if (evt->sync) {
        if (data->wheel_data.mode == INPUT_LISTENER_XY_DATA_MODE_REL) {
            zmk_hid_mouse_scroll_set(data->wheel_data.x, data->wheel_data.y);
        }

        if (data->data.mode == INPUT_LISTENER_XY_DATA_MODE_REL) {
            zmk_hid_mouse_movement_set(data->data.x, data->data.y);
        }

        if (data->button_set != 0) {
            for (int i = 0; i < ZMK_HID_MOUSE_NUM_BUTTONS; i++) {
                if ((data->button_set & BIT(i)) != 0) {
                    zmk_hid_mouse_button_press(i);
                }
            }
        }

        if (data->button_clear != 0) {
            for (int i = 0; i < ZMK_HID_MOUSE_NUM_BUTTONS; i++) {
                if ((data->button_clear & BIT(i)) != 0) {
                    zmk_hid_mouse_button_release(i);
                }
            }
        }

        zmk_endpoints_send_mouse_report();
        zmk_hid_mouse_scroll_set(0, 0);
        zmk_hid_mouse_movement_set(0, 0);

        clear_xy_data(&data->data);
        clear_xy_data(&data->wheel_data);

        data->button_set = data->button_clear = 0;
    }
}

void zmk_input_listener_layer_toggle_input_rel_received(const struct input_listener_config *config,
                                                        struct input_listener_data *data) {
    if (config->layer_toggle == -1) {
        return;
    }

    data->layer_toggle_last_mouse_package_time = k_uptime_get();

    if (data->layer_toggle_layer_enabled == false) {
        k_work_schedule(&data->layer_toggle_activation_delay,
                        K_MSEC(config->layer_toggle_delay_ms));
    } else {
        // Deactivate the layer if no further movement within
        // layer_toggle_timeout_ms
        k_work_reschedule(&data->layer_toggle_deactivation_delay,
                          K_MSEC(config->layer_toggle_timeout_ms));
    }
}

void zmk_input_listener_layer_toggle_activate_layer(struct k_work *item) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(item);

    struct input_listener_data *data =
        CONTAINER_OF(d_work, struct input_listener_data, layer_toggle_activation_delay);
    const struct input_listener_config *config = data->dev->config;

    int64_t current_time = k_uptime_get();
    int64_t last_mv_within_ms = current_time - data->layer_toggle_last_mouse_package_time;

    if (last_mv_within_ms <= config->layer_toggle_timeout_ms * 0.1) {
        LOG_INF("Activating layer %d due to mouse activity...", config->layer_toggle);

        zmk_keymap_layer_activate(config->layer_toggle, false);
        data->layer_toggle_layer_enabled = true;
    } else {
        LOG_INF("Not activating mouse layer %d, because last mouse activity was %lldms ago",
                config->layer_toggle, last_mv_within_ms);
    }
}

void zmk_input_listener_layer_toggle_deactivate_layer(struct k_work *item) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(item);

    struct input_listener_data *data =
        CONTAINER_OF(d_work, struct input_listener_data, layer_toggle_deactivation_delay);
    const struct input_listener_config *config = data->dev->config;

    LOG_INF("Deactivating layer %d due to mouse activity...", config->layer_toggle);

    if (zmk_keymap_layer_active(config->layer_toggle)) {
        zmk_keymap_layer_deactivate(config->layer_toggle);
    }

    data->layer_toggle_layer_enabled = false;
}

static int zmk_input_listener_layer_toggle_init(const struct input_listener_config *config,
                                                struct input_listener_data *data) {
    k_work_init_delayable(&data->layer_toggle_activation_delay,
                          zmk_input_listener_layer_toggle_activate_layer);
    k_work_init_delayable(&data->layer_toggle_deactivation_delay,
                          zmk_input_listener_layer_toggle_deactivate_layer);

    return 0;
}

#define IL_INST(n)                                                                                 \
    static const struct input_listener_config config_##n = {                                       \
        .xy_swap = DT_INST_PROP(n, xy_swap),                                                       \
        .x_invert = DT_INST_PROP(n, x_invert),                                                     \
        .y_invert = DT_INST_PROP(n, y_invert),                                                     \
        .scale_multiplier = DT_INST_PROP(n, scale_multiplier),                                     \
        .scale_divisor = DT_INST_PROP(n, scale_divisor),                                           \
        .layer_toggle = DT_INST_PROP(n, layer_toggle),                                             \
        .layer_toggle_delay_ms = DT_INST_PROP(n, layer_toggle_delay_ms),                           \
        .layer_toggle_timeout_ms = DT_INST_PROP(n, layer_toggle_timeout_ms),                       \
    };                                                                                             \
    static struct input_listener_data data_##n = {                                                 \
        .dev = DEVICE_DT_INST_GET(n),                                                              \
        .layer_toggle_layer_enabled = false,                                                       \
        .layer_toggle_last_mouse_package_time = 0,                                                 \
    };                                                                                             \
    void input_handler_##n(struct input_event *evt) {                                              \
        input_handler(&config_##n, &data_##n, evt);                                                \
    }                                                                                              \
    INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_INST_PHANDLE(n, device)), input_handler_##n);           \
                                                                                                   \
    static int zmk_input_listener_init_##n(const struct device *dev) {                             \
                                                                                                   \
        struct input_listener_data *data = dev->data;                                              \
        const struct input_listener_config *config = dev->config;                                  \
                                                                                                   \
        zmk_input_listener_layer_toggle_init(config, data);                                        \
                                                                                                   \
        return 0;                                                                                  \
    }                                                                                              \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(n, &zmk_input_listener_init_##n, NULL, &data_##n, &config_##n,           \
                          POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(IL_INST)
