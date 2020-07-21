/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <settings/settings.h>
#include <drivers/sensor.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix.h>
#include <zmk/kscan.h>
#include <zmk/endpoints.h>

#define ZMK_KSCAN_DEV DT_LABEL(ZMK_MATRIX_NODE_ID)

static struct sensor_trigger trigger;

void encoder_change(struct device *dev, struct sensor_trigger *trigger)
{
	int err;
	struct sensor_value value;

	LOG_DBG("");

	err = sensor_sample_fetch(dev);
	if (err) {
		LOG_ERR("Failed to fetch new EC11 sample");
		return;
	}

	err = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &value);
	if (err) {
		LOG_DBG("Failed to get the sample for EC11 %d", err);
		return;
	}
}

void init_sensor()
{
	struct device *dev = device_get_binding("Rotary Encoder");
	if (!dev) {
		LOG_DBG("NO ENCODER!");
		return;
	}

	trigger.type = SENSOR_TRIG_DATA_READY;
	trigger.chan = SENSOR_CHAN_ROTATION;

	sensor_trigger_set(dev, &trigger, encoder_change);
}

void main(void)
{
	printk("Welcome to ZMK!\n");

	if (zmk_kscan_init(ZMK_KSCAN_DEV) != 0)
	{
		return;
	}


#ifdef CONFIG_SETTINGS
	settings_load();
#endif

	init_sensor();
}
