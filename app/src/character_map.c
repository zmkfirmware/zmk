/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_character_map

#include <stddef.h>
#include <stdlib.h>
#include <drivers/character_map.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct codepoint_param {
    uint32_t codepoint;
    uint32_t param;
};

struct character_map_config {
    const char *behavior_dev;
    const char *fallback_behavior_dev;
    struct codepoint_param *map;
    size_t map_size;
};

static int compare_codepoints(const void *lhs, const void *rhs) {
    const struct codepoint_param *lhs_item = (const struct codepoint_param *)lhs;
    const struct codepoint_param *rhs_item = (const struct codepoint_param *)rhs;

    return (int64_t)lhs_item->codepoint - (int64_t)rhs_item->codepoint;
}

static int codepoint_to_binding(const struct device *dev, uint32_t codepoint,
                                struct zmk_behavior_binding *binding) {
    const struct character_map_config *config = dev->config;

    const struct codepoint_param key = {.codepoint = codepoint};
    const struct codepoint_param *result =
        bsearch(&key, config->map, config->map_size, sizeof(config->map[0]), compare_codepoints);

    if (result) {
        *binding = (struct zmk_behavior_binding){.behavior_dev = config->behavior_dev,
                                                 .param1 = result->param};
        return 0;
    }

    if (config->fallback_behavior_dev) {
        *binding = (struct zmk_behavior_binding){.behavior_dev = config->fallback_behavior_dev,
                                                 .param1 = codepoint};
        return 0;
    }

    return -ENOTSUP;
}

static const struct character_map_driver_api character_map_driver_api = {
    .codepoint_to_binding = codepoint_to_binding,
};

static int character_map_init(const struct device *dev) {
    const struct character_map_config *config = dev->config;

    // Sort the character map by codepoint for faster lookup
    qsort(config->map, config->map_size, sizeof(config->map[0]), compare_codepoints);

    return 0;
}

#define MAP_LEN(n) DT_INST_PROP_LEN(n, map)
#define MAP_INIT(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx),

#define CHARMAP_INST(n)                                                                            \
    BUILD_ASSERT(MAP_LEN(n) > 0, "'map' property must not be an empty array.");                    \
    BUILD_ASSERT(MAP_LEN(n) % 2 == 0,                                                              \
                 "'map' property must be an array of <codepoint param> pairs.");                   \
                                                                                                   \
    /* Since we can't iterate over the "map" elements pairwise, we write all the values to a flat  \
     * array and then reinterpret it as an array of struct codepoint_param. */                     \
    static uint32_t character_map_array_##n[] = {DT_INST_FOREACH_PROP_ELEM(n, map, MAP_INIT)};     \
                                                                                                   \
    BUILD_ASSERT(sizeof(character_map_array_##n) % sizeof(struct codepoint_param) == 0,            \
                 "sizeof(struct codepoint_param) must evenly divide array size");                  \
                                                                                                   \
    static const struct character_map_config character_map_config_##n = {                          \
        .behavior_dev = DEVICE_DT_NAME(DT_INST_PROP(n, behavior)),                                 \
        .fallback_behavior_dev =                                                                   \
            COND_CODE_1(DT_INST_NODE_HAS_PROP(n, fallback_behavior),                               \
                        (DT_DEVICE_NAME(DT_INST_PROP(n, fallback_behavior))), (NULL)),             \
        .map = (struct codepoint_param *)character_map_array_##n,                                  \
        .map_size = ARRAY_SIZE(character_map_array_##n) / 2,                                       \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(n, character_map_init, NULL, NULL, &character_map_config_##n,            \
                          POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,                           \
                          &character_map_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CHARMAP_INST);
