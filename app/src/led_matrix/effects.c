/* SPDX-License-Identifier: MIT */
#include "effect_shapes.h"

#define EFFECT_HISTORY 8
struct effect_hit {
    uint32_t time;
    uint16_t cell;
    bool active;
};
static struct effect_hit effect_hits[EFFECT_HISTORY];
static uint8_t next_hit;
static uint8_t heat_level[LOGICAL_LED_COUNT];
static uint32_t heat_time[LOGICAL_LED_COUNT];

static void effect_reset_input(void) {
    unsigned int key = irq_lock();
    memset(effect_hits, 0, sizeof(effect_hits));
    memset(heat_level, 0, sizeof(heat_level));
    next_hit = 0;
    irq_unlock(key);
}

static bool effect_requires_grid(uint8_t renderer) {
    return renderer == LED_MATRIX_RENDERER_KEYPRESS || renderer == LED_MATRIX_RENDERER_RIPPLE ||
           renderer == LED_MATRIX_RENDERER_RAIN ||
           (renderer >= LED_MATRIX_RENDERER_GRADIENT && renderer != LED_MATRIX_RENDERER_CHASE);
}

static bool effect_options_valid(uint8_t renderer, unsigned style, unsigned direction) {
    static const uint8_t styles[] = {4, 5, 3, 5, 3, 1, 7, 1};
    _Static_assert(ARRAY_SIZE(styles) == LED_MATRIX_RENDERER_COUNT - LED_MATRIX_RENDERER_GRADIENT,
                   "Each extended renderer needs a style count");
    if (renderer >= LED_MATRIX_RENDERER_COUNT)
        return false;
    if (renderer < LED_MATRIX_RENDERER_GRADIENT)
        return !style && !direction;
    bool reversible =
        renderer <= LED_MATRIX_RENDERER_ROTARY || renderer == LED_MATRIX_RENDERER_CHASE;
    return style < styles[renderer - LED_MATRIX_RENDERER_GRADIENT] && direction <= reversible;
}

/* A key event can preempt a render after its frame timestamp was sampled. */
static uint32_t effect_age(uint32_t now, uint32_t start) {
    return start - now < 1000U ? 0U : now - start;
}

static uint8_t effect_heat(uint16_t led, uint32_t now, uint32_t period) {
    uint32_t elapsed = effect_age(now, heat_time[led]);
    if (elapsed >= period)
        return 0;
    int loss = elapsed * 100U / period;
    return MAX(0, (int)heat_level[led] - loss);
}

static int effect_member(const struct led_matrix_profile_cfg *p, uint16_t led) {
#ifdef CONFIG_ZMK_LED_MATRIX_EDITOR
    if (p->member_index)
        return p->member_index[led];
#else
    ARG_UNUSED(p);
#endif
    return led;
}

static bool effect_press(const struct led_matrix_profile_cfg *p, uint32_t position, uint32_t now) {
    if (position >= ZMK_KEYMAP_LEN)
        return false;
    uint16_t origin = position_to_cell[position];
    if (origin >= grid_cell_count || cell_to_led[origin] >= LOGICAL_LED_COUNT ||
        effect_member(p, cell_to_led[origin]) == LM_NOLED)
        return false;
    if (p->renderer == LED_MATRIX_RENDERER_REACTIVE) {
        unsigned int key = irq_lock();
        effect_hits[next_hit] = (struct effect_hit){now, origin, true};
        next_hit = (next_hit + 1) % EFFECT_HISTORY;
        irq_unlock(key);
    } else {
        uint32_t period = led_matrix_effective_profile_period_ms(p);
        for (uint16_t cell = 0; cell < grid_cell_count; cell++) {
            uint16_t led = cell_to_led[cell];
            if (led >= LOGICAL_LED_COUNT)
                continue;
            int distance = effect_abs(cell_row(cell) - cell_row(origin)) +
                           effect_abs(cell_col(cell) - cell_col(origin));
            if (distance <= 1) {
                unsigned int key = irq_lock();
                heat_level[led] = MIN(100, effect_heat(led, now, period) + (distance ? 12 : 40));
                heat_time[led] = now;
                irq_unlock(key);
            }
        }
    }
    return true;
}

