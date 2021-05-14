/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

/* Mouse press behavior */
/* Left click */
#define MB1 (0x01)
#define LCLK (MB1)

/* Right click */
#define MB2 (0x02)
#define RCLK (MB2)

/* Middle click */
#define MB3 (0x04)
#define MCLK (MB3)

#define MB4 (0x08)

#define MB5 (0x10)

#define MB6 (0x20)

#define MB7 (0x40)

#define MB8 (0x80)

/* Mouse move behavior */
#define MOVE_VERT(vert) ((vert)&0xFFFF)
#define MOVE_VERT_DECODE(encoded) (int16_t)((encoded)&0x0000FFFF)
#define MOVE_HOR(hor) (((hor)&0xFFFF) << 16)
#define MOVE_HOR_DECODE(encoded) (int16_t)(((encoded)&0xFFFF0000) >> 16)

#define MOVE(hor, vert) (MOVE_HOR(hor) + MOVE_VERT(vert))

#define MOVE_UP MOVE_VERT(-600)
#define MOVE_DOWN MOVE_VERT(600)
#define MOVE_LEFT MOVE_HOR(-600)
#define MOVE_RIGHT MOVE_HOR(600)

/* Mouse scroll behavior */
#define SCROLL_VERT(vert) ((vert)&0xFFFF)
#define SCROLL_VERT_DECODE(encoded) (int16_t)((encoded)&0x0000FFFF)
#define SCROLL_HOR(hor) (((hor)&0xFFFF) << 16)
#define SCROLL_HOR_DECODE(encoded) (int16_t)(((encoded)&0xFFFF0000) >> 16)

#define SCROLL(hor, vert) (SCROLL_HOR(hor) + SCROLL_VERT(vert))

#define SCROLL_UP SCROLL_VERT(10)
#define SCROLL_DOWN SCROLL_VERT(-10)
#define SCROLL_LEFT SCROLL_HOR(-10)
#define SCROLL_RIGHT SCROLL_HOR(10)
