/*
 * Copyright (c) 2020 The ZMK Contributors
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
