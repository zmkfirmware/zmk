/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <bluetooth/conn.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/gpio.h>
#include <sys/sys_io.h>
#include <devicetree.h>

#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec _button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static const struct gpio_dt_spec led_one = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios,
						     {0});
static const struct gpio_dt_spec led_two = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios,
						     {0});

/**
 * Set up an LED DT Spec, default off
 */
void configure_led(struct gpio_dt_spec l) {
	int ret = 0;
	if (l.port && !device_is_ready(l.port)) {
		printk("Error %d: LED device %s is not ready; ignoring it\n",
		       ret, l.port->name);
		l.port = NULL;
	}
	if (l.port) {
		ret = gpio_pin_configure_dt(&l, (GPIO_OUTPUT | GPIO_OUTPUT_LOW));
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n",
			       ret, l.port->name, l.pin);
			l.port = NULL;
		} else {
			printk("Set up LED at %s pin %d\n", l.port->name, l.pin);
		}
	}
}

static struct gpio_callback button_cb_data;
/**
 * When the button is pressed, toggle an LED
 */
void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	gpio_pin_toggle_dt(&led_two);
}

/**
 * Configure a button to be clickable and callback to a method
 */
void configure_button(struct gpio_dt_spec button) {
	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return;
	}

	int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return;
	}
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

}

/**
 * When the BT is connected, turn on an LED
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
		int err = gpio_pin_set_dt(&led_one, 0);
		if (err) {
			printk("LED Set failed (err 0x%02x)\n", err);
		}
	} else {
		int err = gpio_pin_set_dt(&led_one, 1);
		if (err) {
			printk("LED Set failed (err 0x%02x)\n", err);
		}
		printk("Connected\n");
	}
}

/**
 * When the BT is disconnected, turn o ff an LED
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
		int err = gpio_pin_set_dt(&led_one, 0);
		if (err) {
			printk("LED Set failed (err 0x%02x)\n", err);
		}
	printk("Disconnected (reason 0x%02x)\n", reason);
}

// Configure the connection callbacks
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};


static int init(const struct device *port) {
	configure_led(led_one);
	configure_led(led_two);
	configure_button(_button);

	return 0;	
}

SYS_INIT(init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
