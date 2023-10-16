/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/mouse_tick.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/mouse.h>

#include <zephyr/sys/util.h> // CLAMP

#if !defined(CONFIG_ZMK_SPLIT) || defined(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
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

struct vector2d move_remainder = {0};
struct vector2d scroll_remainder = {0};

static int64_t ms_since_start(int64_t start, int64_t now, int64_t delay) {
    int64_t move_duration = now - (start + delay);
    // start can be in the future if there's a delay
    if (move_duration < 0) {
        move_duration = 0;
    }
    return move_duration;
}

static float speed(const struct mouse_config *config, float max_speed, int64_t duration_ms) {
    // Calculate the speed based on MouseKeysAccel
    // See https://en.wikipedia.org/wiki/Mouse_keys
    if (duration_ms > config->time_to_max_speed_ms || config->time_to_max_speed_ms == 0 ||
        config->acceleration_exponent == 0) {
        return max_speed;
    }
    float time_fraction = (float)duration_ms / config->time_to_max_speed_ms;
    return max_speed * powf(time_fraction, config->acceleration_exponent);
}

static void track_remainder(float *move, float *remainder) {
    float new_move = *move + *remainder;
    *remainder = new_move - (int)new_move;
    *move = (int)new_move;
}

static struct vector2d update_movement(struct vector2d *remainder,
                                       const struct mouse_config *config, struct vector2d max_speed,
                                       int64_t now, int64_t start_time_x, int64_t start_time_y) {
    struct vector2d move = {0};
    if (max_speed.x == 0 && max_speed.y == 0) {
        *remainder = (struct vector2d){0};
        return move;
    }

    int64_t move_duration_x = ms_since_start(start_time_x, now, config->delay_ms);
    int64_t move_duration_y = ms_since_start(start_time_y, now, config->delay_ms);
    move = (struct vector2d){
        .x = speed(config, max_speed.x, move_duration_x) * CONFIG_ZMK_MOUSE_TICK_DURATION / 1000,
        .y = speed(config, max_speed.y, move_duration_y) * CONFIG_ZMK_MOUSE_TICK_DURATION / 1000,
    };

    track_remainder(&(move.x), &(remainder->x));
    track_remainder(&(move.y), &(remainder->y));

    return move;
}

static void mouse_tick_handler(const struct zmk_mouse_tick *tick) {
    LOG_INF("tick start times: %lld %lld %lld %lld", tick->start_times.m_x, tick->start_times.m_y,
            tick->start_times.s_x, tick->start_times.s_y);
    struct vector2d move =
        update_movement(&move_remainder, &(tick->move_config), tick->max_move, tick->timestamp,
                        tick->start_times.m_x, tick->start_times.m_y);
    zmk_hid_mouse_movement_update((int16_t)CLAMP(move.x, INT16_MIN, INT16_MAX),
                                  (int16_t)CLAMP(move.y, INT16_MIN, INT16_MAX));
    struct vector2d scroll =
        update_movement(&scroll_remainder, &(tick->scroll_config), tick->max_scroll,
                        tick->timestamp, tick->start_times.s_x, tick->start_times.s_y);
    zmk_hid_mouse_scroll_update((int8_t)CLAMP(scroll.x, INT8_MIN, INT8_MAX),
                                (int8_t)CLAMP(scroll.y, INT8_MIN, INT8_MAX));
}

int zmk_mouse_tick_listener(const zmk_event_t *eh) {
    const struct zmk_mouse_tick *tick = as_zmk_mouse_tick(eh);
    if (tick) {
        mouse_tick_handler(tick);
        return 0;
    }
    return 0;
}

ZMK_LISTENER(zmk_mouse_tick_listener, zmk_mouse_tick_listener);
ZMK_SUBSCRIPTION(zmk_mouse_tick_listener, zmk_mouse_tick);

#endif /* !defined(CONFIG_ZMK_SPLIT) || defined(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) */
