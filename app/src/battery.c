/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include <zmk/battery.h>

LOG_MODULE_REGISTER(BATTERY, CONFIG_ADC_LOG_LEVEL);

#define VBATT DT_PATH(vbatt)

#ifdef CONFIG_BOARD_THINGY52_NRF52832
/* This board uses a divider that reduces max voltage to
 * reference voltage (600 mV).
 */
#define BATTERY_ADC_GAIN ADC_GAIN_1
#else
/* Other boards may use dividers that only reduce battery voltage to
 * the maximum supported by the hardware (3.6 V)
 */
#define BATTERY_ADC_GAIN ADC_GAIN_1_6
#endif

struct io_channel_config {
	const char *label;
	u8_t channel;
};

struct gpio_channel_config {
	const char *label;
	u8_t pin;
	u8_t flags;
};

struct divider_config {
	struct io_channel_config io_channel;
	struct gpio_channel_config power_gpios;
	/* output_ohm is used as a flag value: if it is nonzero then
	 * the battery is measured through a voltage divider;
	 * otherwise it is assumed to be directly connected to Vdd.
	 */
	u32_t output_ohm;
	u32_t full_ohm;
};

static const struct divider_config divider_config = {
#if DT_NODE_HAS_STATUS(VBATT, okay)
	.io_channel = {
		DT_IO_CHANNELS_LABEL(VBATT),
		DT_IO_CHANNELS_INPUT(VBATT),
	},
#if DT_NODE_HAS_PROP(VBATT, power_gpios)
	.power_gpios = {
		DT_GPIO_LABEL(VBATT, power_gpios),
		DT_GPIO_PIN(VBATT, power_gpios),
		DT_GPIO_FLAGS(VBATT, power_gpios),
	},
#endif
	.output_ohm = DT_PROP(VBATT, output_ohms),
	.full_ohm = DT_PROP(VBATT, full_ohms),
#else /* /vbatt exists */
	.io_channel = {
		DT_LABEL(DT_ALIAS(adc_0)),
	},
#endif /* /vbatt exists */
};

struct divider_data {
	struct device *adc;
	struct device *gpio;
	struct adc_channel_cfg adc_cfg;
	struct adc_sequence adc_seq;
	s16_t raw;
};
static struct divider_data divider_data;

static int divider_setup(void)
{
	const struct divider_config *cfg = &divider_config;
	const struct io_channel_config *iocp = &cfg->io_channel;
	const struct gpio_channel_config *gcp = &cfg->power_gpios;
	struct divider_data *ddp = &divider_data;
	struct adc_sequence *asp = &ddp->adc_seq;
	struct adc_channel_cfg *accp = &ddp->adc_cfg;
	int rc;

	if (iocp->label == NULL) {
		return -ENOTSUP;
	}

	ddp->adc = device_get_binding(iocp->label);
	if (ddp->adc == NULL) {
		LOG_ERR("Failed to get ADC %s", iocp->label);
		return -ENOENT;
	}

	if (gcp->label) {
		ddp->gpio = device_get_binding(gcp->label);
		if (ddp->gpio == NULL) {
			LOG_ERR("Failed to get GPIO %s", gcp->label);
			return -ENOENT;
		}
		rc = gpio_pin_configure(ddp->gpio, gcp->pin,
					GPIO_OUTPUT_INACTIVE | gcp->flags);
		if (rc != 0) {
			LOG_ERR("Failed to control feed %s.%u: %d",
				gcp->label, gcp->pin, rc);
			return rc;
		}
	}

	*asp = (struct adc_sequence){
		.channels = BIT(0),
		.buffer = &ddp->raw,
		.buffer_size = sizeof(ddp->raw),
		.oversampling = 4,
		.calibrate = true,
	};

#ifdef CONFIG_ADC_NRFX_SAADC
	*accp = (struct adc_channel_cfg){
		.gain = BATTERY_ADC_GAIN,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
	};

	if (cfg->output_ohm != 0) {
		accp->input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0
			+ iocp->channel;
	} else {
		accp->input_positive = SAADC_CH_PSELP_PSELP_VDD;
	}

	asp->resolution = 14;
#else /* CONFIG_ADC_var */
#error Unsupported ADC
#endif /* CONFIG_ADC_var */

	rc = adc_channel_setup(ddp->adc, accp);
	LOG_INF("Setup AIN%u got %d", iocp->channel, rc);

	return rc;
}

static bool battery_ok;

