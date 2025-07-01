/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_input_two_axis

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h> // CLAMP

#include <zmk/behavior.h>
#include <dt-bindings/zmk/pointing.h>

#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
#include <zmk/pointing/resolution_multipliers.h>
#endif // IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct vector2d {
    float x;
    float y;
};

struct movement_state_1d {
    float remainder;
    int16_t speed;
    int64_t start_time;
};

struct movement_state_2d {
    struct movement_state_1d x;
    struct movement_state_1d y;
};

struct behavior_input_two_axis_data {
    struct k_work_delayable tick_work;
    const struct device *dev;

    struct movement_state_2d state;
};

struct behavior_input_two_axis_config {
    int16_t x_code;
    int16_t y_code;
    uint16_t delay_ms;
    uint16_t time_to_max_speed_ms;
    uint8_t trigger_period_ms;
    // acceleration exponent 0: uniform speed
    // acceleration exponent 1: uniform acceleration
    // acceleration exponent 2: uniform jerk
    uint8_t acceleration_exponent;
};

#if CONFIG_MINIMAL_LIBC
static float powf(float base, float exponent) {
    // poor man's power implementation rounds the exponent down to the nearest integer.
    float power = 1.0f;
    for (; exponent >= 1.0f; exponent--) {
        power = power * base;
    }
    return power;
}
#else
#include <math.h>
#endif

static int64_t ticks_since_start(int64_t start, int64_t now, int64_t delay) {
    if (start == 0) {
        return 0;
    }
    int64_t move_duration = now - (start + delay);
    // start can be in the future if there's a delay
    if (move_duration < 0) {
        move_duration = 0;
    }
    return move_duration;
}

#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)

static uint8_t get_acceleration_exponent(const struct behavior_input_two_axis_config *config,
                                         uint16_t code) {
    switch (code) {
    case INPUT_REL_WHEEL:
        return (zmk_pointing_resolution_multipliers_get_current_profile().wheel > 0)
                   ? 0
                   : config->acceleration_exponent;
    case INPUT_REL_HWHEEL:
        return (zmk_pointing_resolution_multipliers_get_current_profile().hor_wheel > 0)
                   ? 0
                   : config->acceleration_exponent;
    default:
        return config->acceleration_exponent;
    }
}

#else

static inline uint8_t get_acceleration_exponent(const struct behavior_input_two_axis_config *config,
                                                uint16_t code) {
    return config->acceleration_exponent;
}

#endif // IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)

static float speed(const struct behavior_input_two_axis_config *config, uint16_t code,
                   float max_speed, int64_t duration_ticks) {
    uint8_t accel_exp = get_acceleration_exponent(config, code);

    if ((1000 * duration_ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC) > config->time_to_max_speed_ms ||
        config->time_to_max_speed_ms == 0 || accel_exp == 0) {
        return max_speed;
    }

    // Calculate the speed based on MouseKeysAccel
    // See https://en.wikipedia.org/wiki/Mouse_keys
    if (duration_ticks == 0) {
        return 0;
    }

    float time_fraction = (float)(1000 * duration_ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC) /
                          config->time_to_max_speed_ms;
    return max_speed * powf(time_fraction, accel_exp);
}

static void track_remainder(float *move, float *remainder) {
    float new_move = *move + *remainder;
    *remainder = new_move - (int)new_move;
    *move = (int)new_move;
}

static float update_movement_1d(const struct behavior_input_two_axis_config *config, uint16_t code,
                                struct movement_state_1d *state, int64_t now) {
    float move = 0;
    if (state->speed == 0) {
        state->remainder = 0;
        return move;
    }

    int64_t move_duration = ticks_since_start(state->start_time, now, config->delay_ms);
    LOG_DBG("Calculated speed: %f", speed(config, code, state->speed, move_duration));
    move =
        (move_duration > 0)
            ? (speed(config, code, state->speed, move_duration) * config->trigger_period_ms / 1000)
            : 0;

    track_remainder(&(move), &(state->remainder));

    return move;
}
static struct vector2d update_movement_2d(const struct behavior_input_two_axis_config *config,
                                          struct movement_state_2d *state, int64_t now) {
    struct vector2d move = {0};

    move = (struct vector2d){
        .x = update_movement_1d(config, config->x_code, &state->x, now),
        .y = update_movement_1d(config, config->y_code, &state->y, now),
    };

    return move;
}

static bool is_non_zero_1d_movement(int16_t speed) { return speed != 0; }

static bool is_non_zero_2d_movement(struct movement_state_2d *state) {
    return is_non_zero_1d_movement(state->x.speed) || is_non_zero_1d_movement(state->y.speed);
}

static bool should_be_working(struct behavior_input_two_axis_data *data) {
    return is_non_zero_2d_movement(&data->state);
}

