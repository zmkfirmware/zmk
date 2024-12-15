/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_listener

#include <zephyr/sys/util_macro.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <zmk/endpoints.h>
#include <drivers/input_processor.h>
#include <zmk/pointing.h>

#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
#include <zmk/pointing/resolution_multipliers.h>
#endif // IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)

#include <zmk/hid.h>
#include <zmk/keymap.h>

#define ONE_IF_DEV_OK(n)                                                                           \
    COND_CODE_1(DT_NODE_HAS_STATUS(DT_INST_PHANDLE(n, device), okay), (1 +), (0 +))

#define VALID_LISTENER_COUNT (DT_INST_FOREACH_STATUS_OKAY(ONE_IF_DEV_OK) 0)

#if VALID_LISTENER_COUNT > 0

enum input_listener_xy_data_mode {
    INPUT_LISTENER_XY_DATA_MODE_NONE,
    INPUT_LISTENER_XY_DATA_MODE_REL,
    INPUT_LISTENER_XY_DATA_MODE_ABS,
};

struct input_listener_axis_data {
    int16_t value;
};

struct input_listener_xy_data {
    enum input_listener_xy_data_mode mode;
    struct input_listener_axis_data x;
    struct input_listener_axis_data y;
};

struct input_listener_config_entry {
    size_t processors_len;
    const struct zmk_input_processor_entry *processors;
};

struct input_listener_layer_override {
    uint32_t layer_mask;
    bool process_next;
    struct input_listener_config_entry config;
};

struct input_processor_remainder_data {
    int16_t x, y, wheel, h_wheel;
};

struct input_listener_processor_data {
    size_t remainders_len;
    struct input_processor_remainder_data *remainders;
};

struct input_listener_config {
    uint8_t listener_index;
    struct input_listener_config_entry base;
    size_t layer_overrides_len;
    struct input_listener_layer_override layer_overrides[];
};

struct input_listener_data {
    union {
        struct {
            struct input_listener_xy_data data;
            struct input_listener_xy_data wheel_data;

            uint8_t button_set;
            uint8_t button_clear;
        } mouse;
    };

#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
    int16_t wheel_remainder;
    int16_t h_wheel_remainder;
#endif // IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)

    struct input_listener_processor_data base_processor_data;
    struct input_listener_processor_data layer_override_data[];
};

static void handle_rel_code(struct input_listener_data *data, struct input_event *evt) {
    switch (evt->code) {
    case INPUT_REL_X:
        data->mouse.data.mode = INPUT_LISTENER_XY_DATA_MODE_REL;
        data->mouse.data.x.value += evt->value;
        break;
    case INPUT_REL_Y:
        data->mouse.data.mode = INPUT_LISTENER_XY_DATA_MODE_REL;
        data->mouse.data.y.value += evt->value;
        break;
    case INPUT_REL_WHEEL:
        data->mouse.wheel_data.mode = INPUT_LISTENER_XY_DATA_MODE_REL;
        data->mouse.wheel_data.y.value += evt->value;
        break;
    case INPUT_REL_HWHEEL:
        data->mouse.wheel_data.mode = INPUT_LISTENER_XY_DATA_MODE_REL;
        data->mouse.wheel_data.x.value += evt->value;
        break;
    default:
        break;
    }
}

static void handle_abs_code(const struct input_listener_config *config,
                            struct input_listener_data *data, struct input_event *evt) {}

static void handle_key_code(const struct input_listener_config *config,
                            struct input_listener_data *data, struct input_event *evt) {
    int8_t btn;

    switch (evt->code) {
    case INPUT_BTN_0:
    case INPUT_BTN_1:
    case INPUT_BTN_2:
    case INPUT_BTN_3:
    case INPUT_BTN_4:
        btn = evt->code - INPUT_BTN_0;
        if (evt->value > 0) {
            WRITE_BIT(data->mouse.button_set, btn, 1);
        } else {
            WRITE_BIT(data->mouse.button_clear, btn, 1);
        }
        break;
    default:
        break;
    }
}

static inline bool is_x_data(const struct input_event *evt) {
    return evt->type == INPUT_EV_REL && evt->code == INPUT_REL_X;
}

static inline bool is_y_data(const struct input_event *evt) {
    return evt->type == INPUT_EV_REL && evt->code == INPUT_REL_Y;
}

