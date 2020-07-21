/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT alps_ec11

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "ec11.h"

LOG_MODULE_REGISTER(EC11, CONFIG_SENSOR_LOG_LEVEL);


static int ec11_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct ec11_data *drv_data = dev->driver_data;
	const struct ec11_config *drv_cfg = dev->config_info;
	u8_t val;
	u8_t delta;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_ROTATION);

	val = (gpio_pin_get(drv_data->a, drv_cfg->a_pin) << 1) | gpio_pin_get(drv_data->b, drv_cfg->b_pin);

	LOG_DBG("prev: %d, new: %d", drv_data->ab_state, val);

	switch(val | (drv_data->ab_state << 2)) {
		case 0b0001: case 0b0111: case 0b1110:
			LOG_DBG("+1");
			delta = 1;
			break;
		default:
			LOG_DBG("FIGURE IT OUT!");
			break;
	}

	LOG_DBG("Delta: %d", delta);


	// if (ec11_reg_read(drv_data, EC11_REG_TOBJ, &val) < 0) {
	// 	return -EIO;
	// }

	// if (val & EC11_DATA_INVALID_BIT) {
	// 	return -EIO;
	// }

	// drv_data->sample = arithmetic_shift_right((s16_t)val, 2);

	return 0;
}

static int ec11_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct ec11_data *drv_data = dev->driver_data;

	// s32_t uval;

	// if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
	// 	return -ENOTSUP;
	// }

	// uval = (s32_t)drv_data->sample * EC11_TEMP_SCALE;
	// val->val1 = uval / 1000000;
	// val->val2 = uval % 1000000;

	return 0;
}

static const struct sensor_driver_api ec11_driver_api = {
#ifdef CONFIG_EC11_TRIGGER
	.trigger_set = ec11_trigger_set,
#endif
	.sample_fetch = ec11_sample_fetch,
	.channel_get = ec11_channel_get,
};

int ec11_init(struct device *dev)
{
	struct ec11_data *drv_data = dev->driver_data;
	const struct ec11_config *drv_cfg = dev->config_info;

	LOG_DBG("resolution %d", drv_cfg->resolution);

	drv_data->a = device_get_binding(drv_cfg->a_label);
	if (drv_data->a == NULL) {
		LOG_ERR("Failed to get pointer to A GPIO device");
		return -EINVAL;
	}

	drv_data->b = device_get_binding(drv_cfg->b_label);
	if (drv_data->b == NULL) {
		LOG_ERR("Failed to get pointer to B GPIO device");
		return -EINVAL;
	}

#ifdef CONFIG_EC11_TRIGGER
	if (ec11_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	return 0;
}

struct ec11_data ec11_data;

const struct ec11_config ec11_cfg = {
	.a_label = DT_INST_GPIO_LABEL(0, a_gpios),
	.a_pin = DT_INST_GPIO_PIN(0, a_gpios),
	.a_flags = DT_INST_GPIO_FLAGS(0, a_gpios),
	.b_label = DT_INST_GPIO_LABEL(0, b_gpios),
	.b_pin = DT_INST_GPIO_PIN(0, b_gpios),
	.b_flags = DT_INST_GPIO_FLAGS(0, b_gpios),
	COND_CODE_0(DT_INST_NODE_HAS_PROP(0, resolution), (1), (DT_INST_PROP(0, resolution))),
};

DEVICE_AND_API_INIT(ec11, DT_INST_LABEL(0), ec11_init,
		    &ec11_data,
		    &ec11_cfg, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &ec11_driver_api);
