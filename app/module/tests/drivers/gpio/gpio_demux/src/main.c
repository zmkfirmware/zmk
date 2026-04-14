/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/ztest.h>

// TODO: Import GPIO emul header!
//
#define DUT_NODE DT_NODELABEL(dut)

static const struct device *dut = DEVICE_DT_GET(DUT_NODE);

static const struct gpio_dt_spec en = GPIO_DT_SPEC_GET(DUT_NODE, en_gpios);

static const struct gpio_dt_spec sel0 = GPIO_DT_SPEC_GET_BY_IDX(DUT_NODE, select_gpios, 0);
static const struct gpio_dt_spec sel1 = GPIO_DT_SPEC_GET_BY_IDX(DUT_NODE, select_gpios, 1);
static const struct gpio_dt_spec sel2 = GPIO_DT_SPEC_GET_BY_IDX(DUT_NODE, select_gpios, 2);

ZTEST(gpio_demux, test_gpio_demux_init)
{
	zassert_true(device_is_ready(dut), "GPIO controller device is ready");
}

ZTEST(gpio_demux, test_gpio_demux_config_output_then_set_on)
{
	int ret;

	ret = gpio_pin_configure(dut, 0, GPIO_OUTPUT);

	zassert_ok(ret, "GPIO configured as output");

	ret = gpio_pin_configure(dut, 7, GPIO_OUTPUT);

	zassert_ok(ret, "GPIO configured as output");

	ret = gpio_pin_set(dut, 0, 1);
	zassert_ok(ret, "GPIO set logically high");

	// Zero pin should set all selects low, and enable pin active
	zassert_true(gpio_emul_output_get(sel0.port, sel0.pin) == 0, "Select pin 0 should be low for first pin active");
	zassert_true(gpio_emul_output_get(sel1.port, sel1.pin) == 0, "Select pin 1 should be low for first pin active");
	zassert_true(gpio_emul_output_get(sel2.port, sel2.pin) == 0, "Select pin 2 should be low for first pin active");

	zassert_true(gpio_emul_output_get(en.port, en.pin) > 0, "en pin should be active for any pin active");

	// Set the highest possible pin high
	ret = gpio_pin_set(dut, 0, 0);
	zassert_ok(ret, "GPIO set logically high");

	zassert_true(gpio_emul_output_get(en.port, en.pin) == 0, "en pin should be inactive for no active pin active");

	// Set the highest possible pin high
	ret = gpio_pin_set(dut, 7, 1);
	zassert_ok(ret, "GPIO set logically high");

	// Zero pin should set all selects low, and enable pin active
	zassert_true(gpio_emul_output_get(sel0.port, sel0.pin) == 1, "Select pin 0 should be low for first pin active");
	zassert_true(gpio_emul_output_get(sel1.port, sel1.pin) == 1, "Select pin 1 should be low for first pin active");
	zassert_true(gpio_emul_output_get(sel2.port, sel2.pin) == 1, "Select pin 2 should be low for first pin active");

	zassert_true(gpio_emul_output_get(en.port, en.pin) > 0, "en pin should be active for any pin active");
}

ZTEST(gpio_demux, test_gpio_demux_config_output_then_set_multiple_on)
{
	int ret;

	ret = gpio_pin_configure(dut, 0, GPIO_OUTPUT);
	zassert_ok(ret, "GPIO configured as output");

	ret = gpio_pin_configure(dut, 7, GPIO_OUTPUT);
	zassert_ok(ret, "GPIO configured as output");

	ret = gpio_pin_set(dut, 0, 1);
	zassert_ok(ret, "GPIO set logically high");
	ret = gpio_pin_set(dut, 7, 1);
	zassert_not_ok(ret, "GPIO not set logically high");
}

ZTEST(gpio_demux, test_gpio_demux_config_input)
{
	int ret;

	ret = gpio_pin_configure(dut, 0, GPIO_INPUT);
	zassert_not_ok(ret, "GPIO configured as input should not be allowed");
}

ZTEST(gpio_demux, test_gpio_demux_disconnect_active_pin)
{
	int ret;

	ret = gpio_pin_configure(dut, 0, GPIO_OUTPUT);
	zassert_ok(ret, "GPIO pin 0 configured as output");

	ret = gpio_pin_set(dut, 0, 1);
	zassert_ok(ret, "GPIO pin 0 set active");

	zassert_true(gpio_emul_output_get(en.port, en.pin) > 0, "en pin should be active for active setting a pin active");

	ret = gpio_pin_configure(dut, 0, GPIO_DISCONNECTED);
	zassert_ok(ret, "GPIO pin 0 configured as disconnected");

	zassert_true(gpio_emul_output_get(en.port, en.pin) == 0, "en pin should be inactive when disconnecting the active pin");
}

static void gpio_demux_teardown(void *unused)
{
	int ret;
	for (gpio_pin_t i = 0; i < 8; i++) {
		ret = gpio_pin_configure(dut, i, GPIO_DISCONNECTED);
		zassert_ok(ret, "Disconnected the pin successfully");
	}
}

/* Test GPIO port configuration */
ZTEST_SUITE(gpio_demux, NULL, NULL, NULL, gpio_demux_teardown, NULL);

