/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_boot_magic_combo

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>

#include <zmk/reset.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)

#include <zephyr/retention/bootmode.h>

#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define _COMBO_LEN(inst) uint8_t _CONCAT(_len_, inst)[DT_INST_PROP_LEN(inst, combo_positions)];
#define MAX_BOOT_COMBO_LEN sizeof(union {DT_INST_FOREACH_STATUS_OKAY(_COMBO_LEN)})

struct boot_key_config {
    const uint16_t *combo_positions;
    uint8_t combo_positions_len;
    bool jump_to_bootloader;
    bool reset_settings;
};

#define BOOT_KEY_COMBO_POSITIONS(n)                                                                \
    static const uint16_t boot_key_combo_positions_##n[] = DT_INST_PROP(n, combo_positions);

#define BOOT_KEY_CONFIG(n)                                                                         \
    BOOT_KEY_COMBO_POSITIONS(n){                                                                   \
        .combo_positions = boot_key_combo_positions_##n,                                           \
        .combo_positions_len = DT_INST_PROP_LEN(n, combo_positions),                               \
        .jump_to_bootloader = DT_INST_PROP_OR(n, jump_to_bootloader, false),                       \
        .reset_settings = DT_INST_PROP_OR(n, reset_settings, false),                               \
    },

static struct boot_key_config boot_keys[] = {DT_INST_FOREACH_STATUS_OKAY(BOOT_KEY_CONFIG)};

static bool boot_key_states[ARRAY_SIZE(boot_keys)][MAX_BOOT_COMBO_LEN] = {};

static int64_t timeout_uptime;

static int timeout_init(const struct device *device) {
    timeout_uptime = k_uptime_get() + CONFIG_ZMK_BOOT_MAGIC_COMBO_TIMEOUT_MS;
    return 0;
}

SYS_INIT(timeout_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static void trigger_boot_key(const struct boot_key_config *config) {
    if (config->reset_settings) {
        LOG_INF("Boot key: resetting settings");
        zmk_reset_settings();
    }

    if (config->jump_to_bootloader) {
        LOG_INF("Boot key: jumping to bootloader");
#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
        zmk_reset(BOOT_MODE_TYPE_BOOTLOADER);
#else
        zmk_reset(ZMK_RESET_BOOTLOADER);
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */
    } else if (config->reset_settings) {
        // If resetting settings but not jumping to bootloader, we need to reboot
        // to ensure all subsystems are properly reset.
#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
        zmk_reset(BOOT_MODE_TYPE_NORMAL);
#else
        zmk_reset(ZMK_RESET_WARM);
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */
    }
}

static int event_listener(const zmk_event_t *eh) {
    if (likely(k_uptime_get() > timeout_uptime)) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < ARRAY_SIZE(boot_keys); i++) {
        const struct boot_key_config *config = &boot_keys[i];
        for (int j = 0; j < config->combo_positions_len; j++) {
            if (ev->position == config->combo_positions[j]) {
                boot_key_states[i][j] = ev->state;
                if (ev->state) {
                    bool all_keys_pressed = true;
                    for (int k = 0; k < config->combo_positions_len; k++) {
                        if (!boot_key_states[i][k]) {
                            all_keys_pressed = false;
                            break;
                        }
                    }
                    if (all_keys_pressed) {
                        trigger_boot_key(config);
                    }
                }
                break;
            }
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(boot_magic_combo, event_listener);
ZMK_SUBSCRIPTION(boot_magic_combo, zmk_position_state_changed);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
