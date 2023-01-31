/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_kscan_deghost

#include <zephyr/device.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/logging/log.h>
#include <dt-bindings/zmk/matrix_transform.h>
#include <zmk/matrix.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct kscan_deghost_config {
    const struct device *child_kscan;
    kscan_callback_t callback_for_child;
    int rows;
    int cols;
    const uint8_t *transform_filled;
    int transform_filled_len;
};

struct kscan_deghost_data {
    kscan_callback_t callback;
    uint8_t *key_status;
};

static int kscan_deghost_enable_callback(const struct device *dev) {
    const struct kscan_deghost_config *config = dev->config;
    return kscan_enable_callback(config->child_kscan);
}

static int kscan_deghost_disable_callback(const struct device *dev) {
    const struct kscan_deghost_config *config = dev->config;
    return kscan_disable_callback(config->child_kscan);
}

static int kscan_deghost_configure(const struct device *dev, kscan_callback_t callback) {
    const struct kscan_deghost_config *config = dev->config;
    struct kscan_deghost_data *data = dev->data;

    if (!callback) {
        return -EINVAL;
    }

    data->callback = callback;
    return kscan_config(config->child_kscan, config->callback_for_child);
}

#define KEY_STATUS_FORCE_NOT_RECHECK (1 << 2)
#define KEY_STATUS_REPORTED_AS_PRESSED_MASK (1 << 1)
#define KEY_STATUS_SEEN_AS_PRESSED_MASK 1
#define CNT_KEY_STATUS_SEEN_AS_PRESSED(value) ((value)&1)
#define WAS_KEY_STATUS_GHOSTING_BEFORE(value) ((value) == 1)
#define KEY(row, col) data->key_status[(row)*config->cols + (col)]
#define EXISTS(row, col)                                                                           \
    ((config->transform_filled_len == 0)                                                           \
         ? 1                                                                                       \
         : ((config->transform_filled_len <= (row)*config->cols + (col))                           \
                ? 0                                                                                \
                : (config->transform_filled[(row)*config->cols + (col)])))

static void kscan_deghost_callback_for_child(const struct device *deghost_dev, uint32_t row,
                                             uint32_t col, bool pressed) {
    const struct kscan_deghost_config *config = deghost_dev->config;
    struct kscan_deghost_data *data = deghost_dev->data;
    if (!EXISTS(row, col))
        return;
    if (pressed) {
        KEY(row, col) |= KEY_STATUS_SEEN_AS_PRESSED_MASK;
        bool ghosting = false;
        for (uint32_t orow = 0; (orow < config->rows) && !ghosting; orow++) {
            if (orow == row || !EXISTS(orow, col))
                continue;
            const int other_row_pressed = CNT_KEY_STATUS_SEEN_AS_PRESSED(KEY(orow, col));
            for (uint32_t ocol = 0; ocol < config->cols; ocol++) {
                if (ocol == col || (!EXISTS(row, ocol)) || (!EXISTS(orow, ocol)))
                    continue;
                const int pressed_in_rectangle = 1 + other_row_pressed +
                                                 CNT_KEY_STATUS_SEEN_AS_PRESSED(KEY(row, ocol)) +
                                                 CNT_KEY_STATUS_SEEN_AS_PRESSED(KEY(orow, ocol));
                if (pressed_in_rectangle > 2) {
                    ghosting = true;
                    break;
                }
            }
        }
        if (!ghosting) {
            KEY(row, col) |= KEY_STATUS_REPORTED_AS_PRESSED_MASK;
            data->callback(deghost_dev, row, col, pressed);
        }
    } else {
        if (KEY(row, col) & KEY_STATUS_REPORTED_AS_PRESSED_MASK) {
            KEY(row, col) = 0;
            data->callback(deghost_dev, row, col, pressed);
        } else {
            KEY(row, col) = 0;
        }
        for (uint32_t orow = 0; (orow < config->rows); orow++) {
            if (orow == row || !EXISTS(orow, col))
                continue;
            const int other_row_pressed = CNT_KEY_STATUS_SEEN_AS_PRESSED(KEY(orow, col));
            int check_orow_col = WAS_KEY_STATUS_GHOSTING_BEFORE(KEY(orow, col));
            for (uint32_t ocol = 0; ocol < config->cols; ocol++) {
                if (ocol == col || (!EXISTS(row, ocol)) || (!EXISTS(orow, ocol)))
                    continue;
                const int pressed_in_rectangle = 0 + other_row_pressed +
                                                 CNT_KEY_STATUS_SEEN_AS_PRESSED(KEY(row, ocol)) +
                                                 CNT_KEY_STATUS_SEEN_AS_PRESSED(KEY(orow, ocol));
                if (pressed_in_rectangle == 2) {
                    if (WAS_KEY_STATUS_GHOSTING_BEFORE(KEY(row, ocol))) {
                        kscan_deghost_callback_for_child(deghost_dev, row, ocol, true);
                        KEY(row, ocol) |=
                            KEY_STATUS_FORCE_NOT_RECHECK; // optimization, next
                                                          // WAS_KEY_STATUS_GHOSTING_BEFORE()
                                                          // call will be false
                    }
                    if (WAS_KEY_STATUS_GHOSTING_BEFORE(KEY(orow, ocol))) {
                        kscan_deghost_callback_for_child(deghost_dev, orow, ocol, true);
                    }
                    if (check_orow_col) {
                        kscan_deghost_callback_for_child(deghost_dev, orow, col, true);
                        check_orow_col = 0; // for optimization
                    }
                }
            }
        }
        for (uint32_t ocol = 0; ocol < config->cols; ocol++) {
            KEY(row, ocol) &= ~KEY_STATUS_FORCE_NOT_RECHECK;
        }
    }
}

