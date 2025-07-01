/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/matrix_transform.h>
#include <zmk/event_manager.h>

struct zmk_physical_layout_selection_changed {
    uint8_t selection;
};

ZMK_EVENT_DECLARE(zmk_physical_layout_selection_changed);

struct zmk_key_physical_attrs {
    int16_t width;
    int16_t height;
    int16_t x;
    int16_t y;
#if IS_ENABLED(CONFIG_ZMK_PHYSICAL_LAYOUT_KEY_ROTATION)
    int16_t rx;
    int16_t ry;
    int16_t r;
#endif
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

int zmk_physical_layouts_get_position_map(uint8_t source, uint8_t dest, size_t map_size,
                                          uint32_t map[map_size]);

/**
 * @brief Get a pointer to a position map array for mapping a key position in the selected
 *        physical layout to the stock/chosen physical layout
 *
 * @retval a negative errno value in the case of errors
 * @retval a positive length of the position map array that map is updated to point to.
 */
int zmk_physical_layouts_get_selected_to_stock_position_map(uint32_t const **map);