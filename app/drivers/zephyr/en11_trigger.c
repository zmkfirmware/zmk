/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT alps_en11

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>

#include "en11.h"

extern struct en11_data en11_driver;

#include <logging/log.h>
LOG_MODULE_DECLARE(EN11, CONFIG_SENSOR_LOG_LEVEL);

static inline void setup_int(struct device *dev,
			     bool enable)
{
	struct en11_data *data = dev->driver_data;
	const struct en11_config *cfg = dev->config_info;

	LOG_DBG("enabled %s", (enable ? "true" : "false"));

	if (gpio_pin_interrupt_configure(data->a,
				     cfg->a_pin,
				     enable
				     ? GPIO_INT_EDGE_BOTH
				     : GPIO_INT_DISABLE)) {
						 LOG_WRN("Unable to set A pin GPIO interrupt");
					 }

	if (gpio_pin_interrupt_configure(data->b,
				     cfg->b_pin,
				     enable
				     ? GPIO_INT_EDGE_BOTH
				     : GPIO_INT_DISABLE)) {
						 LOG_WRN("Unable to set A pin GPIO interrupt");
					 }
}

static void en11_a_gpio_callback(struct device *dev,
				 struct gpio_callback *cb, u32_t pins)
{
	struct en11_data *drv_data =
		CONTAINER_OF(cb, struct en11_data, a_gpio_cb);

	LOG_DBG("");

	setup_int(drv_data->dev, false);

#if defined(CONFIG_EN11_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_EN11_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void en11_b_gpio_callback(struct device *dev,
				 struct gpio_callback *cb, u32_t pins)
{
	struct en11_data *drv_data =
		CONTAINER_OF(cb, struct en11_data, b_gpio_cb);

	LOG_DBG("");

	setup_int(drv_data->dev, false);

#if defined(CONFIG_EN11_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_EN11_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void en11_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct en11_data *drv_data = dev->driver_data;
	const struct en11_config *cfg = dev->config_info;
	u16_t status;

	// gpio_pin_get(drv_data->a, cfg->a_pin)

	// if (en11_reg_read(drv_data, EN11_REG_STATUS, &status) < 0) {
	// 	return;
	// }

	// if (status & EN11_DATA_READY_INT_BIT &&
	//     drv_data->drdy_handler != NULL) {
	// 	drv_data->drdy_handler(dev, &drv_data->drdy_trigger);
	// }

	// if (status & EN11_TOBJ_TH_INT_BITS &&
	//     drv_data->th_handler != NULL) {
	// 	drv_data->th_handler(dev, &drv_data->th_trigger);
	// }

	setup_int(dev, true);
}

#ifdef CONFIG_EN11_TRIGGER_OWN_THREAD
static void en11_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct en11_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		en11_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_EN11_TRIGGER_GLOBAL_THREAD
static void en11_work_cb(struct k_work *work)
{
	struct en11_data *drv_data =
		CONTAINER_OF(work, struct en11_data, work);

	LOG_DBG("");

	en11_thread_cb(drv_data->dev);
}
#endif

int en11_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct en11_data *drv_data = dev->driver_data;

	setup_int(dev, false);

	drv_data->trigger = *trig;
	drv_data->handler = handler;

	setup_int(dev, true);

	return 0;
}

int en11_init_interrupt(struct device *dev)
{
	struct en11_data *drv_data = dev->driver_data;
	const struct en11_config *drv_cfg = dev->config_info;

	drv_data->dev = dev;
	/* setup gpio interrupt */

	LOG_DBG("A: %s %d B: %s %d", drv_cfg->a_label, drv_cfg->a_pin, drv_cfg->b_label, drv_cfg->b_pin);
	
	if (gpio_pin_configure(drv_data->a, drv_cfg->a_pin,
			   drv_cfg->a_flags
			   | GPIO_INPUT)) {
		LOG_DBG("Failed to configure B pin");
		return -EIO;
	}

	gpio_init_callback(&drv_data->a_gpio_cb,
			   en11_a_gpio_callback,
			   BIT(drv_cfg->a_pin));

	if (gpio_add_callback(drv_data->a, &drv_data->a_gpio_cb) < 0) {
		LOG_DBG("Failed to set A callback!");
		return -EIO;
	}

	if (gpio_pin_configure(drv_data->b, drv_cfg->b_pin,
			   drv_cfg->b_flags
			   | GPIO_INPUT)) {
		LOG_DBG("Failed to configure B pin");
		return -EIO;
	}

	gpio_init_callback(&drv_data->b_gpio_cb,
			   en11_b_gpio_callback,
			   BIT(drv_cfg->b_pin));

	if (gpio_add_callback(drv_data->b, &drv_data->b_gpio_cb) < 0) {
		LOG_DBG("Failed to set B callback!");
		return -EIO;
	}

	LOG_DBG("A Pin? %d, B Pin? %d", gpio_pin_get(drv_data->a, drv_cfg->a_pin), gpio_pin_get(drv_data->b, drv_cfg->b_pin));

#if defined(CONFIG_EN11_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_EN11_THREAD_STACK_SIZE,
			(k_thread_entry_t)en11_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_EN11_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_EN11_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = en11_work_cb;
#endif

	return 0;
}
