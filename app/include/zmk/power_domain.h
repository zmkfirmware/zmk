/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <pm/device.h>

#pragma once

#if CONFIG_ZMK_EXT_POWER

const int zmk_power_domain_disable(const struct device *pd_dev, bool save_state);
const int zmk_power_domain_enable(const struct device *pd_dev, bool save_state);
const int zmk_power_domain_toggle(const struct device *pd_dev, bool save_state);


/*
 * What's the difference between get_state_actual and get_state_user_intended?
 *
 * We distinguish whether power is actually on and whether the user wants the
 * power to be on.
 *
 * For example, when the user turns on the power, the user intended state is
 * ON, bt internal features may despite that turn the power off temporarily.
 *
 * For example, the auto-off-on-idle feature cuts the power on idle and changes
 * the actual state to on, but the intended state stays as on. When activity
 * resumes, it checks whether the user wants the power to be on, and if yes,
 * enables the actual power again.
 *
 */
const int zmk_power_domain_get_state_actual(const struct device *pd_dev);
const int zmk_power_domain_get_state_user_intended(const struct device *pd_dev);


/*
 * Power Domain manager helper function
 *
 * In order to receive power domain notifications in ZMK devices, such as the
 * display and rgb_underglow, we need to create a power domain manager device
 * that has a power domain configured.
 *
 * This sets the power domain to a dynamically created device and syncs the
 * state of the device to the power domain.
 */
int zmk_power_domain_init_power_domain_manager_helper(const struct device *dev, const struct device *pd_dev);

/**
 * @cond INTERNAL_HIDDEN
 *
 * Internal helper functions
 */

#define DEFAULT_POWER_DOMAIN DT_CHOSEN(zmk_default_power_domain)

/** @brief ZMK Power Domain Actions. */
enum zmk_power_domain_action {
	/** Turn the Power Domain off. */
	ZMK_PD_ACTION_TURN_OFF,
	/** Turn the Power Domain on. */
	ZMK_PD_ACTION_TURN_ON,
	/** Toggle the Power Domain*/
	ZMK_PD_ACTION_TOGGLE,
};
const int zmk_power_domain_run_action(const struct device *pd_dev, enum zmk_power_domain_action zmk_action, bool save_state);
int zmk_power_domain_save_state();

const struct device * zmk_power_domain_get_default();
struct zmk_power_domain_data *zmk_power_domain_get_pd_data_for_pd(const struct device *pd_dev);
struct zmk_power_domain_data *zmk_power_domain_get_pd_data_by_name(const char *name);
void zmk_power_domain_set_pd_data_default();
const char *zmk_pm_device_action_str(enum pm_device_action action);
const char *zmk_pm_action_str(enum zmk_power_domain_action action);

/**
 * @endcond
 */

#else

const inline int zmk_power_domain_disable(const struct device *pd_dev, bool save_state) { return 0; };
const inline int zmk_power_domain_enable(const struct device *pd_dev, bool save_state) { return 0; };
const inline int zmk_power_domain_toggle(const struct device *pd_dev, bool save_state) { return 0; };
const inline int zmk_power_domain_get_state_actual(const struct device *pd_dev) { return 0; };
const inline int zmk_power_domain_get_state_user_intended(const struct device *pd_dev) { return 0; };
inline int zmk_power_domain_init_power_domain_manager_helper(const struct device *dev, const struct device *pd_dev) { return 0; };

#endif
