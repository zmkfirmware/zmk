/* SPDX-License-Identifier: MIT */
#pragma once

/* Geometry is normalized to integer units. A full color turn is 360 degrees. */
struct effect_sample {
    int hue;
    int level; /* 0..100 */
};

static bool effect_cycles_hue(unsigned renderer, unsigned style) {
    switch (renderer) {
    case LED_MATRIX_RENDERER_GRADIENT:
    case LED_MATRIX_RENDERER_RADIAL:
    case LED_MATRIX_RENDERER_HEATMAP:
        return true;
    case LED_MATRIX_RENDERER_WAVE:
        return style != 4;
    case LED_MATRIX_RENDERER_ROTARY:
        return style != 2 && style != 3;
    case LED_MATRIX_RENDERER_REACTIVE:
        return style == 5;
    case LED_MATRIX_RENDERER_PARTICLES:
        return style != 0;
    default:
        return false;
    }
}

static int effect_wrap(int value, int modulus) {
    int remainder = value % modulus;
    return remainder < 0 ? remainder + modulus : remainder;
}

static int effect_abs(int value) { return value < 0 ? -value : value; }

static int effect_triangle(int phase) {
    return 100 - effect_abs(effect_wrap(phase, 360) - 180) * 100 / 180;
}

/* Octant-linear angle avoids floating point and has continuous quadrant boundaries. */
static int effect_angle(int x, int y) {
    int ax = effect_abs(x), ay = effect_abs(y);
    if (!ax && !ay)
        return 0;
    int angle = ax >= ay ? 45 * ay / ax : 90 - 45 * ax / ay;
    if (x < 0)
        angle = 180 - angle;
    return y < 0 ? -angle : angle;
}

static uint32_t effect_hash(uint32_t seed) {
    seed ^= seed >> 16;
    seed *= 0x7feb352dU;
    seed ^= seed >> 15;
    seed *= 0x846ca68bU;
    return seed ^ (seed >> 16);
}

static int effect_phase(uint32_t now, uint32_t period) {
    return (uint64_t)(now % period) * 360U / period;
}

static struct effect_sample effect_geometry(unsigned renderer, unsigned style, bool reverse,
                                            int row, int col, int rows, int cols, uint32_t now,
                                            uint32_t period, int rank, int count) {
    int x = cols > 1 ? col * 256 / (cols - 1) : 128;
    int y = rows > 1 ? row * 256 / (rows - 1) : 128;
    int dx = x - 128, dy = y - 128;
    int phase = effect_phase(now, period);
    if (reverse)
        phase = -phase;
    struct effect_sample s = {.level = 100};
    switch (renderer) {
    case LED_MATRIX_RENDERER_GRADIENT: {
        int axis = style == 1 || style == 3 ? y : x;
        if (reverse)
            axis = 256 - axis;
        if (style >= 2)
            axis = (axis * 3 / 256) * 85;
        s.hue = axis;
        s.level = 35 + axis * 65 / 256;
        break;
    }
    case LED_MATRIX_RENDERER_WAVE: {
        int axis = style == 1 ? y : x;
        if (style == 2)
            axis += effect_abs(dy);
        if (style == 3)
            axis += effect_triangle(y + phase) - 2 * effect_triangle(phase);
        s.hue = axis - phase;
        if (style == 4) {
            s.level = effect_triangle(s.hue);
            s.hue = 0;
        }
        break;
    }
    case LED_MATRIX_RENDERER_RADIAL: {
        int distance = effect_abs(dx) + effect_abs(dy);
        if (style == 1)
            distance = effect_abs(effect_abs(dx) - 64) + effect_abs(dy);
        if (style == 2)
            distance = effect_abs(dx) + (reverse ? y : 256 - y);
        s.hue = 2 * distance - (style == 2 ? effect_abs(phase) : phase);
        s.level = 25 + effect_triangle(s.hue) * 3 / 4;
        break;
    }
    case LED_MATRIX_RENDERER_ROTARY: {
        int angle = effect_angle(dx, dy);
        int band = 2 * angle - phase;
        if (style == 1 || style == 2)
            band += effect_abs(dx) + effect_abs(dy);
        s.hue = style == 2 || style == 3 ? 0 : band;
        if (style >= 2)
            s.level = effect_triangle(band);
        break;
    }
    case LED_MATRIX_RENDERER_PARTICLES: {
        /* Adjacent cells share the same bloom seed, with a delayed perimeter. */
        uint32_t seed = style == 2 ? (uint32_t)(row / 3 * cols + col / 3) : (uint32_t)rank;
        uint32_t offset = effect_hash(seed + 1) % period;
        uint32_t tick = now + offset;
        uint32_t cycle = tick / period;
        uint32_t variation = effect_hash(seed ^ (cycle * 2654435761U));
        int pulse = effect_phase(tick, period) - (int)(variation % (style == 2 ? 160U : 240U));
        if (style == 2)
            pulse -= 32 * (effect_abs(col % 3 - 1) + effect_abs(row % 3 - 1));
        s.level = pulse >= 0 && pulse < 120 ? effect_triangle(pulse * 3) : 0;
        if (style != 0)
            s.hue = variation % 360;
        break;
    }
    case LED_MATRIX_RENDERER_CHASE: {
        int head = (uint64_t)(now % period) * count / period;
        if (reverse)
            rank = count - 1 - rank;
        int tail = effect_wrap(head - rank, count);
        s.level = tail < 4 ? 100 >> tail : 0;
        break;
    }
    }
    s.hue = effect_wrap(s.hue, 360);
    return s;
}

static int effect_reaction(unsigned style, int dr, int dc, int span, unsigned progress) {
    int distance = dr + dc;
    int fade = 100 - (int)progress;
    if (style == 0)
        return distance < 3 ? fade / (1 + distance) : 0;
    if (style == 1 && dr && dc)
        return 0;
    if (style == 2)
        distance = dr > dc ? dr : dc;
    if (style == 3)
        return dc == 0 ? fade : 0;
    int delta = effect_abs(distance - (int)(progress * (span + 2) / 100));
    return delta == 0 ? fade : delta == 1 ? fade / 2 : 0;
}
