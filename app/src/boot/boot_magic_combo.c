/*
 * Copyright (c) 2026 The ZMK Contributors
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
#include <zephyr/drivers/hwinfo.h>

#include <zmk/reset.h>
#include <zmk/settings.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/boot_magic.h>
#include <zmk/physical_layouts.h>

#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
#include <zephyr/retention/bootmode.h>
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#endif /* IS_ENABLED(CONFIG_ZMK_BLE) */

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ALLOWED_RESET_CAUSE (RESET_PIN | RESET_HARDWARE)

#define BOOT_KEY_CONFIG(n)                                                                         \
    static const uint16_t boot_key_combo_positions_##n[] = DT_INST_PROP(n, combo_positions);       \
    static bool boot_key_state_##n[DT_INST_PROP_LEN(n, combo_positions)];                          \
    const struct zmk_boot_magic_combo_config ZMK_BOOT_MAGIC_COMBO_CONFIG_NAME(DT_DRV_INST(n)) = {  \
        .combo_positions = boot_key_combo_positions_##n,                                           \
        .combo_positions_len = DT_INST_PROP_LEN(n, combo_positions),                               \
        .jump_to_bootloader = DT_INST_PROP_OR(n, jump_to_bootloader, false),                       \
        .unpair_ble = DT_INST_PROP_OR(n, unpair_ble, false),                                       \
        .reset_settings = DT_INST_PROP_OR(n, reset_settings, false),                               \
        .state = boot_key_state_##n,                                                               \
    };

DT_INST_FOREACH_STATUS_OKAY(BOOT_KEY_CONFIG)

static int64_t timeout_uptime;

static int timeout_init(void) {
    timeout_uptime = k_uptime_get() + CONFIG_ZMK_BOOT_MAGIC_COMBO_TIMEOUT_MS;
    return 0;
}

SYS_INIT(timeout_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static void trigger_boot_key(const struct zmk_boot_magic_combo_config *config) {
    if (config->unpair_ble) {
        LOG_INF("Boot key: unpairing BLE");
#if IS_ENABLED(CONFIG_ZMK_BLE)
        zmk_ble_unpair_all();
#endif
    }

    if (config->reset_settings) {
        LOG_INF("Boot key: erasing settings");
        zmk_settings_erase();
    }

    if (config->jump_to_bootloader) {
        LOG_INF("Boot key: jumping to bootloader");
#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
        zmk_reset(BOOT_MODE_TYPE_BOOTLOADER);
#else
        zmk_reset(ZMK_RESET_BOOTLOADER);
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */
    } else if (config->unpair_ble || config->reset_settings) {
        // If unpairing BLE or erasing settings but not jumping to bootloader, we
        // need to reboot to ensure all subsystems are properly reset.
#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
        zmk_reset(BOOT_MODE_TYPE_NORMAL);
#else
        zmk_reset(ZMK_RESET_WARM);
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */
    }
}

static int event_listener(const zmk_event_t *eh) {
    // Boot combos only processed for CONFIG_ZMK_BOOT_MAGIC_COMBO_TIMEOUT_MS.
    if (likely(k_uptime_get() > timeout_uptime)) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // Ignore resets not caused by power cycle or reset button
    // I.e. if something else reset the keyboard, don't process boot combos
    // If hardware doesn't support reset causes (ENOSYS), skip the check.
    uint32_t cause;
    int result = hwinfo_get_reset_cause(&cause);
    if (result != -ENOSYS && (result < 0 || (result & ALLOWED_RESET_CAUSE) == 0)) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    int selected = zmk_physical_layouts_get_selected();

    // Ensure a physical layout is in use
    if (!ev || selected < 0) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_physical_layout *const *layouts;
    zmk_physical_layouts_get_list(&layouts);
    const struct zmk_physical_layout *active = layouts[selected];

    // Itereate through all combos and check if any are active
    for (int i = 0; i < active->boot_magic_combos_len; i++) {
        const struct zmk_boot_magic_combo_config *config = active->boot_magic_combos[i];
        for (int j = 0; j < config->combo_positions_len; j++) {
            if (ev->position == config->combo_positions[j]) {
                config->state[j] = ev->state;
                if (ev->state) {
                    bool all_keys_pressed = true;
                    for (int k = 0; k < config->combo_positions_len; k++) {
                        if (!config->state[k]) {
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
