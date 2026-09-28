/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/combos.h>
#include <zmk/matrix.h>
#include <zmk/keymap.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define CMD_HELP_COMBOS "Manage runtime combos."
#define CMD_HELP_COMBOS_CREATE "Create a new combo."
#define CMD_HELP_COMBOS_LIST "List all registered combos."

#include <dt-bindings/zmk/keys.h>

static void print_combo_details(const struct shell *shell, const struct zmk_combo_runtime *cr) {
    shell_print(shell, "\tID: %d", cr->id);
    shell_print(shell, "\tbehavior: %s - x%x/x%x", cr->combo.behavior.behavior_dev,
                cr->combo.behavior.param1, cr->combo.behavior.param2);

    shell_fprintf_normal(shell, "\tPositions: ");
    for (size_t kp = 0; kp < cr->combo.key_position_len; kp++) {
        shell_fprintf_normal(shell, "%d ", cr->combo.key_positions[kp]);
    }
    shell_print(shell, "");
    shell_print(shell, "\ttimeout (ms): %d", cr->combo.timeout_ms);
    shell_print(shell, "\trequire prior idle (ms): %d", cr->combo.require_prior_idle_ms);
    shell_fprintf_normal(shell, "\tLayers: ");
    if (cr->combo.layer_mask) {
        for (size_t l = 0; l < ZMK_KEYMAP_LAYERS_LEN; l++) {
            if (cr->combo.layer_mask & BIT(l)) {
                shell_fprintf_normal(shell, "%d ", l);
            }
        }
        shell_print(shell, "");
    } else {
        shell_print(shell, "All");
    }
    shell_print(shell, "\tSlow release: %s", cr->combo.slow_release ? "Yes" : "No");
}

static int cmd_combo_add_with_behavior_cb(const struct shell *shell, size_t argc, char **argv) {
    char *endptr;
    const struct device *behavior_dev = device_get_binding(argv[0]);
    if (!behavior_dev) {
        shell_print(shell, "Failed to add the combo: %s is not a valid behavior", argv[0]);
        return -ENODEV;
    }

    int32_t param1 = strtoul(argv[1], &endptr, 10);
    if (*endptr != '\0') {
        shell_print(shell, "Invalid behavior parameter %s", argv[1]);
        return -EINVAL;
    }

    int32_t param2 = strtoul(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        shell_print(shell, "Invalid behavior 2nd parameter %s", argv[2]);
        return -EINVAL;
    }

    struct combo_cfg combo = (struct combo_cfg){
        .behavior =
            {
                .behavior_dev = behavior_dev->name,
                .param1 = param1,
                .param2 = param2,
            },
        .timeout_ms = 5000, /* Default value */
    };

    size_t kp_len = argc - 1 - 2; // Subtract command name and two behavior params

    if (kp_len < 2) {
        shell_print(shell, "Combos need have at least two key positions");
        return -EINVAL;
    }

    for (size_t i = 0; i < kp_len; i++) {
        int pos = strtoul(argv[i + 1 + 2], &endptr, 10);
        if (*endptr != '\0' || pos >= ZMK_KEYMAP_LEN) {
            shell_print(shell, "Invalid key position %s for combo", argv[i]);
            return -EINVAL;
        }

        combo.key_positions[i] = (uint16_t)pos;
    }

    combo.key_position_len = kp_len;

    int ret = zmk_combo_runtime_add_combo(&combo);
    if (ret < 0) {
        shell_print(shell, "Failed to add the combo (%d)", ret);
        return ret;
    }
    shell_print(shell, "Added ret is %d", ret);

    const struct zmk_combo_runtime *rc = zmk_combo_runtime_get_combo(ret);

    shell_print(shell, "Added the combo:");
    print_combo_details(shell, rc);

    return 0;
}

static void cmd_add_behaviors_get(size_t idx, struct shell_static_entry *entry) {
    /* -1 because the last element in the list is a "list terminator" */
    size_t beh_count;
    STRUCT_SECTION_COUNT(zmk_behavior_ref, &beh_count);
    if (idx < beh_count - 1) {
        struct zmk_behavior_ref *ref;

        STRUCT_SECTION_GET(zmk_behavior_ref, idx, &ref);

        entry->syntax = ref->device->name;
        entry->handler = &cmd_combo_add_with_behavior_cb;
        entry->subcmd = NULL;
        entry->args.mandatory = 1 + 2 + 2;
        entry->args.optional = MAX_COMBO_KEYS - 2;
        entry->help = ref->metadata.display_name;
    } else {
        entry->syntax = NULL;
    }
}

