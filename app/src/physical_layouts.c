/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/kscan.h>

#if IS_ENABLED(CONFIG_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix.h>
#include <zmk/physical_layouts.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

ZMK_EVENT_IMPL(zmk_physical_layout_selection_changed);

#define DT_DRV_COMPAT zmk_physical_layout

#define USE_PHY_LAYOUTS                                                                            \
    (DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) && !DT_HAS_CHOSEN(zmk_matrix_transform))

BUILD_ASSERT(
    !IS_ENABLED(CONFIG_ZMK_STUDIO) || USE_PHY_LAYOUTS,
    "ISSUE FOUND: Keyboards require additional configuration to allow for firmware with ZMK "
    "Studio enabled. You have attempted to build a keyboard lacking such configuration. Please see "
    "https://zmk.dev/docs/features/studio#adding-zmk-studio-support-to-a-keyboard for "
    "more information on how to resolve this error, or contact the maintainer of your keyboard's "
    "firmware for assistance.");

#if USE_PHY_LAYOUTS

#define ZKPA_INIT(i, n)                                                                            \
    (const struct zmk_key_physical_attrs) {                                                        \
        .width = (int16_t)(int32_t)DT_INST_PHA_BY_IDX(n, keys, i, width),                          \
        .height = (int16_t)(int32_t)DT_INST_PHA_BY_IDX(n, keys, i, height),                        \
        .x = (int16_t)(int32_t)DT_INST_PHA_BY_IDX(n, keys, i, x),                                  \
        .y = (int16_t)(int32_t)DT_INST_PHA_BY_IDX(n, keys, i, y),                                  \
        COND_CODE_1(IS_ENABLED(CONFIG_ZMK_PHYSICAL_LAYOUT_KEY_ROTATION),                           \
                    (.rx = (int16_t)(int32_t)DT_INST_PHA_BY_IDX(n, keys, i, rx),                   \
                     .ry = (int16_t)(int32_t)DT_INST_PHA_BY_IDX(n, keys, i, ry),                   \
                     .r = (int16_t)(int32_t)DT_INST_PHA_BY_IDX(n, keys, i, r), ),                  \
                    ())                                                                            \
    }

#define ZMK_LAYOUT_INST(n)                                                                         \
    BUILD_ASSERT(!IS_ENABLED(CONFIG_ZMK_STUDIO) || DT_INST_NODE_HAS_PROP(n, keys),                 \
                 "ZMK Studio requires physical layouts with key positions. See "                   \
                 "https://zmk.dev/docs/development/hardware-integration/studio-setup");            \
    static const struct zmk_key_physical_attrs const _CONCAT(                                      \
        _zmk_physical_layout_keys_, n)[DT_INST_PROP_LEN_OR(n, keys, 0)] = {                        \
        LISTIFY(DT_INST_PROP_LEN_OR(n, keys, 0), ZKPA_INIT, (, ), n)};                             \
    ZMK_MATRIX_TRANSFORM_EXTERN(DT_INST_PHANDLE(n, transform));                                    \
    static const struct zmk_physical_layout const _CONCAT(_zmk_physical_layout_,                   \
                                                          DT_DRV_INST(n)) = {                      \
        .display_name = DT_INST_PROP_OR(n, display_name, "Layout #" #n),                           \
        .matrix_transform = ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_INST_PHANDLE(n, transform)),        \
        .keys = _CONCAT(_zmk_physical_layout_keys_, n),                                            \
        .keys_len = DT_INST_PROP_LEN_OR(n, keys, 0),                                               \
        .kscan = DEVICE_DT_GET(COND_CODE_1(DT_INST_PROP_LEN(n, kscan),                             \
                                           (DT_INST_PHANDLE(n, kscan)), (DT_CHOSEN(zmk_kscan))))};

DT_INST_FOREACH_STATUS_OKAY(ZMK_LAYOUT_INST)

#define POS_MAP_COMPAT zmk_physical_layout_position_map
#define HAVE_POS_MAP DT_HAS_COMPAT_STATUS_OKAY(POS_MAP_COMPAT)

#define POS_MAP_COMPLETE (HAVE_POS_MAP && DT_PROP(DT_INST(0, POS_MAP_COMPAT), complete))

#if HAVE_POS_MAP

// Using sizeof + union trick to calculate the "positions" length statically.
#define ZMK_POS_MAP_POSITIONS_ARRAY(node_id)                                                       \
    uint8_t _CONCAT(prop_, node_id)[DT_PROP_LEN(node_id, positions)];

#define ZMK_POS_MAP_LEN                                                                            \
    sizeof(union {DT_FOREACH_CHILD(DT_INST(0, POS_MAP_COMPAT), ZMK_POS_MAP_POSITIONS_ARRAY)})

