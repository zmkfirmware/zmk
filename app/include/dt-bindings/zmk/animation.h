/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * Maps HSL color settings into a single uint32_t value
 * that can be cast to zmk_color_hsl.
 */
#ifdef CONFIG_BIG_ENDIAN
#define HSL(h, s, l) ((h << 16) + (s << 8) + l)
#else
#define HSL(h, s, l) (h + (s << 16) + (l << 24))
#endif

/**
 * Animation blending modes
 */
#define BLENDING_MODE_NORMAL 0
#define BLENDING_MODE_MULTIPLY 1
#define BLENDING_MODE_LIGHTEN 2
#define BLENDING_MODE_DARKEN 3
#define BLENDING_MODE_SCREEN 4
#define BLENDING_MODE_SUBTRACT 5

/**
 * Animation control commands
 */
#define ANIMATION_CMD_TOGGLE 0
#define ANIMATION_CMD_NEXT 1
#define ANIMATION_CMD_PREVIOUS 2
#define ANIMATION_CMD_SELECT 3
#define ANIMATION_CMD_BRIGHTEN 4
#define ANIMATION_CMD_DIM 5
#define ANIMATION_CMD_NEXT_CONTROL_ZONE 6
#define ANIMATION_CMD_PREVIOUS_CONTROL_ZONE 7

/**
 * Generic animation command macro
 */
#define ANIMATION_CONTROL_CMD(command, zone, param) ((zone << 24) | (command << 16) | (param))

/**
 * Animation behavior helpers
 */
#define ANIMATION_TOGGLE(zone) ANIMATION_CONTROL_CMD(ANIMATION_CMD_TOGGLE, zone, 0)
#define ANIMATION_NEXT(zone) ANIMATION_CONTROL_CMD(ANIMATION_CMD_NEXT, zone, 0)
#define ANIMATION_PREVIOUS(zone) ANIMATION_CONTROL_CMD(ANIMATION_CMD_PREVIOUS, zone, 0)
#define ANIMATION_SELECT(zone, target_animation)                                                   \
    ANIMATION_CONTROL_CMD(ANIMATION_CMD_SELECT, zone, target_animation)
#define ANIMATION_BRIGHTEN(zone) ANIMATION_CONTROL_CMD(ANIMATION_CMD_BRIGHTEN, zone, 0)
#define ANIMATION_DIM(zone) ANIMATION_CONTROL_CMD(ANIMATION_CMD_DIM, zone, 0)
#define ANIMATION_NEXT_CONTROL_ZONE ANIMATION_CONTROL_CMD(ANIMATION_CMD_NEXT_CONTROL_ZONE, 0, 0)
#define ANIMATION_PREVIOUS_CONTROL_ZONE                                                            \
    ANIMATION_CONTROL_CMD(ANIMATION_CMD_PREVIOUS_CONTROL_ZONE, 0, 0)
