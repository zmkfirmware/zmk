/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

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

#define MOVE_UP (0x0000FFFF)

#define MOVE_DOWN (0x00000001)

#define MOVE_LEFT (0xFFFF0000)

#define MOVE_RIGHT (0x00010000)

#define WHEEL_UP (0x0001)

#define WHEEL_DOWN (0x00FF)

#define WHEEL_LEFT (0xFF00)

#define WHEEL_RIGHT (0x0100)
