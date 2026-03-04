/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#define LED_MAP_TOG_CMD 0
#define LED_MAP_ON_CMD 1
#define LED_MAP_OFF_CMD 2
#define LED_MAP_EFF_CMD 3   /* Next effect */
#define LED_MAP_EFR_CMD 4   /* Previous effect */
#define LED_MAP_EFS_CMD 5   /* Select effect (param2 = effect number) */

#define LED_MAP_TOG LED_MAP_TOG_CMD
#define LED_MAP_ON LED_MAP_ON_CMD
#define LED_MAP_OFF LED_MAP_OFF_CMD
#define LED_MAP_EFF LED_MAP_EFF_CMD
#define LED_MAP_EFR LED_MAP_EFR_CMD
#define LED_MAP_EFS LED_MAP_EFS_CMD

/* Effect values for LED_MAP_EFS */
#define LED_MAP_EFFECT_OFF 0
#define LED_MAP_EFFECT_SOLID 1
#define LED_MAP_EFFECT_REACTIVE 2
#define LED_MAP_EFFECT_REACTIVE_FADE 3
#define LED_MAP_EFFECT_BREATHE 4
#define LED_MAP_EFFECT_SPECTRUM 5
#define LED_MAP_EFFECT_SWIRL 6
