/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>

/**
 * @brief Set a speed multiplier on a behavior-input-two-axis instance.
 *
 * The multiplier is expressed as a fraction (numerator / denominator) and is
 * applied to the computed movement each tick. Setting numerator=1, denominator=1
 * restores normal speed.
 *
 * @param dev The behavior-input-two-axis device.
 * @param numerator Multiplier numerator.
 * @param denominator Multiplier denominator (must not be 0).
 *
 * @retval 0 on success.
 * @retval -EINVAL if denominator is 0.
 */
int behavior_input_two_axis_set_speed_multiplier(const struct device *dev, int16_t numerator,
                                                 int16_t denominator);