SHELL_DYNAMIC_CMD_CREATE(sub_combos_add_behavior, cmd_add_behaviors_get);

static int cmd_combos_list_cb(const struct shell *shell, size_t argc, char **argv) {
    const struct zmk_combo_runtime *list;
    int count = zmk_combo_runtime_get_combos(&list);
    if (count < 0) {
        shell_error(shell, "Failed to get the combo list (%d)", count);
    }

    for (size_t i = 0; i < count; i++) {
        shell_print(shell, "Combo #%d:", list[i].id);
        print_combo_details(shell, &list[i]);
        shell_print(shell, "");
    }

    return 0;
}

static int cmd_combo_item_show_cb(const struct shell *shell, size_t argc, char **argv) {
    char *endptr;

    int combo_id = strtoul(argv[-1], &endptr, 10);
    if (*endptr != '\0') {
        return -EINVAL;
    }

    const struct zmk_combo_runtime *rc = zmk_combo_runtime_get_combo(combo_id);

    shell_print(shell, "Combo %d:", combo_id);
    print_combo_details(shell, rc);

    return 0;
}

static int cmd_combo_item_remove_cb(const struct shell *shell, size_t argc, char **argv) {
    char *endptr;
    int combo_id = strtoul(argv[-1], &endptr, 10);
    if (*endptr != '\0') {
        return -EINVAL;
    }

    return zmk_combo_runtime_remove_combo(combo_id);
}

static int cmd_combo_item_add_remove_layer_position_cb(const struct shell *shell, size_t argc,
                                                       char **argv) {
    char *endptr;
    int ret;

    __ASSERT(argc == 2, "Need two arguments");

    int combo_id = strtoul(argv[-2], &endptr, 10);
    if (*endptr != '\0') {
        return -EINVAL;
    }

    int layer = strtoul(argv[1], &endptr, 10);
    if (*endptr != '\0') {
        shell_error(shell, "Invalid value %s", argv[1]);
        return -EINVAL;
    }

    if (!strcmp("layers", argv[-1])) {
        ret = zmk_combo_runtime_set_combo_layer(combo_id, layer, !strcmp(argv[0], "add"));
        if (ret < 0) {
            shell_error(shell, "Failed to %s layer (%d)", argv[0], ret);
        }
    } else if (!strcmp("positions", argv[-1])) {
        if (!strcmp(argv[0], "add")) {
            ret = zmk_combo_runtime_add_combo_position(combo_id, layer);
        } else {
            ret = zmk_combo_runtime_remove_combo_position(combo_id, layer);
        }

        if (ret < 0) {
            shell_error(shell, "Failed to %s position (%d)", argv[0], ret);
        }
    } else {
        shell_error(shell, "Invalid property to add/remove from: %s", argv[-1]);
        return -EINVAL;
    }

    return ret;
}

static int cmd_combo_item_all_layers_cb(const struct shell *shell, size_t argc, char **argv) {
    char *endptr;

    int combo_id = strtoul(argv[-2], &endptr, 10);
    if (*endptr != '\0') {
        shell_error(shell, "Invalid combo %s", argv[-2]);
        return -EINVAL;
    }

    int ret = zmk_combo_runtime_clear_combo_layers(combo_id);
    if (ret < 0) {
        shell_error(shell, "Failed to clear layers (%d)", ret);
    }

    return ret;
}

