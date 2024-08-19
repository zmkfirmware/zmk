/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/matrix_transform.h>

struct zmk_key_physical_attrs {
    int16_t width;
    int16_t height;
    int16_t x;
    int16_t y;
    int16_t rx;
    int16_t ry;
    int16_t r;
};

struct zmk_physical_layout {
    const char *display_name;

    zmk_matrix_transform_t matrix_transform;
    const struct device *kscan;

    const struct zmk_key_physical_attrs *keys;
    size_t keys_len;
};

#define ZMK_PHYS_LAYOUTS_FOREACH(_ref) STRUCT_SECTION_FOREACH(zmk_physical_layout, _ref)

size_t zmk_physical_layouts_get_list(struct zmk_physical_layout const *const **phys_layouts);

int zmk_physical_layouts_select(uint8_t index);
int zmk_physical_layouts_get_selected(void);

int zmk_physical_layouts_check_unsaved_selection(void);
int zmk_physical_layouts_save_selected(void);
int zmk_physical_layouts_revert_selected(void);

int zmk_physical_layouts_get_position_map(uint8_t source, uint8_t dest, uint32_t *map);
