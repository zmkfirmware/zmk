/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/bluetooth/uuid.h>

#define ZMK_BT_STUDIO_UUID(num) BT_UUID_128_ENCODE(num, 0x0196, 0x6107, 0xc967, 0xc5cfb1c2482a)
#define ZMK_STUDIO_BT_SERVICE_UUID ZMK_BT_STUDIO_UUID(0x00000000)
#define ZMK_STUDIO_BT_RPC_CHRC_UUID ZMK_BT_STUDIO_UUID(0x00000001)