static int cmd_combo_item_set_value_cb(const struct shell *shell, size_t argc, char **argv) {
    char *endptr;

    int combo_id = strtoul(argv[-2], &endptr, 10);
    if (*endptr != '\0') {
        return -EINVAL;
    }

    if (strcmp(argv[0], "timeout") == 0) {
        int timeout = strtoul(argv[argc - 1], &endptr, 10);
        if (*endptr != '\0') {
            shell_error(shell, "Invalid timeout %s", argv[argc - 1]);
            return -EINVAL;
        }
        return zmk_combo_runtime_set_combo_timeout(combo_id, timeout);
    } else if (strcmp(argv[0], "prior_idle") == 0) {
        int prior_idle = strtoul(argv[argc - 1], &endptr, 10);
        if (*endptr != '\0') {
            shell_error(shell, "Invalid prior idle %s", argv[argc - 1]);
            return -EINVAL;
        }
        return zmk_combo_runtime_set_combo_prior_idle(combo_id, prior_idle);
    } else if (strcmp(argv[0], "slow_release") == 0) {
        return zmk_combo_runtime_set_combo_slow_release(combo_id, !strcmp(argv[-1], "set"));
    } else {
        return -ENOTSUP;
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    cmd_combo_item_set_list,
    SHELL_CMD_ARG(timeout, NULL, "Timeout (ms)\n", cmd_combo_item_set_value_cb, 2, 0),
    SHELL_CMD_ARG(prior_idle, NULL, "Prior Idle (ms)\n", cmd_combo_item_set_value_cb, 2, 0),
    SHELL_CMD(slow_release, NULL, "Slow Release\n", cmd_combo_item_set_value_cb),
    SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(cmd_combo_item_unset_list,
                               SHELL_CMD(slow_release, NULL, "Slow Release\n",
                                         cmd_combo_item_set_value_cb),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(cmd_combo_item_layers_list,
                               SHELL_CMD(all, NULL, "Clear layers filter, apply on all layers\n",
                                         cmd_combo_item_all_layers_cb),
                               SHELL_CMD_ARG(add, NULL, "Add a layer\n",
                                             cmd_combo_item_add_remove_layer_position_cb, 2, 0),
                               SHELL_CMD_ARG(remove, NULL, "Remove a layer\n",
                                             cmd_combo_item_add_remove_layer_position_cb, 2, 0),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(cmd_combo_item_positions_list,
                               SHELL_CMD_ARG(add, NULL, "Add a position\n",
                                             cmd_combo_item_add_remove_layer_position_cb, 2, 0),
                               SHELL_CMD_ARG(remove, NULL, "Remove a position\n",
                                             cmd_combo_item_add_remove_layer_position_cb, 2, 0),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(
    cmd_combo_item_list, SHELL_CMD(show, NULL, "Show\n", cmd_combo_item_show_cb),
    SHELL_CMD(remove, NULL, "Remove\n", cmd_combo_item_remove_cb),
    SHELL_CMD(set, &cmd_combo_item_set_list, "Set Properties\n", NULL),
    SHELL_CMD(unset, &cmd_combo_item_unset_list, "Unset Properties\n", NULL),
    SHELL_CMD(layers, &cmd_combo_item_layers_list, "Layers\n", NULL),
    SHELL_CMD(positions, &cmd_combo_item_positions_list, "Positions\n", NULL),
    SHELL_SUBCMD_SET_END /* Array terminated. */
);

static int compare_ids(const void *a, const void *b) {
    return *((zmk_combo_runtime_id_t *)a) - *((zmk_combo_runtime_id_t *)b);
}

static void cmd_combo_commands_get(size_t idx, struct shell_static_entry *entry) {
    const struct zmk_combo_runtime *list;
    int count = zmk_combo_runtime_get_combos(&list);

    if (idx == count) {
        entry->syntax = "add";
        entry->subcmd = &sub_combos_add_behavior;
        entry->handler = NULL;
        entry->help = "Add a new combo";
    } else if (idx == count + 1) {
        entry->syntax = "list";
        entry->subcmd = NULL;
        entry->handler = &cmd_combos_list_cb;
        entry->help = "List the available combos";
    } else if (idx > count + 1) {
        entry->syntax = NULL;
    } else {
        zmk_combo_runtime_id_t ids[count];
        for (size_t i = 0; i < count; i++) {
            ids[i] = list[i].id;
        }

        qsort(&ids, count, sizeof(ids[0]), compare_ids);
        static char count_str[4];
        snprintf(count_str, ARRAY_SIZE(count_str), "%ld", idx);
        entry->syntax = count_str;
        entry->handler = NULL;
        entry->subcmd = &cmd_combo_item_list;
        entry->help = "Show/Edit this combo";
    }
}

SHELL_DYNAMIC_CMD_CREATE(sub_combos_command_list, cmd_combo_commands_get);

SHELL_CMD_REGISTER(combos, &sub_combos_command_list, "Combo commands", NULL);