static int kscan_deghost_init(const struct device *dev) { return 0; }

static const struct kscan_driver_api deghost_driver_api = {
    .config = kscan_deghost_configure,
    .enable_callback = kscan_deghost_enable_callback,
    .disable_callback = kscan_deghost_disable_callback,
};

#define _TRANSFORM_ENTRY(i, transform_node)                                                        \
    [(KT_ROW(DT_PROP_BY_IDX(transform_node, map, i)) * DT_PROP(transform_node, columns)) +         \
        KT_COL(DT_PROP_BY_IDX(transform_node, map, i))] = 1

#define KSCAN_DEGHOST_INIT(n)                                                                      \
    static const uint8_t transform_filled_##n[] = {                                                \
        COND_CODE_1(DT_INST_NODE_HAS_PROP(n, transform),                                           \
                    (LISTIFY(DT_PROP_LEN(DT_INST_PHANDLE(n, transform), map), _TRANSFORM_ENTRY,    \
                             (, ), DT_INST_PHANDLE(n, transform))),                                \
                    ())};                                                                          \
    static void kscan_deghost_callback_for_child_##n(const struct device *child_dev, uint32_t row, \
                                                     uint32_t column, bool pressed) {              \
        const struct device *deghost_dev = DEVICE_DT_GET(DT_DRV_INST(n));                          \
        kscan_deghost_callback_for_child(deghost_dev, row, column, pressed);                       \
    }                                                                                              \
                                                                                                   \
    static uint8_t                                                                                 \
        key_status_##n[DT_INST_PROP_OR(                                                            \
            n, rows,                                                                               \
            DT_PROP_OR(DT_INST_PHANDLE(n, transform), rows,                                        \
                       DT_PROP_LEN_OR(DT_INST_PHANDLE(n, kscan), row_gpios, ZMK_MATRIX_ROWS)))]    \
                      [DT_INST_PROP_OR(n, columns,                                                 \
                                       DT_PROP_OR(DT_INST_PHANDLE(n, transform), columns,          \
                                                  DT_PROP_LEN_OR(DT_INST_PHANDLE(n, kscan),        \
                                                                 col_gpios, ZMK_MATRIX_COLS)))];   \
                                                                                                   \
    static const struct kscan_deghost_config kscan_deghost_config_##n = {                          \
        .child_kscan = DEVICE_DT_GET(DT_INST_PHANDLE(n, kscan)),                                   \
        .callback_for_child = &kscan_deghost_callback_for_child_##n,                               \
        .rows = ARRAY_SIZE(key_status_##n),                                                        \
        .cols = ARRAY_SIZE(key_status_##n[0]),                                                     \
        .transform_filled = transform_filled_##n,                                                  \
        .transform_filled_len = ARRAY_SIZE(transform_filled_##n),                                  \
    };                                                                                             \
                                                                                                   \
    static struct kscan_deghost_data kscan_deghost_data_##n = {                                    \
        .key_status = &key_status_##n[0][0],                                                       \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(n, kscan_deghost_init, NULL, &kscan_deghost_data_##n,                    \
                          &kscan_deghost_config_##n, APPLICATION,                                  \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &deghost_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_DEGHOST_INIT);
