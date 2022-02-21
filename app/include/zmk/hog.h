/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/keys.h>
#include <zmk/hid.h>

int zmk_hog_init();

int zmk_hog_send_keyboard_report();
int zmk_hog_send_consumer_report();
