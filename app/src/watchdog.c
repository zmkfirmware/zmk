/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

const struct device *wdog = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zmk_watchdog));

const struct wdt_timeout_cfg wdt_config = {
    /* Reset SOC when timer expires */
    .flags = WDT_FLAG_RESET_SOC,

    /* Setup max only, we do not want watchdog in windowed mode */
    .window.max = CONFIG_ZMK_WATCHDOG_TIMEOUT,
};

static int wdog_channel_id;

/* Feed function. Runs on a kernel timer. */
static void zmk_watchdog_feed(struct k_timer *timer) {

    (void)wdt_feed(wdog, wdog_channel_id);
    /* Feed watchdog */
    LOG_DBG("Fed hardware watchdog");
}

/* Define kernel timer */
K_TIMER_DEFINE(watchdog_timer, zmk_watchdog_feed, NULL);

int zmk_watchdog_init(void) {
    int ret;

    if (!device_is_ready(wdog)) {
        LOG_ERR("Watchdog not ready");
        return -ENODEV;
    }

    ret = wdt_install_timeout(wdog, &wdt_config);
    if (ret < 0) {
        LOG_ERR("Could not install watchdog timeout (%d)", ret);
        return ret;
    }
    /* Save channel ID if we installed timeout successfully */
    wdog_channel_id = ret;
    /* Setup WDT. After this point we must call wdt_feed periodically */
    ret = wdt_setup(wdog, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (ret < 0) {
        LOG_ERR("Could not setup watchdog timeout (%d)", ret);
    }

    /* Start watchdog feed timer, with period shorter than
     * the watchdog timeout
     */
    k_timer_start(&watchdog_timer, K_MSEC(CONFIG_ZMK_WATCHDOG_TIMEOUT - 100U),
                  K_MSEC(CONFIG_ZMK_WATCHDOG_TIMEOUT - 100U));
    return ret;
}

SYS_INIT(zmk_watchdog_init, APPLICATION, CONFIG_ZMK_WATCHDOG_INIT_PRIORITY);
