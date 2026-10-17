/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#define INPUT_SPEED_DOWN_CMD 0
#define INPUT_SPEED_UP_CMD 1
#define INPUT_SPEED_SET_CMD 2

#define INPUT_SPEED_DOWN INPUT_SPEED_DOWN_CMD 0
#define INPUT_SPEED_UP INPUT_SPEED_UP_CMD 0
#define INPUT_SPEED_SET(level) INPUT_SPEED_SET_CMD level
