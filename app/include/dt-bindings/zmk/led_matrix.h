/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define LM_ON_CMD 0
#define LM_OFF_CMD 1
#define LM_TOG_CMD 2
#define LM_BRI_CMD 3
#define LM_BRD_CMD 4
#define LM_EFF_CMD 5
#define LM_EFR_CMD 6
#define LM_EFS_CMD 7
#define LM_BRT_CMD 8
#define LM_STA_CMD 9
#define LM_SPI_CMD 10
#define LM_SPD_CMD 11
#define LM_SPS_CMD 12
#define LM_HUI_CMD 13
#define LM_HUD_CMD 14
#define LM_HUS_CMD 15
#define LM_CFF_CMD 16
#define LM_CFR_CMD 17
#define LM_CFS_CMD 18

#define LM_CHILD_PROFILE_PACK(child_id, profile_id)                                              \
    ((((child_id) & 0xFF) << 8) | ((profile_id) & 0xFF))
#define LM_CHILD_PROFILE_CHILD_ID(encoded) (((encoded) >> 8) & 0xFF)
#define LM_CHILD_PROFILE_PROFILE_ID(encoded) ((encoded) & 0xFF)

#define LM_ON LM_ON_CMD 0
#define LM_OFF LM_OFF_CMD 0
#define LM_TOG LM_TOG_CMD 0
#define LM_BRI LM_BRI_CMD 0
#define LM_BRD LM_BRD_CMD 0
#define LM_EFF LM_EFF_CMD 0
#define LM_EFR LM_EFR_CMD 0
#define LM_EFS LM_EFS_CMD
#define LM_BRT LM_BRT_CMD
#define LM_STA LM_STA_CMD
#define LM_SPI LM_SPI_CMD 0
#define LM_SPD LM_SPD_CMD 0
#define LM_SPS LM_SPS_CMD
#define LM_HUI LM_HUI_CMD 0
#define LM_HUD LM_HUD_CMD 0
#define LM_HUS LM_HUS_CMD
#define LM_CFF LM_CFF_CMD
#define LM_CFR LM_CFR_CMD
#define LM_CFS(child_id, profile_id)                                                             \
    LM_CFS_CMD LM_CHILD_PROFILE_PACK(child_id, profile_id)

#define LM_STA_BATTERY 0x01
#define LM_STA_USB 0x02
#define LM_STA_BLE 0x04
#define LM_STA_HID 0x08
#define LM_STA_ALL (LM_STA_BATTERY | LM_STA_USB | LM_STA_BLE | LM_STA_HID)

#define LM_PH 0xFFFD
#define LM_BREAK 0xFFFE
#define LM_NOLED 0xFFFF