struct position_map_entry {
    const struct zmk_physical_layout *layout;
    const uint32_t positions[ZMK_POS_MAP_LEN];
};

#define ZMK_POS_MAP_LEN_CHECK(node_id)                                                             \
    BUILD_ASSERT(ZMK_POS_MAP_LEN == DT_PROP_LEN(node_id, positions),                               \
                 "Position maps must all have the same number of entries")

DT_FOREACH_CHILD_SEP(DT_INST(0, POS_MAP_COMPAT), ZMK_POS_MAP_LEN_CHECK, (;));

#define ZMK_POS_MAP_ENTRY(node_id)                                                                 \
    {                                                                                              \
        .layout = COND_CODE_1(                                                                     \
            UTIL_AND(DT_NODE_HAS_COMPAT(DT_PHANDLE(node_id, physical_layout), DT_DRV_COMPAT),      \
                     DT_NODE_HAS_STATUS(DT_PHANDLE(node_id, physical_layout), okay)),              \
            (&_CONCAT(_zmk_physical_layout_, DT_PHANDLE(node_id, physical_layout))), (NULL)),      \
        .positions = DT_PROP(node_id, positions),                                                  \
    }

static const struct position_map_entry positions_maps[] = {
    DT_FOREACH_CHILD_SEP(DT_INST(0, POS_MAP_COMPAT), ZMK_POS_MAP_ENTRY, (, ))};

#endif

#define ZMK_LAYOUT_REF(n) &_CONCAT(_zmk_physical_layout_, DT_DRV_INST(n)),

static const struct zmk_physical_layout *const layouts[] = {
    DT_INST_FOREACH_STATUS_OKAY(ZMK_LAYOUT_REF)};

#elif DT_HAS_CHOSEN(zmk_matrix_transform)

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#warning                                                                                           \
    "Ignoring the physical layouts and using the chosen matrix transform. Consider setting a chosen physical layout instead."

#endif

ZMK_MATRIX_TRANSFORM_EXTERN(DT_CHOSEN(zmk_matrix_transform));

static const struct zmk_physical_layout _CONCAT(_zmk_physical_layout_, chosen) = {
    .display_name = "Default",
    .matrix_transform = ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_CHOSEN(zmk_matrix_transform)),
    COND_CODE_1(DT_HAS_CHOSEN(zmk_kscan), (.kscan = DEVICE_DT_GET(DT_CHOSEN(zmk_kscan)), ), ())};

static const struct zmk_physical_layout *const layouts[] = {
    &_CONCAT(_zmk_physical_layout_, chosen)};

#elif DT_HAS_CHOSEN(zmk_kscan)

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#warning                                                                                           \
    "Ignoring the physical layouts and using the chosen kscan with a synthetic transform. Consider setting a chosen physical layout instead."

#endif

ZMK_MATRIX_TRANSFORM_DEFAULT_EXTERN();
static const struct zmk_physical_layout _CONCAT(_zmk_physical_layout_, chosen) = {
    .display_name = "Default",
    .matrix_transform = &zmk_matrix_transform_default,
    .kscan = DEVICE_DT_GET(DT_CHOSEN(zmk_kscan)),
};

static const struct zmk_physical_layout *const layouts[] = {
    &_CONCAT(_zmk_physical_layout_, chosen)};

#endif

const struct zmk_physical_layout *active;

size_t zmk_physical_layouts_get_list(struct zmk_physical_layout const *const **dest_layouts) {
    *dest_layouts = &layouts[0];

    return ARRAY_SIZE(layouts);
}

#define ZMK_KSCAN_EVENT_STATE_PRESSED 0
#define ZMK_KSCAN_EVENT_STATE_RELEASED 1

struct zmk_kscan_event {
    uint32_t row;
    uint32_t column;
    uint32_t state;
};

static struct zmk_kscan_msg_processor {
    struct k_work work;
} msg_processor;

K_MSGQ_DEFINE(physical_layouts_kscan_msgq, sizeof(struct zmk_kscan_event),
              CONFIG_ZMK_KSCAN_EVENT_QUEUE_SIZE, 4);

static void zmk_physical_layout_kscan_callback(const struct device *dev, uint32_t row,
                                               uint32_t column, bool pressed) {
    if (dev != active->kscan) {
        return;
    }

    struct zmk_kscan_event ev = {
        .row = row,
        .column = column,
        .state = (pressed ? ZMK_KSCAN_EVENT_STATE_PRESSED : ZMK_KSCAN_EVENT_STATE_RELEASED)};

    k_msgq_put(&physical_layouts_kscan_msgq, &ev, K_NO_WAIT);
    k_work_submit(&msg_processor.work);
}

