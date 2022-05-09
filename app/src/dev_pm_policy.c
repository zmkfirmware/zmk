/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zmk_dev_pm_policy

#include <device.h>
#include <devicetree.h>
#include <zmk/power_domain.h>
#include <zmk/usb.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// TODO: Add check if dev is power domain or device

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

// Device PM Policy config struct
struct zmk_dev_pm_policy_config {
	const struct device *device;
    const char *device_compatible;
    bool auto_off_on_idle;
    bool usb_auto_toggle;
};

// Array that contains all policy devices that we can loop through during
// events.
// Limited to 10 policies.
#define MAX_POLICY_NUM 10
int policy_count = 0;
const struct device *policies[MAX_POLICY_NUM];

/*
 * Enabling & Disabling of devices
 */

void zmk_dev_pm_policy_enable_device(const struct device *dev, bool save_state) {
    const struct zmk_dev_pm_policy_config *cfg = dev->config;

    LOG_DBG("Enabling device `%s` with compatible `%s`.", cfg->device->name, cfg->device_compatible);

    if(strcmp(cfg->device_compatible, "power-domain-gpio") == 0) {

        zmk_power_domain_enable(cfg->device, save_state);
    } else { // Regular Device

        pm_device_action_run(cfg->device, PM_DEVICE_ACTION_RESUME);
    }
}

void zmk_dev_pm_policy_disable_device(const struct device *dev, bool save_state) {
    const struct zmk_dev_pm_policy_config *cfg = dev->config;

    LOG_DBG("Disabling device `%s` with compatible `%s`.", cfg->device->name, cfg->device_compatible);

    if(strcmp(cfg->device_compatible, "power-domain-gpio") == 0) {

        zmk_power_domain_disable(cfg->device, save_state);
    } else { // Regular Device

        pm_device_action_run(cfg->device, PM_DEVICE_ACTION_TURN_OFF);
    }
}

const int zmk_dev_pm_policy_get_device_state(const struct device *dev) {
    const struct zmk_dev_pm_policy_config *cfg = dev->config;

    if(strcmp(cfg->device_compatible, "power-domain-gpio") == 0) {

        return zmk_power_domain_get_state_user_intended(cfg->device);
    } else { // Regular Device

        return 1;
    }
}

/*
 * USB Event Handler
 */

void zmk_dev_pm_policy_auto_toggle_usb(const struct device *dev) {
    LOG_DBG("Doing usb auto toggling for `%s`.", dev->name);

    if (zmk_usb_is_powered() == true) {
        LOG_INF("USB power was connected. Enabling external power for device `%s`.", dev->name);
        zmk_dev_pm_policy_enable_device(dev, true);

    } else {
        LOG_INF("USB power was removed. Disabling external power for device `%s`.", dev->name);
        zmk_dev_pm_policy_disable_device(dev, true);
    }
}

static int zmk_dev_pm_policy_usb_event_handler(const zmk_event_t *eh) {
    if (as_zmk_usb_conn_state_changed(eh)) {
        LOG_DBG("USB conn state changed: %d", zmk_usb_is_powered());

        for(int i=0; i < policy_count; i++) {
            const struct device *dev = policies[i];
            const struct zmk_dev_pm_policy_config *cfg = dev->config;

            if(cfg->usb_auto_toggle == true) {
                zmk_dev_pm_policy_auto_toggle_usb(dev);
            }
        }
    }

    return 0;
}
ZMK_LISTENER(zmk_dev_pm_policy_usb, zmk_dev_pm_policy_usb_event_handler);
ZMK_SUBSCRIPTION(zmk_dev_pm_policy_usb, zmk_usb_conn_state_changed);


/*
 * Activity Event Handler
 */

void zmk_dev_pm_policy_auto_activity_toggle(const struct device *dev, struct zmk_activity_state_changed *ev) {

    if(zmk_dev_pm_policy_get_device_state(dev) != 1) {
        // User has turned off the power domain.
        // Therefore we don't need want to do activity toggling.
        return;
    }

    const struct zmk_dev_pm_policy_config *cfg = dev->config;

    // Enable or disable the device / power domain, but don't save the new
    // state in the settings
    switch (ev->state) {
        case ZMK_ACTIVITY_ACTIVE:
            LOG_INF("Device became active - Enabling power for device `%s`.", cfg->device->name);
            zmk_dev_pm_policy_enable_device(dev, false);
            break;
        case ZMK_ACTIVITY_IDLE:
            LOG_INF("Device became idle - Disabling power for device `%s`.", cfg->device->name);
            zmk_dev_pm_policy_disable_device(dev, false);
            break;
        case ZMK_ACTIVITY_SLEEP:
            LOG_DBG("Device going to sleep - Doing nothing.");
            break;
        default:
            LOG_WRN("Unhandled activity state: %d", ev->state);
    }
}

int zmk_dev_pm_policy_activity_event_handler(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    LOG_DBG("Activity state changed to: %d", ev->state);

    for(int i=0; i < policy_count; i++) {
        const struct device *dev = policies[i];
        const struct zmk_dev_pm_policy_config *cfg = dev->config;

        if(cfg->auto_off_on_idle == true) {
            zmk_dev_pm_policy_auto_activity_toggle(dev, ev);
        }
    }

    return 0;
}
ZMK_LISTENER(zmk_dev_pm_policy_activity, zmk_dev_pm_policy_activity_event_handler);
ZMK_SUBSCRIPTION(zmk_dev_pm_policy_activity, zmk_activity_state_changed);


/*
 * Initialization
 */

static int zmk_dev_pm_policy_init(const struct device *dev)
{
	LOG_DBG("Initing zmk_dev_pm_policy_init: %s", dev->name);
	const struct zmk_dev_pm_policy_config *cfg = dev->config;

    if(policy_count < MAX_POLICY_NUM) {
        policies[policy_count] = dev;
        policy_count++;

        if(cfg->usb_auto_toggle == true) {
            zmk_dev_pm_policy_auto_toggle_usb(dev);
        }
    } else {
        LOG_ERR("Could not add dev_pm_policy `%s` to policies list, because the number of policies exceeds the maximum number of %d.", dev->name, MAX_POLICY_NUM);
    }

	return 0;
}

#define CONFIG_zmk_dev_pm_policy_INIT_PRIORITY 99
#define KP_INST(id) \
	static const struct zmk_dev_pm_policy_config zmk_dev_pm_policy_##id = { \
		.device = DEVICE_DT_GET(DT_INST_PHANDLE(id, device)), \
		.device_compatible = DT_PROP_BY_IDX(DT_INST_PHANDLE(id, device), compatible, 0), \
		.auto_off_on_idle = DT_INST_PROP(id, auto_off_on_idle), \
		.usb_auto_toggle = DT_INST_PROP(id, usb_auto_toggle), \
	}; \
    DEVICE_DT_INST_DEFINE( \
        id, \
        zmk_dev_pm_policy_init, \
        NULL, \
        NULL, \
        &zmk_dev_pm_policy_##id, \
        APPLICATION, \
        CONFIG_zmk_dev_pm_policy_INIT_PRIORITY, \
        NULL \
    );

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
