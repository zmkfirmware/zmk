/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/rgb_underglow/state.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct rgb_underglow_state state;
static struct rgb_underglow_state preserved_state;

#if IS_ENABLED(CONFIG_SETTINGS)
static int rgb_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, "state", &next) && !next) {
        if (len != sizeof(preserved_state)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &preserved_state, sizeof(preserved_state));
        if (rc >= 0) {
            return 0;
        }

        return rc;
    }

    return -ENOENT;
}

struct settings_handler rgb_conf = {.name = "rgb/underglow", .h_set = rgb_settings_set};

static void zmk_rgb_ug_save_state_work(struct k_work *_work) {
    settings_save_one("rgb/underglow/state", &preserved_state, sizeof(preserved_state));
}

static struct k_work_delayable underglow_save_work;
#endif

struct rgb_underglow_state *zmk_rgb_ug_get_state(void) { return &state; }
struct rgb_underglow_state *zmk_rgb_ug_get_save_state(void) { return &preserved_state; }

int zmk_rgb_ug_state_init(void) {
    state = default_rgb_settings;
    preserved_state = default_rgb_settings;
#if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();

    int err = settings_register(&rgb_conf);
    if (err) {
        LOG_ERR("Failed to register the ext_power settings handler (err %d)", err);
        return err;
    }

    k_work_init_delayable(&underglow_save_work, zmk_rgb_ug_save_state_work);

    settings_load_subtree("rgb/underglow");
#endif
    return 0;
}

int zmk_rgb_ug_save_state(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    int ret = k_work_reschedule(&underglow_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
    return MIN(ret, 0);
#else
    return 0;
#endif
}