static int battery_setup(struct device *arg)
{
	int rc = divider_setup();

	battery_ok = (rc == 0);
	LOG_INF("Battery setup: %d %d", rc, battery_ok);
	return rc;
}

SYS_INIT(battery_setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

int battery_measure_enable(bool enable)
{
	int rc = -ENOENT;

	if (battery_ok) {
		const struct divider_data *ddp = &divider_data;
		const struct gpio_channel_config *gcp = &divider_config.power_gpios;

		rc = 0;
		if (ddp->gpio) {
			rc = gpio_pin_set(ddp->gpio, gcp->pin, enable);
		}
	}
	return rc;
}

int battery_sample(void)
{
	int rc = -ENOENT;

	if (battery_ok) {
		struct divider_data *ddp = &divider_data;
		const struct divider_config *dcp = &divider_config;
		struct adc_sequence *sp = &ddp->adc_seq;

		rc = adc_read(ddp->adc, sp);
		sp->calibrate = false;
		if (rc == 0) {
			s32_t val = ddp->raw;

			adc_raw_to_millivolts(adc_ref_internal(ddp->adc),
					      ddp->adc_cfg.gain,
					      sp->resolution,
					      &val);

			if (dcp->output_ohm != 0) {
				rc = val * (u64_t)dcp->full_ohm
					/ dcp->output_ohm;
				LOG_INF("raw %u ~ %u mV => %d mV\n",
					ddp->raw, val, rc);
			} else {
				rc = val;
				LOG_INF("raw %u ~ %u mV\n", ddp->raw, val);
			}
		}
	}

	return rc;
}

unsigned int battery_level_pptt(unsigned int batt_mV,
				const struct battery_level_point *curve)
{
	const struct battery_level_point *pb = curve;

	if (batt_mV >= pb->lvl_mV) {
		/* Measured voltage above highest point, cap at maximum. */
		return pb->lvl_pptt;
	}
	/* Go down to the last point at or below the measured voltage. */
	while ((pb->lvl_pptt > 0)
	       && (batt_mV < pb->lvl_mV)) {
		++pb;
	}
	if (batt_mV < pb->lvl_mV) {
		/* Below lowest point, cap at minimum */
		return pb->lvl_pptt;
	}

	/* Linear interpolation between below and above points. */
	const struct battery_level_point *pa = pb - 1;

	return pb->lvl_pptt
	       + ((pa->lvl_pptt - pb->lvl_pptt)
		  * (batt_mV - pb->lvl_mV)
		  / (pa->lvl_mV - pb->lvl_mV));
}

/* was main.c */
/** A discharge curve specific to the power source. */
static const struct battery_level_point levels[] = {
#if DT_NODE_HAS_PROP(DT_INST(0, voltage_divider), io_channels)
	/* "Curve" here eyeballed from captured data for the [Adafruit
	 * 3.7v 2000 mAh](https://www.adafruit.com/product/2011) LIPO
	 * under full load that started with a charge of 3.96 V and
	 * dropped about linearly to 3.58 V over 15 hours.  It then
	 * dropped rapidly to 3.10 V over one hour, at which point it
	 * stopped transmitting.
	 *
	 * Based on eyeball comparisons we'll say that 15/16 of life
	 * goes between 3.95 and 3.55 V, and 1/16 goes between 3.55 V
	 * and 3.1 V.
	 */

	{ 10000, 3950 },
	{ 625, 3550 },
	{ 0, 3100 },
#else
	/* Linear from maximum voltage to minimum voltage. */
	{ 10000, 3600 },
	{ 0, 1700 },
#endif
};

static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	u32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
		 h, min, s, ms);
	return buf;
}

int zmk_log_battery_enable(void)
{
	int rc = battery_measure_enable(true);

	if (rc != 0) {
		printk("Failed initialize battery measurement: %d\n", rc);
		return -1;
	}

    return 0;
}

void zmk_log_battery_disable(void)
{
    printk("Disable: %d\n", battery_measure_enable(false));
}


void zmk_log_battery_sample(void)
{
    int batt_mV = battery_sample();

    if (batt_mV < 0) {
        printk("Failed to read battery voltage: %d\n",
               batt_mV);
        return;
    }

    unsigned int batt_pptt = battery_level_pptt(batt_mV, levels);

    printk("[%s]: %d mV; %u pptt\n", now_str(),
           batt_mV, batt_pptt);

    /* Burn battery so you can see that this works over time */
    k_busy_wait(5 * USEC_PER_SEC);
}
