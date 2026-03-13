/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
#include <zmk/rgb_underglow/startup_mutex.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static K_MUTEX_DEFINE(startup_mutex);
bool starting_up = false;

bool set_starting_up(bool value) {
    if (k_mutex_lock(&startup_mutex, K_MSEC(300)) != 0) {
        LOG_WRN("Can't start startup sequence, mutex is locked");
        return false;
    }
    starting_up = value;
    int unlock = k_mutex_unlock(&startup_mutex);
    return true;
}

bool is_starting_up() {
    if (k_mutex_lock(&startup_mutex, K_MSEC(300)) != 0) {
        return true;
    } else {
        bool ret = starting_up;
        int unlock = k_mutex_unlock(&startup_mutex);
        return ret;
    }
}

bool start_startup() { return set_starting_up(true); }

void stop_startup() { return set_starting_up(false); }
