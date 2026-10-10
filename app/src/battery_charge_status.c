/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_battery_charge_status

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/workqueue.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define HAS_FULL_GPIO DT_INST_NODE_HAS_PROP(0, full_gpios)

static const struct gpio_dt_spec charging_gpio = GPIO_DT_SPEC_INST_GET(0, charging_gpios);
static struct gpio_callback charging_callback;

#if HAS_FULL_GPIO
static const struct gpio_dt_spec full_gpio = GPIO_DT_SPEC_INST_GET(0, full_gpios);
static struct gpio_callback full_callback;
#endif

static enum zmk_battery_charge_state charge_state = ZMK_BATTERY_CHARGE_STATE_UNKNOWN;

enum zmk_battery_charge_state zmk_battery_charge_state(void) { return charge_state; }

static enum zmk_battery_charge_state read_charge_state(void) {
    int charging = gpio_pin_get_dt(&charging_gpio);
    if (charging < 0) {
        LOG_WRN("Failed to read charge status pin (%d)", charging);
        return ZMK_BATTERY_CHARGE_STATE_UNKNOWN;
    }

    if (charging) {
        return ZMK_BATTERY_CHARGE_STATE_CHARGING;
    }

#if HAS_FULL_GPIO
    int full = gpio_pin_get_dt(&full_gpio);
    if (full < 0) {
        LOG_WRN("Failed to read charge complete pin (%d)", full);
    } else if (full) {
        return ZMK_BATTERY_CHARGE_STATE_FULL;
    }
#endif

    // A single status pin cannot tell charge complete from no power applied.
    return ZMK_BATTERY_CHARGE_STATE_DISCHARGING;
}

static void charge_status_update(struct k_work *work) {
    ARG_UNUSED(work);

    enum zmk_battery_charge_state new_state = read_charge_state();
    if (new_state == charge_state) {
        return;
    }

    LOG_DBG("Battery charge state changed from %d to %d", charge_state, new_state);
    charge_state = new_state;

    // Publish via a battery sample, so the state is paired with a real level.
    zmk_battery_update_now();
}

static K_WORK_DELAYABLE_DEFINE(charge_status_work, charge_status_update);

static void charge_status_gpio_callback(const struct device *port, struct gpio_callback *cb,
                                        gpio_port_pins_t pins) {
    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &charge_status_work,
                                K_MSEC(DT_INST_PROP(0, debounce_ms)));
}

static int configure_status_gpio(const struct gpio_dt_spec *gpio, struct gpio_callback *callback) {
    if (!gpio_is_ready_dt(gpio)) {
        LOG_ERR("Charge status GPIO is not ready");
        return -ENODEV;
    }

    int rc = gpio_pin_configure_dt(gpio, GPIO_INPUT);
    if (rc < 0) {
        LOG_ERR("Failed to configure charge status pin (%d)", rc);
        return rc;
    }

    rc = gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_BOTH);
    if (rc < 0) {
        LOG_ERR("Failed to configure charge status interrupt (%d)", rc);
        return rc;
    }

    gpio_init_callback(callback, charge_status_gpio_callback, BIT(gpio->pin));
    return gpio_add_callback(gpio->port, callback);
}

static int zmk_battery_charge_status_init(void) {
    int rc = configure_status_gpio(&charging_gpio, &charging_callback);
    if (rc < 0) {
        return rc;
    }

#if HAS_FULL_GPIO
    rc = configure_status_gpio(&full_gpio, &full_callback);
    if (rc < 0) {
        return rc;
    }
#endif

    // Leave the state unknown so the work reports whatever we are doing at boot.
    k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &charge_status_work,
                                K_MSEC(DT_INST_PROP(0, debounce_ms)));

    return 0;
}

SYS_INIT(zmk_battery_charge_status_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
