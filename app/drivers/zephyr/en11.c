/*
 * Copyright (c) 2020 Peter Johanson
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT alps_en11

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "en11.h"

LOG_MODULE_REGISTER(EN11, CONFIG_SENSOR_LOG_LEVEL);

static int en11_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct en11_data *drv_data = dev->driver_data;
	u16_t val;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	// if (en11_reg_read(drv_data, EN11_REG_TOBJ, &val) < 0) {
	// 	return -EIO;
	// }

	// if (val & EN11_DATA_INVALID_BIT) {
	// 	return -EIO;
	// }

	// drv_data->sample = arithmetic_shift_right((s16_t)val, 2);

	return 0;
}

static int en11_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct en11_data *drv_data = dev->driver_data;
	// s32_t uval;

	// if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
	// 	return -ENOTSUP;
	// }

	// uval = (s32_t)drv_data->sample * EN11_TEMP_SCALE;
	// val->val1 = uval / 1000000;
	// val->val2 = uval % 1000000;

	return 0;
}

static const struct sensor_driver_api en11_driver_api = {
#ifdef CONFIG_EN11_TRIGGER
	.trigger_set = en11_trigger_set,
#endif
	.sample_fetch = en11_sample_fetch,
	.channel_get = en11_channel_get,
};

int en11_init(struct device *dev)
{
	struct en11_data *drv_data = dev->driver_data;
	const struct en11_config *drv_cfg = dev->config_info;

	LOG_DBG("");

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

#ifdef CONFIG_EN11_TRIGGER
	if (en11_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	return 0;
}

struct en11_data en11_data;

const struct en11_config en11_cfg = {
	.a_label = DT_INST_GPIO_LABEL(0, a_gpios),
	.a_pin = DT_INST_GPIO_PIN(0, a_gpios),
	.a_flags = DT_INST_GPIO_FLAGS(0, a_gpios),
	.b_label = DT_INST_GPIO_LABEL(0, b_gpios),
	.b_pin = DT_INST_GPIO_PIN(0, b_gpios),
	.b_flags = DT_INST_GPIO_FLAGS(0, b_gpios),
};

DEVICE_AND_API_INIT(en11, DT_INST_LABEL(0), en11_init,
		    &en11_data,
		    &en11_cfg, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &en11_driver_api);
