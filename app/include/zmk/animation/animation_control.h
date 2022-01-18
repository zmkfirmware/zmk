/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr.h>
#include <device.h>

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

int animation_control_handle_command(const struct device *dev, uint8_t command, uint8_t param);