static struct effect_sample reactive_sample(const struct effect_hit *hits, uint16_t cell,
                                            unsigned style, uint32_t now, uint32_t period,
                                            bool *moving) {
    struct effect_sample s = {.level = style == 6 ? 18 : 0};
    for (size_t i = 0; i < ARRAY_SIZE(effect_hits); i++) {
        const struct effect_hit *hit = &hits[i];
        uint32_t elapsed = effect_age(now, hit->time);
        if (!hit->active || elapsed >= period)
            continue;
        int dr = effect_abs(cell_row(cell) - cell_row(hit->cell));
        int dc = effect_abs(cell_col(cell) - cell_col(hit->cell));
        if ((style == 0 && dr + dc >= 3) || (style == 1 && dr && dc) || (style == 3 && dc))
            continue;
        unsigned progress = elapsed * 100U / period;
        int level = effect_reaction(style, dr, dc, grid_row_count + grid_col_count - 2, progress);
        *moving = true;
        if (level > s.level) {
            s.level = level;
            if (style == 5)
                s.hue = (dr + dc) * 45 + progress * 2;
        }
    }
    return s;
}

static void render_extended_effect(const struct led_matrix_profile_cfg *p, uint32_t now) {
    uint32_t period = led_matrix_effective_profile_period_ms(p);
    int count = LOGICAL_LED_COUNT;
    struct effect_hit hits[EFFECT_HISTORY];
    if (p->renderer == LED_MATRIX_RENDERER_REACTIVE) {
        unsigned int key = irq_lock();
        for (size_t i = 0; i < ARRAY_SIZE(effect_hits); i++) {
            if (effect_age(now, effect_hits[i].time) >= period)
                effect_hits[i].active = false;
        }
        memcpy(hits, effect_hits, sizeof(hits));
        irq_unlock(key);
    }
#ifdef CONFIG_ZMK_LED_MATRIX_EDITOR
    if (p->renderer == LED_MATRIX_RENDERER_CHASE && p->member_index) {
        count = 0;
        for (size_t led = 0; led < LOGICAL_LED_COUNT; led++)
            count += effect_member(p, led) != LM_NOLED;
    }
#endif
    for (uint16_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        uint16_t cell = led_to_cell[led];
        int rank = effect_member(p, led);
        if (rank == LM_NOLED ||
            (cell >= grid_cell_count && p->renderer != LED_MATRIX_RENDERER_CHASE))
            continue;
        struct effect_sample s;
        bool moving = false;
        if (p->renderer == LED_MATRIX_RENDERER_REACTIVE) {
            s = reactive_sample(hits, cell, p->effect_style, now, period, &moving);
        } else if (p->renderer == LED_MATRIX_RENDERER_HEATMAP) {
            unsigned int key = irq_lock();
            s.level = effect_heat(led, now, period);
            if (!s.level)
                heat_level[led] = 0;
            irq_unlock(key);
            s.hue = (100 - s.level) * 2;
            moving = s.level > 0;
        } else {
            s = effect_geometry(p->renderer, p->effect_style, p->effect_direction,
                                cell < grid_cell_count ? cell_row(cell) : 0,
                                cell < grid_cell_count ? cell_col(cell) : 0, grid_row_count,
                                grid_col_count, now, period, rank, count);
            moving = p->renderer != LED_MATRIX_RENDERER_GRADIENT &&
                     !(p->renderer == LED_MATRIX_RENDERER_CHASE && count == 1);
        }
        uint32_t configured = main_profile_color(p, led);
        logical_leds_dynamic[led] =
            (s.level || moving) && led_matrix_color_needs_time(configured, p->brightness, moving);
        struct led_rgb8 color = resolve_profile_color(p, led, now);
        /* Use the value of a neutral base as the peak of a color-cycle effect. */
        if (effect_cycles_hue(p->renderer, p->effect_style) && color.r == color.g &&
            color.g == color.b) {
            color.g = color.b = 0;
        }
        if (s.hue)
            color = shift_color_hue(color, s.hue % 360);
        logical_leds[led] = scale_color(scale_color(color, p->brightness), s.level);
    }
}
