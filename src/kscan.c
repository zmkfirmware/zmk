/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/kscan.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(zmk_kscan, CONFIG_ZMK_KSCAN_LOG_LEVEL);

#include <zmk/keymap.h>
#include <zmk/handlers.h>

#define ZMK_KSCAN_EVENT_STATE_PRESSED 0
#define ZMK_KSCAN_EVENT_STATE_RELEASED 1

struct zmk_kscan_event
{
	u32_t row;
	u32_t column;
	u32_t state;
};

struct zmk_kscan_msg_processor
{
	struct k_work work;
} msg_processor;

K_MSGQ_DEFINE(zmk_kscan_msgq, sizeof(struct zmk_kscan_event), CONFIG_ZMK_KSCAN_EVENT_QUEUE_SIZE, 4);

static void zmk_kscan_callback(struct device *dev, u32_t row, u32_t column, bool pressed)
{
	struct zmk_kscan_event ev = {
		.row = row,
		.column = column,
		.state = (pressed ? ZMK_KSCAN_EVENT_STATE_PRESSED : ZMK_KSCAN_EVENT_STATE_RELEASED)};

	k_msgq_put(&zmk_kscan_msgq, &ev, K_NO_WAIT);
	k_work_submit(&msg_processor.work);
}

void zmk_kscan_process_msgq(struct k_work *item)
{
	struct zmk_kscan_event ev;

	while (k_msgq_get(&zmk_kscan_msgq, &ev, K_NO_WAIT) == 0)
	{
		bool pressed = (ev.state == ZMK_KSCAN_EVENT_STATE_PRESSED);
		zmk_key key = zmk_keymap_keycode_from_position(ev.row, ev.column);
		struct zmk_key_event kev = (struct zmk_key_event){.key = key, .pressed = pressed};

		LOG_DBG("Row: %d, col: %d, key: %lld, pressed: %s\n", ev.row, ev.column, key, (pressed ? "true" : "false"));
		zmk_handle_key(kev);
	}
}

int zmk_kscan_init(char *name)
{
	struct device *dev = device_get_binding(name);
	if (dev == NULL)
	{
		LOG_ERR("Failed to get the KSCAN device");
		return -EINVAL;
	}

	k_work_init(&msg_processor.work, zmk_kscan_process_msgq);

	kscan_config(dev, zmk_kscan_callback);
	kscan_enable_callback(dev);

	return 0;
}