static int apply_config(uint8_t listener_index, const struct input_listener_config_entry *cfg,
                        struct input_listener_processor_data *processor_data,
                        struct input_listener_data *data, struct input_event *evt) {
    size_t remainder_index = 0;
    for (size_t p = 0; p < cfg->processors_len; p++) {
        const struct zmk_input_processor_entry *proc_e = &cfg->processors[p];
        struct input_processor_remainder_data *remainders = NULL;
        if (proc_e->track_remainders) {
            remainders = &processor_data->remainders[remainder_index++];
        }

        int16_t *remainder = NULL;
        if (remainders) {
            if (evt->type == INPUT_EV_REL) {
                switch (evt->code) {
                case INPUT_REL_X:
                    remainder = &remainders->x;
                    break;
                case INPUT_REL_Y:
                    remainder = &remainders->y;
                    break;
                case INPUT_REL_WHEEL:
                    remainder = &remainders->wheel;
                    break;
                case INPUT_REL_HWHEEL:
                    remainder = &remainders->h_wheel;
                    break;
                }
            }
        }

        LOG_DBG("LISTENER INDEX: %d", listener_index);
        struct zmk_input_processor_state state = {.input_device_index = listener_index,
                                                  .remainder = remainder};

        int ret = zmk_input_processor_handle_event(proc_e->dev, evt, proc_e->param1, proc_e->param2,
                                                   &state);
        switch (ret) {
        case ZMK_INPUT_PROC_CONTINUE:
            continue;
        default:
            return ret;
        }
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static int filter_with_input_config(const struct input_listener_config *cfg,
                                    struct input_listener_data *data, struct input_event *evt) {
    if (!evt->dev) {
        return -ENODEV;
    }

    for (size_t oi = 0; oi < cfg->layer_overrides_len; oi++) {
        const struct input_listener_layer_override *override = &cfg->layer_overrides[oi];
        struct input_listener_processor_data *override_data = &data->layer_override_data[oi];
        uint32_t mask = override->layer_mask;
        uint8_t layer = 0;
        while (mask != 0) {
            if (mask & BIT(0) && zmk_keymap_layer_active(layer)) {
                int ret =
                    apply_config(cfg->listener_index, &override->config, override_data, data, evt);

                if (ret < 0) {
                    return ret;
                }
                if (!override->process_next) {
                    return 0;
                }
            }

            layer++;
            mask = mask >> 1;
        }
    }

    return apply_config(cfg->listener_index, &cfg->base, &data->base_processor_data, data, evt);
}

static void clear_xy_data(struct input_listener_xy_data *data) {
    data->x.value = data->y.value = 0;
    data->mode = INPUT_LISTENER_XY_DATA_MODE_NONE;
}

#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
static void apply_resolution_scaling(struct input_listener_data *data, struct input_event *evt) {
    int16_t *remainder;
    uint8_t div;

    switch (evt->code) {
    case INPUT_REL_WHEEL:
        remainder = &data->wheel_remainder;
        div = (16 - zmk_pointing_resolution_multipliers_get_current_profile().wheel);
        break;
    case INPUT_REL_HWHEEL:
        remainder = &data->h_wheel_remainder;
        div = (16 - zmk_pointing_resolution_multipliers_get_current_profile().hor_wheel);
        break;
    default:
        return;
    }

    int16_t val = evt->value + *remainder;
    int16_t scaled = val / (int16_t)div;
    *remainder = val - (scaled * (int16_t)div);
    evt->value = val;
}
#endif // IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)

static void input_handler(const struct input_listener_config *config,
                          struct input_listener_data *data, struct input_event *evt) {
    // First, process to update the event data as needed.
    int ret = filter_with_input_config(config, data, evt);

    if (ret < 0) {
        LOG_ERR("Error applying input processors: %d", ret);
        return;
    } else if (ret == ZMK_INPUT_PROC_STOP) {
        return;
    }

#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
    apply_resolution_scaling(data, evt);
#endif // IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)

    switch (evt->type) {
    case INPUT_EV_REL:
        handle_rel_code(data, evt);
        break;
    case INPUT_EV_ABS:
        handle_abs_code(config, data, evt);
        break;
    case INPUT_EV_KEY:
        handle_key_code(config, data, evt);
        break;
    }

    if (evt->sync) {
        if (data->mouse.wheel_data.mode == INPUT_LISTENER_XY_DATA_MODE_REL) {
            zmk_hid_mouse_scroll_set(data->mouse.wheel_data.x.value,
                                     data->mouse.wheel_data.y.value);
        }

        if (data->mouse.data.mode == INPUT_LISTENER_XY_DATA_MODE_REL) {
            zmk_hid_mouse_movement_set(data->mouse.data.x.value, data->mouse.data.y.value);
        }

        if (data->mouse.button_set != 0) {
            for (int i = 0; i < ZMK_HID_MOUSE_NUM_BUTTONS; i++) {
                if ((data->mouse.button_set & BIT(i)) != 0) {
                    zmk_hid_mouse_button_press(i);
                }
            }
        }

        if (data->mouse.button_clear != 0) {
            for (int i = 0; i < ZMK_HID_MOUSE_NUM_BUTTONS; i++) {
                if ((data->mouse.button_clear & BIT(i)) != 0) {
                    zmk_hid_mouse_button_release(i);
                }
            }
        }

        zmk_endpoints_send_mouse_report();
        zmk_hid_mouse_scroll_set(0, 0);
        zmk_hid_mouse_movement_set(0, 0);

        clear_xy_data(&data->mouse.data);
        clear_xy_data(&data->mouse.wheel_data);

        data->mouse.button_set = data->mouse.button_clear = 0;
    }
}

#endif // VALID_LISTENER_COUNT > 0

#define ONE_FOR_TRACKED(n, elem, idx)                                                              \
    +DT_PROP(DT_PHANDLE_BY_IDX(n, input_processors, idx), track_remainders)
#define PROCESSOR_REM_TRACKERS(n) (0 DT_FOREACH_PROP_ELEM(n, input_processors, ONE_FOR_TRACKED))

#define SCOPED_PROCESSOR(scope, n, id)                                                             \
    COND_CODE_1(DT_NODE_HAS_PROP(n, input_processors),                                             \
                (static struct input_processor_remainder_data _CONCAT(                             \
                     input_processor_remainders_##id, scope)[PROCESSOR_REM_TRACKERS(n)] = {};),    \
                ())                                                                                \
    static const struct zmk_input_processor_entry _CONCAT(                                         \
        processor_##id, scope)[DT_PROP_LEN_OR(n, input_processors, 0)] =                           \
        COND_CODE_1(DT_NODE_HAS_PROP(n, input_processors),                                         \
                    ({LISTIFY(DT_PROP_LEN(n, input_processors), ZMK_INPUT_PROCESSOR_ENTRY_AT_IDX,  \
                              (, ), n)}),                                                          \
                    ({}));

#define IL_EXTRACT_CONFIG(n, id, scope)                                                            \
    {                                                                                              \
        .processors_len = DT_PROP_LEN_OR(n, input_processors, 0),                                  \
        .processors = _CONCAT(processor_##id, scope),                                              \
    }

#define IL_EXTRACT_DATA(n, id, scope)                                                              \
    {COND_CODE_1(DT_NODE_HAS_PROP(n, input_processors),                                            \
                 (.remainders_len = PROCESSOR_REM_TRACKERS(n),                                     \
                  .remainders = _CONCAT(input_processor_remainders_##id, scope), ),                \
                 ())}

#define IL_ONE(...) +1

#define CHILD_CONFIG(node, parent) SCOPED_PROCESSOR(node, node, parent)

#define OVERRIDE_LAYER_BIT(node, prop, idx) BIT(DT_PROP_BY_IDX(node, prop, idx))

#define IL_OVERRIDE(node, parent)                                                                  \
    {                                                                                              \
        .layer_mask = DT_FOREACH_PROP_ELEM_SEP(node, layers, OVERRIDE_LAYER_BIT, (|)),             \
        .process_next = DT_PROP_OR(node, process_next, false),                                     \
        .config = IL_EXTRACT_CONFIG(node, parent, node),                                           \
    }

#define IL_OVERRIDE_DATA(node, parent) IL_EXTRACT_DATA(node, parent, node)

#define IL_INST(n)                                                                                 \
    COND_CODE_1(                                                                                   \
        DT_NODE_HAS_STATUS(DT_INST_PHANDLE(n, device), okay),                                      \
        (SCOPED_PROCESSOR(base, DT_DRV_INST(n), n);                                                \
         DT_INST_FOREACH_CHILD_VARGS(n, CHILD_CONFIG,                                              \
                                     n) static const struct input_listener_config config_##n =     \
             {                                                                                     \
                 .listener_index = n,                                                              \
                 .base = IL_EXTRACT_CONFIG(DT_DRV_INST(n), n, base),                               \
                 .layer_overrides_len = (0 DT_INST_FOREACH_CHILD(n, IL_ONE)),                      \
                 .layer_overrides = {DT_INST_FOREACH_CHILD_SEP_VARGS(n, IL_OVERRIDE, (, ), n)},    \
             };                                                                                    \
         static struct input_listener_data data_##n =                                              \
             {                                                                                     \
                 .base_processor_data = IL_EXTRACT_DATA(DT_DRV_INST(n), n, base),                  \
                 .layer_override_data = {DT_INST_FOREACH_CHILD_SEP_VARGS(n, IL_OVERRIDE_DATA,      \
                                                                         (, ), n)},                \
             };                                                                                    \
         void input_handler_##n(struct input_event *evt) {                                         \
             input_handler(&config_##n, &data_##n, evt);                                           \
         } INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_INST_PHANDLE(n, device)), input_handler_##n);),  \
        ())

DT_INST_FOREACH_STATUS_OKAY(IL_INST)
