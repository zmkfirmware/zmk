/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_battery_voltage_divider

#include <device.h>
#include <drivers/kscan.h>
#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <math.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define VBATT DT_PATH(vbatt)

struct battery_config {
  struct device *adc;
  struct adc_channel_cfg acc;
  struct adc_sequence as;
  int16_t adc_raw;
};

static struct battery_config battery_config;

static int lithium_ion_mv_to_pct(int16_t bat_mv) {
  // Magic function that maps mV to this discharge graph from adafruit: https://learn.adafruit.com/li-ion-and-lipoly-batteries/voltages
 return round(106.818 + (-0.032685 - 106.818) / pow(1 + pow(bat_mv/3679.35, 58.979), 0.347386));
}

static void battery_read(struct k_work *workd) {
  struct battery_config *cfg = &battery_config;
  struct adc_sequence *as = &cfg->as;

  int rc = adc_read(cfg->adc, as);
  as->calibrate = false;
  if (rc == 0) {
    int32_t val = cfg->adc_raw;

    adc_raw_to_millivolts(adc_ref_internal(cfg->adc), cfg->acc.gain, as->resolution, &val);

    rc = val * (uint64_t)DT_PROP(VBATT, full_ohms) / DT_PROP(VBATT, output_ohms);
    LOG_DBG("ADC raw %d ~ %d mV => %d mV\n", cfg->adc_raw, val, rc);
    int percent = lithium_ion_mv_to_pct(rc);
    LOG_DBG("Percent: %d", percent);
  } else {
    LOG_DBG("Failed to read ADC: %d", rc);
  }
}

K_WORK_DEFINE(battery_work, battery_read);

static void battery_handler(struct k_timer *timer)
{
    k_work_submit(&battery_work);
}

K_TIMER_DEFINE(battery_tick, battery_handler, NULL);

static int battery_setup(struct device *_arg) {
  struct battery_config *cfg = &battery_config;
  struct adc_sequence *as = &cfg->as;
  struct adc_channel_cfg *acc = &cfg->acc;

  cfg->adc = device_get_binding(DT_IO_CHANNELS_LABEL(VBATT));

  if (cfg->adc == NULL) {
		LOG_ERR("ADC %s failed to retrieve", DT_IO_CHANNELS_LABEL(VBATT));
		return -ENOENT;
	}

  *as = (struct adc_sequence){
		.channels = BIT(0),
		.buffer = &cfg->adc_raw,
		.buffer_size = sizeof(cfg->adc_raw),
		.oversampling = 4,
		.calibrate = true,
	};

#ifdef CONFIG_ADC_NRFX_SAADC
	*acc = (struct adc_channel_cfg){
		.gain = ADC_GAIN_1_5,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
    .input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + DT_IO_CHANNELS_INPUT(VBATT),
	};

	as->resolution = 12;
#else
#error Unsupported ADC
#endif

	int adc_rc = adc_channel_setup(cfg->adc, acc);
	LOG_DBG("AIN%u setup returned %d", DT_IO_CHANNELS_INPUT(VBATT), adc_rc);

  if (adc_rc != 0) {
    return adc_rc;
  }

  k_timer_start(&battery_tick, K_NO_WAIT, K_SECONDS(5));

  return 0;
}

SYS_INIT(battery_setup,
        APPLICATION,
        CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */