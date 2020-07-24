/*
 * Copyright (c) 2020 Nick Winans <nick@winans.codes>
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <init.h>

#include <logging/log.h>

#include <drivers/led_strip.h>
#include <device.h>
#include <drivers/spi.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define STRIP_LABEL		DT_LABEL(DT_ALIAS(led_strip))
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)

#define DELAY_TIME K_MSEC(50)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct led_rgb colors[] = {
	RGB(0x0f, 0x00, 0x00), /* red */
	RGB(0x00, 0x0f, 0x00), /* green */
	RGB(0x00, 0x00, 0x0f), /* blue */
};

struct led_rgb pixels[STRIP_NUM_PIXELS];

static void zmk_rgb_underglow_start()
{
    struct device *strip;
	size_t cursor = 0, color = 0;
	int rc;

	strip = device_get_binding(STRIP_LABEL);
	if (strip) {
		LOG_INF("Found LED strip device %s", STRIP_LABEL);
	} else {
		LOG_ERR("LED strip device %s not found", STRIP_LABEL);
		return;
	}

	LOG_INF("Displaying pattern on strip");
	while (1) {
		memset(&pixels, 0x00, sizeof(pixels));
		memcpy(&pixels[cursor], &colors[color], sizeof(struct led_rgb));
		rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

		if (rc) {
			LOG_ERR("couldn't update strip: %d", rc);
		}

		cursor++;
		if (cursor >= STRIP_NUM_PIXELS) {
			cursor = 0;
			color++;
			if (color == ARRAY_SIZE(colors)) {
				color = 0;
			}
		}

		k_sleep(DELAY_TIME);
	}
}

static int zmk_rgb_underglow_init(struct device *_arg)
{
    zmk_rgb_underglow_start();

    return 0;
}

SYS_INIT(zmk_rgb_underglow_init,
        APPLICATION,
        CONFIG_APPLICATION_INIT_PRIORITY);