static void zmk_physical_layouts_kscan_process_msgq(struct k_work *item) {
    struct zmk_kscan_event ev;

    while (k_msgq_get(&physical_layouts_kscan_msgq, &ev, K_NO_WAIT) == 0) {
        bool pressed = (ev.state == ZMK_KSCAN_EVENT_STATE_PRESSED);
        int32_t position = zmk_matrix_transform_row_column_to_position(active->matrix_transform,
                                                                       ev.row, ev.column);

        if (position < 0) {
            LOG_WRN("Not found in transform: row: %d, col: %d, pressed: %s", ev.row, ev.column,
                    (pressed ? "true" : "false"));
            continue;
        }

        LOG_DBG("Row: %d, col: %d, position: %d, pressed: %s", ev.row, ev.column, position,
                (pressed ? "true" : "false"));
        raise_zmk_position_state_changed(
            (struct zmk_position_state_changed){.source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
                                                .state = pressed,
                                                .position = position,
                                                .timestamp = k_uptime_get()});
    }
}

static const struct zmk_physical_layout *get_default_layout(void) {
    const struct zmk_physical_layout *initial;

#if USE_PHY_LAYOUTS && DT_HAS_CHOSEN(zmk_physical_layout)
    initial = &_CONCAT(_zmk_physical_layout_, DT_CHOSEN(zmk_physical_layout));
#else
    initial = layouts[0];
#endif

    return initial;
}

static int get_index_of_layout(const struct zmk_physical_layout *layout) {
    for (int i = 0; i < ARRAY_SIZE(layouts); i++) {
        if (layouts[i] == layout) {
            return i;
        }
    }

    return -ENODEV;
}

static uint32_t selected_to_stock_map[ZMK_KEYMAP_LEN];

int zmk_physical_layouts_get_selected_to_stock_position_map(uint32_t const **map) {
    *map = selected_to_stock_map;
    return ZMK_KEYMAP_LEN;
}

int zmk_physical_layouts_select_layout(const struct zmk_physical_layout *dest_layout) {
    if (!dest_layout) {
        return -ENODEV;
    }

    if (dest_layout == active) {
        return 0;
    }

    if (active) {
        if (active->kscan) {
            kscan_disable_callback(active->kscan);
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
            pm_device_runtime_put(active->kscan);
#elif IS_ENABLED(CONFIG_PM_DEVICE)
            pm_device_action_run(active->kscan, PM_DEVICE_ACTION_SUSPEND);
#endif
        }
    }

    int new_idx = get_index_of_layout(dest_layout);
    int stock_idx = get_index_of_layout(get_default_layout());
    int ret = zmk_physical_layouts_get_position_map(stock_idx, new_idx, ZMK_KEYMAP_LEN,
                                                    selected_to_stock_map);
    if (ret < 0) {
        LOG_ERR("Failed to refresh the selected to stock mapping (%d)", ret);
        return ret;
    }

    active = dest_layout;

    if (active->kscan) {
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
        int err = pm_device_runtime_get(active->kscan);
        if (err < 0) {
            LOG_WRN("PM runtime get of kscan device to enable it %d", err);
            return err;
        }
#elif IS_ENABLED(CONFIG_PM_DEVICE)
        pm_device_action_run(active->kscan, PM_DEVICE_ACTION_RESUME);
#endif
        kscan_config(active->kscan, zmk_physical_layout_kscan_callback);
        kscan_enable_callback(active->kscan);
    }

    return 0;
}

int zmk_physical_layouts_select(uint8_t index) {
    if (index >= ARRAY_SIZE(layouts)) {
        return -EINVAL;
    }

    int ret = zmk_physical_layouts_select_layout(layouts[index]);

    if (ret >= 0) {
        raise_zmk_physical_layout_selection_changed(
            (struct zmk_physical_layout_selection_changed){.selection = index});
    }

    return ret;
}

int zmk_physical_layouts_get_selected(void) {
    for (int i = 0; i < ARRAY_SIZE(layouts); i++) {
        if (layouts[i] == active) {
            return i;
        }
    }

    return -ENODEV;
}

#if IS_ENABLED(CONFIG_SETTINGS)

static int8_t saved_selected_index = -1;

#endif

int zmk_physical_layouts_select_initial(void) {
    int ret = zmk_physical_layouts_select_layout(get_default_layout());

    return ret;
}

int zmk_physical_layouts_check_unsaved_selection(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    return saved_selected_index < 0 ||
                   saved_selected_index == (uint8_t)zmk_physical_layouts_get_selected()
               ? 0
               : 1;
#else
    return -ENOTSUP;
#endif
}

