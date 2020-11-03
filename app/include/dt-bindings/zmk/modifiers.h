/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#define MOD_LCTL 0x01
#define MOD_LSFT 0x02
#define MOD_LALT 0x04
#define MOD_LGUI 0x08
#define MOD_RCTL 0x10
#define MOD_RSFT 0x20
#define MOD_RALT 0x40
#define MOD_RGUI 0x80

#define SELECT_MODS(keycode) (keycode >> 24)
#define STRIP_MODS(keycode) (keycode & ~(0xFF << 24))
#define APPLY_MODS(mods, keycode) (mods << 24 | keycode)

#define LC(keycode) APPLY_MODS(MOD_LCTL, keycode)
#define LS(keycode) APPLY_MODS(MOD_LSFT, keycode)
#define LA(keycode) APPLY_MODS(MOD_LALT, keycode)
#define LG(keycode) APPLY_MODS(MOD_LGUI, keycode)
#define RC(keycode) APPLY_MODS(MOD_RCTL, keycode)
#define RS(keycode) APPLY_MODS(MOD_RSFT, keycode)
#define RA(keycode) APPLY_MODS(MOD_RALT, keycode)
#define RG(keycode) APPLY_MODS(MOD_RGUI, keycode)