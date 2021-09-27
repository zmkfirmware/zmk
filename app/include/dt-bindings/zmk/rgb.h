/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define RGB_TOG_CMD 0
#define RGB_ON_CMD 1
#define RGB_OFF_CMD 2
#define RGB_HUI_CMD 3
#define RGB_HUD_CMD 4
#define RGB_SAI_CMD 5
#define RGB_SAD_CMD 6
#define RGB_BRI_CMD 7
#define RGB_BRD_CMD 8
#define RGB_SPI_CMD 9
#define RGB_SPD_CMD 10
#define RGB_EFF_CMD 11
#define RGB_EFR_CMD 12
#define RGB_COLOR_HSB_CMD 13

#define RGB_TOG RGB_TOG_CMD 0
#define RGB_ON RGB_ON_CMD 0
#define RGB_OFF RGB_OFF_CMD 0
#define RGB_HUI RGB_HUI_CMD 0
#define RGB_HUD RGB_HUD_CMD 0
#define RGB_SAI RGB_SAI_CMD 0
#define RGB_SAD RGB_SAD_CMD 0
#define RGB_BRI RGB_BRI_CMD 0
#define RGB_BRD RGB_BRD_CMD 0
#define RGB_SPI RGB_SPI_CMD 0
#define RGB_SPD RGB_SPD_CMD 0
#define RGB_EFF RGB_EFF_CMD 0
#define RGB_EFR RGB_EFR_CMD 0
#define RGB_COLOR_HSB_VAL(h, s, v) (((h) << 16) + ((s) << 8) + (v))
#define RGB_COLOR_HSB(h, s, v) RGB_COLOR_HSB_CMD##(RGB_COLOR_HSB_VAL(h, s, v))
#define RGB_COLOR_HSV RGB_COLOR_HSB