int zmk_physical_layouts_save_selected(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    uint8_t val = (uint8_t)zmk_physical_layouts_get_selected();

    return settings_save_one("physical_layouts/selected", &val, sizeof(val));
#else
    return -ENOTSUP;
#endif
}

int zmk_physical_layouts_revert_selected(void) { return zmk_physical_layouts_select_initial(); }

int zmk_physical_layouts_get_position_map(uint8_t source, uint8_t dest, size_t map_size,
                                          uint32_t map[map_size]) {
    if (source >= ARRAY_SIZE(layouts) || dest >= ARRAY_SIZE(layouts)) {
        return -EINVAL;
    }

    if (source == dest) {
        for (int i = 0; i < map_size; i++) {
            map[i] = i;
        }

        return 0;
    }

    const struct zmk_physical_layout *src_layout = layouts[source];
    const struct zmk_physical_layout *dest_layout = layouts[dest];
    int max_kp = dest_layout->keys_len;

#if HAVE_POS_MAP
    const struct position_map_entry *src_pos_map = NULL;
    const struct position_map_entry *dest_pos_map = NULL;

    for (int pm = 0; pm < ARRAY_SIZE(positions_maps); pm++) {
        if (positions_maps[pm].layout == src_layout) {
            src_pos_map = &positions_maps[pm];
        }

        if (positions_maps[pm].layout == dest_layout) {
            dest_pos_map = &positions_maps[pm];
        }
    }

    // Maps can place items "off the end" of other layouts so they are
    // preserved but not visible, so adjust our max here if that is being used.
    if (src_pos_map && dest_pos_map) {
        for (int mp = 0; mp < ZMK_POS_MAP_LEN; mp++) {
            max_kp =
                MAX(max_kp, MAX(src_pos_map->positions[mp] + 1, dest_pos_map->positions[mp] + 1));
        }
    }
#endif

    if (map_size < max_kp) {
        return -EINVAL;
    }

    memset(map, UINT32_MAX, map_size);

    for (int b = 0; b < max_kp; b++) {
        bool found = false;

#if HAVE_POS_MAP
        if (src_pos_map && dest_pos_map) {
            for (int m = 0; m < ZMK_POS_MAP_LEN; m++) {
                if (dest_pos_map->positions[m] == b) {
                    map[b] = src_pos_map->positions[m];
                    found = true;
                    break;
                }
            }
        }
#endif

#if !POS_MAP_COMPLETE
        if (!found) {
            const struct zmk_key_physical_attrs *key = &dest_layout->keys[b];
            for (int old_b = 0; old_b < src_layout->keys_len; old_b++) {
                const struct zmk_key_physical_attrs *candidate_key = &src_layout->keys[old_b];

                if (candidate_key->x == key->x && candidate_key->y == key->y) {
                    map[b] = old_b;
                    found = true;
                    break;
                }
            }
        }
#endif
    }

    return max_kp;
}

#if IS_ENABLED(CONFIG_SETTINGS)

static int physical_layouts_handle_set(const char *name, size_t len, settings_read_cb read_cb,
                                       void *cb_arg) {
    const char *next;

    if (settings_name_steq(name, "selected", &next) && !next) {
        if (len != sizeof(saved_selected_index)) {
            return -EINVAL;
        }

        int err = read_cb(cb_arg, &saved_selected_index, len);
        if (err <= 0) {
            LOG_ERR("Failed to handle selected physical dest_layout from settings (err %d)", err);
            return err;
        }

        return zmk_physical_layouts_select(saved_selected_index);
    }

    return 0;
};

SETTINGS_STATIC_HANDLER_DEFINE(physical_layouts, "physical_layouts", NULL,
                               physical_layouts_handle_set, NULL, NULL);

#endif // IS_ENABLED(CONFIG_SETTINGS)

static int zmk_physical_layouts_init(void) {
    k_work_init(&msg_processor.work, zmk_physical_layouts_kscan_process_msgq);

#if IS_ENABLED(CONFIG_PM_DEVICE)
    for (int l = 0; l < ARRAY_SIZE(layouts); l++) {
        const struct zmk_physical_layout *pl = layouts[l];
        if (pl->kscan && pm_device_wakeup_is_capable(pl->kscan) &&
            !pm_device_wakeup_enable(pl->kscan, true)) {
            LOG_WRN("Failed to wakeup enable %s", pl->kscan->name);
        }
    }
#endif // IS_ENABLED(CONFIG_PM_DEVICE)

    // Initialize a sane mapping
    for (int i = 0; i < ZMK_KEYMAP_LEN; i++) {
        selected_to_stock_map[i] = i;
    }

    return zmk_physical_layouts_select_initial();
}

SYS_INIT(zmk_physical_layouts_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
