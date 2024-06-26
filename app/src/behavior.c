/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/util_macro.h>
#include <string.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/hid.h>
#include <zmk/matrix.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

const struct device *zmk_behavior_get_binding(const char *name) {
    return behavior_get_binding(name);
}

const struct device *z_impl_behavior_get_binding(const char *name) {
    if (name == NULL || name[0] == '\0') {
        return NULL;
    }

    STRUCT_SECTION_FOREACH(zmk_behavior_ref, item) {
        if (z_device_is_ready(item->device) && item->device->name == name) {
            return item->device;
        }
    }

    STRUCT_SECTION_FOREACH(zmk_behavior_ref, item) {
        if (z_device_is_ready(item->device) && strcmp(item->device->name, name) == 0) {
            return item->device;
        }
    }

    return NULL;
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

int zmk_behavior_get_empty_param_metadata(const struct device *dev,
                                          struct behavior_parameter_metadata *metadata) {
    metadata->sets_len = 0;
    return 0;
}

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static int validate_hid_usage(uint16_t usage_page, uint16_t usage_id) {
    LOG_DBG("Validate usage %d in page %d", usage_id, usage_page);
    switch (usage_page) {
    case HID_USAGE_KEY:
        if (usage_id == 0 || (usage_id > ZMK_HID_KEYBOARD_NKRO_MAX_USAGE &&
                              usage_id < LEFT_CONTROL && usage_id > RIGHT_GUI)) {
            return -EINVAL;
        }
        break;
    case HID_USAGE_CONSUMER:
        if (usage_id >
            COND_CODE_1(IS_ENABLED(CONFIG_ZMK_HID_CONSUMER_REPORT_USAGES_BASIC), (0xFF), (0xFFF))) {
            return -EINVAL;
        }
        break;
    default:
        LOG_WRN("Unsupported HID usage page %d", usage_page);
        return -EINVAL;
    }

    return 0;
}

#define PARAM_MATCHES 0

static int check_param_matches_value(const struct behavior_parameter_value_metadata *value_meta,
                                     uint32_t param) {
    switch (value_meta->type) {
    case BEHAVIOR_PARAMETER_VALUE_TYPE_NIL:
        if (param == 0) {
            return PARAM_MATCHES;
        }
        break;
    case BEHAVIOR_PARAMETER_VALUE_TYPE_HID_USAGE:
        if (validate_hid_usage(ZMK_HID_USAGE_PAGE(param), ZMK_HID_USAGE_ID(param)) >= 0) {
            return PARAM_MATCHES;
        }

        break;
    case BEHAVIOR_PARAMETER_VALUE_TYPE_LAYER_INDEX:
        if (param >= 0 && param < ZMK_KEYMAP_LEN) {
            return PARAM_MATCHES;
        }
        break;
        /* TODO: Restore with HSV -> RGB refactor
        case BEHAVIOR_PARAMETER_STANDARD_DOMAIN_HSV:
            // TODO: No real way to validate? Maybe max brightness?
            break;
        */
    case BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE:
        if (param == value_meta->value) {
            return PARAM_MATCHES;
        }
        break;
    case BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE:
        if (param >= value_meta->range.min && param <= value_meta->range.max) {
            return PARAM_MATCHES;
        }
        break;
    default:
        LOG_WRN("Unknown type %d", value_meta->type);
        break;
    }

    return -ENOTSUP;
}

int zmk_behavior_validate_param_values(const struct behavior_parameter_value_metadata *values,
                                       size_t values_len, uint32_t param) {
    if (values_len == 0) {
        return -ENODEV;
    }

    for (int v = 0; v < values_len; v++) {
        int ret = check_param_matches_value(&values[v], param);
        if (ret >= 0) {
            return ret;
        }
    }

    return -EINVAL;
}

int zmk_behavior_check_params_match_metadata(const struct behavior_parameter_metadata *metadata,
                                             uint32_t param1, uint32_t param2) {
    if (!metadata || metadata->sets_len == 0) {
        if (!metadata) {
            LOG_ERR("No metadata to check against");
        }

        return (param1 == 0 && param2 == 0) ? 0 : -ENODEV;
    }

    for (int s = 0; s < metadata->sets_len; s++) {
        const struct behavior_parameter_metadata_set *set = &metadata->sets[s];
        int param1_ret =
            zmk_behavior_validate_param_values(set->param1_values, set->param1_values_len, param1);
        int param2_ret =
            zmk_behavior_validate_param_values(set->param2_values, set->param2_values_len, param2);

        if ((param1_ret >= 0 || (param1_ret == -ENODEV && param1 == 0)) &&
            (param2_ret >= 0 || (param2_ret == -ENODEV && param2 == 0))) {
            return 0;
        }
    }

    return -EINVAL;
}

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

int zmk_behavior_validate_binding(const struct zmk_behavior_binding *binding) {
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    const struct device *behavior = zmk_behavior_get_binding(binding->behavior_dev);

    if (!behavior) {
        return -ENODEV;
    }

    struct behavior_parameter_metadata metadata;
    int ret = behavior_get_parameter_metadata(behavior, &metadata);

    if (ret < 0) {
        LOG_WRN("Failed getting metadata for %s: %d", binding->behavior_dev, ret);
        return ret;
    }

    return zmk_behavior_check_params_match_metadata(&metadata, binding->param1, binding->param2);
#else
    return 0;
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
}

#if IS_ENABLED(CONFIG_LOG)
static int check_behavior_names(void) {
    // Behavior names must be unique, but we don't have a good way to enforce this
    // at compile time, so log an error at runtime if they aren't unique.
    ptrdiff_t count;
    STRUCT_SECTION_COUNT(zmk_behavior_ref, &count);

    for (ptrdiff_t i = 0; i < count; i++) {
        const struct zmk_behavior_ref *current;
        STRUCT_SECTION_GET(zmk_behavior_ref, i, &current);

        for (ptrdiff_t j = i + 1; j < count; j++) {
            const struct zmk_behavior_ref *other;
            STRUCT_SECTION_GET(zmk_behavior_ref, j, &other);

            if (strcmp(current->device->name, other->device->name) == 0) {
                LOG_ERR("Multiple behaviors have the same name '%s'", current->device->name);
            }
        }
    }

    return 0;
}

SYS_INIT(check_behavior_names, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif // IS_ENABLED(CONFIG_LOG)
