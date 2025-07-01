/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <zephyr/dt-bindings/dt-util.h>

/* Mouse press behavior */
/* Left click */
#define MB1 BIT(0)
#define LCLK (MB1)

/* Right click */
#define MB2 BIT(1)
#define RCLK (MB2)

/* Middle click */
#define MB3 BIT(2)
#define MCLK (MB3)

#define MB4 BIT(3)
#define MB5 BIT(4)

#ifndef ZMK_POINTING_DEFAULT_MOVE_VAL
#define ZMK_POINTING_DEFAULT_MOVE_VAL 600
#endif

#ifndef ZMK_POINTING_DEFAULT_SCRL_VAL
#define ZMK_POINTING_DEFAULT_SCRL_VAL 10
#endif

/* Mouse move behavior */
#define MOVE_Y(vert) ((vert) & 0xFFFF)
#define MOVE_Y_DECODE(encoded) (int16_t)((encoded) & 0x0000FFFF)
#define MOVE_X(hor) (((hor) & 0xFFFF) << 16)
#define MOVE_X_DECODE(encoded) (int16_t)(((encoded) & 0xFFFF0000) >> 16)

#define MOVE(hor, vert) (MOVE_X(hor) + MOVE_Y(vert))

#define MOVE_UP MOVE_Y(-ZMK_POINTING_DEFAULT_MOVE_VAL)
#define MOVE_DOWN MOVE_Y(ZMK_POINTING_DEFAULT_MOVE_VAL)
#define MOVE_LEFT MOVE_X(-ZMK_POINTING_DEFAULT_MOVE_VAL)
#define MOVE_RIGHT MOVE_X(ZMK_POINTING_DEFAULT_MOVE_VAL)

#define SCRL_UP MOVE_Y(ZMK_POINTING_DEFAULT_SCRL_VAL)
#define SCRL_DOWN MOVE_Y(-ZMK_POINTING_DEFAULT_SCRL_VAL)
#define SCRL_LEFT MOVE_X(-ZMK_POINTING_DEFAULT_SCRL_VAL)
#define SCRL_RIGHT MOVE_X(ZMK_POINTING_DEFAULT_SCRL_VAL)
