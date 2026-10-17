/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/display/status_screen.h>

#include "display/status_screen.h"

lv_obj_t *zmk_display_status_screen() {
    return numpad_display_status_screen();
}