static void tick_work_cb(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    struct behavior_input_two_axis_data *data =
        CONTAINER_OF(d_work, struct behavior_input_two_axis_data, tick_work);
    const struct device *dev = data->dev;
    const struct behavior_input_two_axis_config *cfg = dev->config;

    uint64_t timestamp = k_uptime_ticks();

    // LOG_INF("x start: %llu, y start: %llu, current timestamp: %llu", data->state.x.start_time,
    //         data->state.y.start_time, timestamp);

    struct vector2d move = update_movement_2d(cfg, &data->state, timestamp);

    int ret = 0;
    bool have_x = is_non_zero_1d_movement(move.x);
    bool have_y = is_non_zero_1d_movement(move.y);
    if (have_x) {
        ret = input_report_rel(dev, cfg->x_code, (int16_t)CLAMP(move.x, INT16_MIN, INT16_MAX),
                               !have_y, K_NO_WAIT);
    }
    if (have_y) {
        ret = input_report_rel(dev, cfg->y_code, (int16_t)CLAMP(move.y, INT16_MIN, INT16_MAX), true,
                               K_NO_WAIT);
    }

    if (should_be_working(data)) {
        k_work_schedule(&data->tick_work, K_MSEC(cfg->trigger_period_ms));
    }
}

static void set_start_times_for_activity_1d(struct movement_state_1d *state) {
    if (state->speed != 0 && state->start_time == 0) {
        state->start_time = k_uptime_ticks();
    } else if (state->speed == 0) {
        state->start_time = 0;
    }
}
static void set_start_times_for_activity(struct movement_state_2d *state) {
    set_start_times_for_activity_1d(&state->x);
    set_start_times_for_activity_1d(&state->y);
}

static void update_work_scheduling(const struct device *dev) {
    struct behavior_input_two_axis_data *data = dev->data;
    const struct behavior_input_two_axis_config *cfg = dev->config;

    set_start_times_for_activity(&data->state);

    if (should_be_working(data)) {
        k_work_schedule(&data->tick_work, K_MSEC(cfg->trigger_period_ms));
    } else {
        k_work_cancel_delayable(&data->tick_work);
        data->state.y.remainder = 0;
        data->state.x.remainder = 0;
    }
}

int behavior_input_two_axis_adjust_speed(const struct device *dev, int16_t dx, int16_t dy) {
    struct behavior_input_two_axis_data *data = dev->data;

    LOG_DBG("Adjusting: %d %d", dx, dy);
    data->state.x.speed += dx;
    data->state.y.speed += dy;

    LOG_DBG("After: %d %d", data->state.x.speed, data->state.y.speed);

    update_work_scheduling(dev);

    return 0;
}

static int behavior_input_two_axis_init(const struct device *dev) {
    struct behavior_input_two_axis_data *data = dev->data;

    data->dev = dev;
    k_work_init_delayable(&data->tick_work, tick_work_cb);

    return 0;
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {

    const struct device *behavior_dev = zmk_behavior_get_binding(binding->behavior_dev);

    LOG_DBG("position %d keycode 0x%02X", event.position, binding->param1);

    int16_t x = MOVE_X_DECODE(binding->param1);
    int16_t y = MOVE_Y_DECODE(binding->param1);

    behavior_input_two_axis_adjust_speed(behavior_dev, x, y);
    return 0;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *behavior_dev = zmk_behavior_get_binding(binding->behavior_dev);

    LOG_DBG("position %d keycode 0x%02X", event.position, binding->param1);

    int16_t x = MOVE_X_DECODE(binding->param1);
    int16_t y = MOVE_Y_DECODE(binding->param1);

    behavior_input_two_axis_adjust_speed(behavior_dev, -x, -y);
    return 0;
}

static const struct behavior_driver_api behavior_input_two_axis_driver_api = {
    .binding_pressed = on_keymap_binding_pressed, .binding_released = on_keymap_binding_released};

#define ITA_INST(n)                                                                                \
    static struct behavior_input_two_axis_data behavior_input_two_axis_data_##n = {};              \
    static struct behavior_input_two_axis_config behavior_input_two_axis_config_##n = {            \
        .x_code = DT_INST_PROP(n, x_input_code),                                                   \
        .y_code = DT_INST_PROP(n, y_input_code),                                                   \
        .trigger_period_ms = DT_INST_PROP(n, trigger_period_ms),                                   \
        .delay_ms = DT_INST_PROP_OR(n, delay_ms, 0),                                               \
        .time_to_max_speed_ms = DT_INST_PROP(n, time_to_max_speed_ms),                             \
        .acceleration_exponent = DT_INST_PROP_OR(n, acceleration_exponent, 1),                     \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(                                                                       \
        n, behavior_input_two_axis_init, NULL, &behavior_input_two_axis_data_##n,                  \
        &behavior_input_two_axis_config_##n, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,     \
        &behavior_input_two_axis_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ITA_INST)
