/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/devicetree.h>

/* The PMU(BQ24075) enable pin(LOW active) on M60 is controlled by a NAND gate.
 * Partial reverse engineering shows:
 * P0.28 affects the NAND gate.
 * The button affects P0.27 the NAND gate.
 * P0.03 is detection pin for charging state(possibly attached to PMU LED pin).
 */

static const struct device *p0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

static inline void power_off(void) {
    gpio_pin_set(p0, 28, 0); // turn off the PMU battery path
}

static inline bool is_charging(void) {
    // 0: charging
    return gpio_pin_get_raw(p0, 3) == 0;
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    // wait the button stable at 0 so the keyboard won't be powered up accidentally
    k_sleep(K_SECONDS(1));

    if (!is_charging()) {
        power_off();
    }
}

static int pinmux_nrf52840_m2_init(void) {
    // back button
    // NOTE: if used to power off the keyboard, make sure the action is done
    // AFTER the button is released.
    //
    // To wake up the keyboard from sleep with this button, must use interrupt.
    // To avoid the callback triggered after keyboard woke up(powering off the
    // keyboard), must use GPIO_INT_EDGE_FALLING(trigger on button pressed).
    gpio_pin_interrupt_configure(p0, 27, GPIO_INT_EDGE_FALLING);

    // GPIO 0.28(LDO control) is already configured by bootloader,
    // so configuring it here is just an ensurance.
    gpio_pin_configure(p0, 28, GPIO_OUTPUT_ACTIVE | GPIO_PULL_UP | GPIO_OPEN_DRAIN);

    // this is the pin for charging detection
    gpio_pin_configure(p0, 3, GPIO_INPUT | GPIO_PULL_UP);

    // setup the interrupt handler to poweroff the keyboard
    static struct gpio_callback button_cb;
    gpio_init_callback(&button_cb, button_pressed, BIT(27));
    gpio_add_callback(p0, &button_cb);

    return 0;
}

SYS_INIT(pinmux_nrf52840_m2_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
