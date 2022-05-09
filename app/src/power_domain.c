/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <device.h>
#include <pm/device.h>
#include <pm/device_runtime.h>
#include <init.h>
#include <kernel.h>
#include <settings/settings.h>
#include <drivers/gpio.h>
#include <zmk/power_domain.h>
#include <zmk/events/activity_state_changed.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/*
 * Macros to get a list of power domains
 */

// Generates a count of power_domain_gpio devices
// For example, if you have 2 power domains, then `PD_COUNT()`, will generate:
// 0 + 1 + 1
// Which is 2 :)
#define _PD_COUNT_F(id)						   \
	+ 1
#define PD_COUNT() \
    0 DT_FOREACH_STATUS_OKAY(power_domain_gpio, _PD_COUNT_F)

// Generate the power_domains array definition.
// Should be used with DT_FOREACH_STATUS_OKAY.
#define GEN_PD_ARRAY(id)	\
    { \
        .pd = DEVICE_DT_GET(id), \
        .state_user_intended = false, \
        .settings_init = false, \
    },


/*
 * Global Variables
 */

// Struct that stores both power domain device pointers as well as whether
// the user desires for the domain to be on or off
struct zmk_power_domain_data {
    const struct device *pd;
    bool state_user_intended;
    bool settings_init;
};

// Count of available power domains
#define POWER_DOMAIN_COUNT PD_COUNT()

// Initialize array of structs with available power domains using above macro
struct zmk_power_domain_data pd_data_list[] = {
    DT_FOREACH_STATUS_OKAY(power_domain_gpio, GEN_PD_ARRAY)
};

// Default power domain. Set by chosen node `zmk,default-power-domain`
// and if not present to first initialized power domain.
struct zmk_power_domain_data *pd_data_default = NULL;

// Whether settings have been initialized yet
bool zmk_pd_settings_init = false;

/*
 * End-User Functions
 *
 * These functions call `pm_device_action_run()`...
 *   - which in turn calls the callback `zmk_power_domain_pm_action()`...
 *   - which then calls `zmk_power_domain_set_gpio_pin()`,
 *   - which actually sets the control pin that enables or disables the power.
 *
 * `pm_device_action_run()` also calls `_pm_action()` callbacks of the devices
 * assigned to the power-domain so that they can shutdown down and
 * re-initialize.
 */

const int zmk_power_domain_disable(const struct device *pd_dev, bool save_state) {
    return zmk_power_domain_run_action(pd_dev, ZMK_PD_ACTION_TURN_OFF, save_state);
}

const int zmk_power_domain_enable(const struct device *pd_dev, bool save_state) {
    return zmk_power_domain_run_action(pd_dev, ZMK_PD_ACTION_TURN_ON, save_state);
}

const int zmk_power_domain_toggle(const struct device *pd_dev, bool save_state) {
    return zmk_power_domain_run_action(pd_dev, ZMK_PD_ACTION_TOGGLE, save_state);
}

const int zmk_power_domain_get_state_actual(const struct device *pd_dev) {
    if(pd_dev == NULL) {
        pd_dev = pd_data_default->pd;
        if(pd_dev == NULL) {
            LOG_ERR("Could not get power domain state: Found now power domain.");
            return -1;
        }
    }

    enum pm_device_state pm_state;
    if(pm_device_state_get(pd_dev, &pm_state) != 0) {

        LOG_ERR("Could not get pm device state for power domain `%s`", pd_dev->name);
        return -1;
    }

    if(pm_state == PM_DEVICE_STATE_ACTIVE) {
        return 1;
    } else {
        return 0;
    }
}

const int zmk_power_domain_get_state_user_intended(const struct device *pd_dev) {

    if(pd_dev == NULL) {
        pd_dev = pd_data_default->pd;
        if(pd_dev == NULL) {
            return -1;
        }
    }
    struct zmk_power_domain_data *pd_data = zmk_power_domain_get_pd_data_for_pd(pd_dev);
    return (const int)pd_data->state_user_intended;
}


/*
 * Power Domain Run Action Helper Function
 */

const int zmk_power_domain_run_action(const struct device *pd_dev, enum zmk_power_domain_action zmk_action, bool save_state) {
    if(pd_dev == NULL) {
        pd_dev = pd_data_default->pd;
        if(pd_dev == NULL) {

            LOG_ERR("Could not run power domain action: Found no power domains.");
            return -1;
        }
    }

    LOG_DBG("In zmk_power_domain_run_action %s on pd `%s`.", zmk_pm_action_str(zmk_action), pd_dev->name);

    enum pm_device_state pm_state;
    if(pm_device_state_get(pd_dev, &pm_state) != 0) {
        LOG_ERR("Could not get pm device state for power domain `%s`", pd_dev->name);
        return -EIO;
    }

    LOG_DBG("Current pm_device_state of power domain `%s`: %s.", pd_dev->name, pm_device_state_str(pm_state));

    if(zmk_action == ZMK_PD_ACTION_TOGGLE) {
        if(pm_state == PM_DEVICE_STATE_ACTIVE) {
            zmk_action = ZMK_PD_ACTION_TURN_OFF;
        } else {
            zmk_action = ZMK_PD_ACTION_TURN_ON;
        }
    }

    enum pm_device_action zephyr_action;
    switch (zmk_action) {
        case ZMK_PD_ACTION_TURN_OFF:
            zephyr_action = PM_DEVICE_ACTION_TURN_OFF;
            break;

        case ZMK_PD_ACTION_TURN_ON:
            // Only PM_DEVICE_ACTION_RESUME is a valid option for turning on.
            // While PM_DEVICE_ACTION_TURN_ON may seem like the correct action,
            // it actually sets the power domain to state `suspended`, which
            // doesn't allow further ON / OFF actions.
            zephyr_action = PM_DEVICE_ACTION_RESUME;
            break;

        default:
            LOG_ERR("Action %d not implemented", zmk_action);
            return -ENOTSUP;
    }

    LOG_INF("Running pm_device_action %s on pd `%s`.", pm_device_action_str(zephyr_action), pd_dev->name);

    pm_device_action_run(pd_dev, zephyr_action);

    LOG_DBG("Finished running pm_device_action %s on pd `%s`.", pm_device_action_str(zephyr_action), pd_dev->name);

    if(save_state == true) {
        struct zmk_power_domain_data *pd_data = zmk_power_domain_get_pd_data_for_pd(pd_dev);
        pd_data->state_user_intended = (bool) zmk_action;

        return zmk_power_domain_save_state();
    }

    return 0;
}


/*
 * State Saving
 */
#if IS_ENABLED(CONFIG_SETTINGS)

static struct k_work_delayable zmk_power_domain_save_work;

static void zmk_power_domain_save_state_work(struct k_work *work) {
    LOG_DBG("");

    for(int i=0; i < POWER_DOMAIN_COUNT; i++) {
        struct zmk_power_domain_data *pd_data = &pd_data_list[i];

        char setting_path[40];
        snprintf(setting_path, sizeof(setting_path), "power_domain/state/%s", pd_data->pd->name);

        LOG_DBG("Saving `%d` to `%s`", pd_data->state_user_intended, setting_path);

        settings_save_one(setting_path, &pd_data->state_user_intended, sizeof(pd_data->state_user_intended));
    }
}
#endif

int zmk_power_domain_save_state() {
    LOG_DBG("");

#if IS_ENABLED(CONFIG_SETTINGS)
    int ret = k_work_reschedule(&zmk_power_domain_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
    return MIN(ret, 0);
#else
    return 0;
#endif
}

// This function is called when settings are loaded from flash by
// `settings_load_subtree`.
// It's called once for each power domain state that has been stored.
static int zmk_power_domain_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    LOG_DBG("Restoring settings for power domain %s", name);

    // Since we used `.name = "power_domain/state",` in `zmk_pd_settings_conf`,
    // the name variable contains only the last part of the key, which is the
    // name of the power domain.
    struct zmk_power_domain_data *pd_data = zmk_power_domain_get_pd_data_by_name(name);

    if(pd_data == NULL) {
        LOG_WRN("Could not restore state for power domain %s, because it does not exist in device tree.", name);
        return -EIO;
    }

    if (len != sizeof(pd_data->state_user_intended)) {
        LOG_WRN("Could not restore state for power domain %s, because the size of the value is invalid.", name);
        return -EINVAL;
    }

    int rc = read_cb(
        cb_arg,
        &pd_data->state_user_intended,
        sizeof(pd_data->state_user_intended)
    );
    if (rc >= 0) {
        LOG_INF("Restored state for power domain %s: %d", name, pd_data->state_user_intended);

        pd_data->settings_init = true;

        if(pd_data->state_user_intended == true) {
            zmk_power_domain_enable(pd_data->pd, false);
        } else {
            zmk_power_domain_disable(pd_data->pd, false);
        }

        return 0;
    }

    LOG_WRN("Could not restore state for power domain %s, because value could not be read: %d.", name, rc);
    return -ENOENT;
}

struct settings_handler zmk_pd_settings_conf = {
    .name = "power_domain/state",
    .h_set = zmk_power_domain_settings_set,
};

int zmk_power_domain_init_state_saving(const struct device *pd_dev) {
#if IS_ENABLED(CONFIG_SETTINGS)
    LOG_DBG("");

    settings_subsys_init();

    int err = settings_register(&zmk_pd_settings_conf);
    if (err) {
        LOG_ERR("Failed to register the power domain settings handler (err %d)", err);
        return err;
    }

    k_work_init_delayable(&zmk_power_domain_save_work, zmk_power_domain_save_state_work);

    // This will load the settings and then call
    // `zmk_power_domain_settings_set`, which will enable power
    settings_load_subtree("power_domain");

    // Here we check if any of the power domains didn't have a saved state
    // and enable them as the default action.
    for(int i=0; i < POWER_DOMAIN_COUNT; i++) {
        if(pd_data_list[i].settings_init != true) {
            LOG_INF("Found no existing settings for power domain %s. Setting it to enabled.", pd_data_list[i].pd->name);

            k_work_schedule(&zmk_power_domain_save_work, K_NO_WAIT);

            zmk_power_domain_enable(pd_data_list[i].pd, true);
            pd_data_list[i].settings_init = true;
        }
    }
#endif

    return 0;
}


/*
 * Init Functions
 */

// Zephyr's power_domain_gpio device is initialized at POST_KERNEL, 75.
//
// This function is ran right after that at POST_KERNEL, 76.
//
// Here we enable the power control pins so that power is available to all
// devices connected to the power domains during their initialization.
//
// This is important for some devices, such as the ssd1306 oled display driver.
// Without power, it will fail initialization.
//
// Once all devices are successfuly initialized,
// `zmk_power_domain_manager_post_boot_init` is run, which loads the user
// settings and turns on or off the power domains based on the user's
// preference.
//
// To catch log output add the following to your config:
// `CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS=4000`
static int zmk_power_domain_manager_init(const struct device *dev) {
    LOG_DBG("");

    if(POWER_DOMAIN_COUNT == 0) {
        LOG_WRN("No power domain devices configured. Exiting.");

        return -1;
    }

    zmk_power_domain_set_pd_data_default();

    for(int i=0; i < POWER_DOMAIN_COUNT; i++) {
        const struct device *pd = pd_data_list[i].pd;

        LOG_DBG("Turning on domain: %s",pd->name);
        zmk_power_domain_enable(pd, false);
    }

    return 0;
}

// power_domain_gpio inits at 75, so we init right after that
#define ZMK_POWER_DOMAIN_MANAGER_INIT_PRIORITY 76
SYS_INIT(zmk_power_domain_manager_init, POST_KERNEL, ZMK_POWER_DOMAIN_MANAGER_INIT_PRIORITY);


// This function is run after all other devices have initialized.
// Before this function is run `zmk_power_domain_init` is run, which activates
// the power domain's control pins to ensure all devices that depend on them
// have power to initialize.
//
// After they are all initialized, this function is run, which loads the user
// settings and enables or disables power depending on the last state before
// the boot.
static int zmk_power_domain_manager_post_boot_init(const struct device *dev) {
    LOG_DBG("");

    if(POWER_DOMAIN_COUNT == 0) {
        LOG_WRN("No power domain devices configured. Exiting.");

        return -1;
    }

#if IS_ENABLED(CONFIG_SETTINGS)

    // This will load the previous power state from the settings and then
    // enable or disable the power in `zmk_power_domain_settings_set`.
    zmk_power_domain_init_state_saving(dev);

#else
    LOG_INF("Settings are disabled. Setting all power domains to enabled");

    for(int i=0; i < POWER_DOMAIN_COUNT; i++) {
        const struct device *pd = pd_data_list[i].pd;

        LOG_DBG("Turning on domain: %s",pd->name);
        zmk_power_domain_enable(pd, true);
    }
#endif

    return 0;
}

// Run this post_boot_init function at the second lowest priority.
// This ensures this function is run after all other devices have been
// initialized, except for the dev_pm_policy devices, which must be run after
// this.
#define ZMK_POWER_DOMAIN_MANAGER_POST_BOOT_INIT_PRIORITY 98
SYS_INIT(zmk_power_domain_manager_post_boot_init, APPLICATION, ZMK_POWER_DOMAIN_MANAGER_POST_BOOT_INIT_PRIORITY);


/*
 * Activity Event Handler
 */

int zmk_power_domain_activity_event_handler(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    switch (ev->state) {
        case ZMK_ACTIVITY_ACTIVE:
            LOG_DBG("Device became active - Doing nothing.");
            break;
        case ZMK_ACTIVITY_IDLE:
            LOG_DBG("Device became idle - Doing nothing.");
            break;
        case ZMK_ACTIVITY_SLEEP:
            LOG_DBG("Device going to sleep - Disabling power.");

            for(int i=0; i < POWER_DOMAIN_COUNT; i++) {
                const struct device *pd_dev = pd_data_list[i].pd;

                zmk_power_domain_disable(pd_dev, false);
            }

            break;
        default:
            LOG_WRN("Unhandled activity state: %d", ev->state);
            return -EINVAL;
    }
    return 0;
}
ZMK_LISTENER(zmk_power_domain, zmk_power_domain_activity_event_handler);
ZMK_SUBSCRIPTION(zmk_power_domain, zmk_activity_state_changed);


/*
 * Helper functions
 */

int zmk_power_domain_init_power_domain_manager_helper(const struct device *dev, const struct device *pd_dev) {

    LOG_DBG("Setting `%s` power domain to `%s`", dev->name, pd_dev->name);

    // This function requires a zephyr fork with this commit:
    // https://github.com/zephyrproject-rtos/zephyr/commit/0b13b44a662a1baadd4ed370441fbe8a74b4d531
    //
    // It dynamically adds a power domain to a device.
    // This way the user doesn't need to manually create power domain
    // management device in the device tree.
    if(pm_device_power_domain_add(dev, pd_dev) != 0) {
        LOG_ERR("Could not set power domain of %s.", dev->name);
        return -1;
    }

    // Get the state of the actual power domain and then set
    // the state of the manager device to the same state.
    // We have to do this, because this device is initialized
    // after the power domain is initialized and runs it's actions
    int domain_state = zmk_power_domain_get_state_actual(pd_dev);
    if(domain_state == 0) {

        pm_device_action_run(dev, PM_DEVICE_ACTION_TURN_OFF);
    } else if(domain_state == 1) {

        // By default the power domain manager device is initialized as
        // on / active.
        // If we run RESUME, it will not run the callback, because it's
        // already in the on / active / resumed state.
        // So, first we set it to off without triggering the off action
        // and then we run the RESUME action.
        pm_device_runtime_init_off(dev);
        pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
    } else {
        LOG_ERR("Could not determine power domain state to correctly init power domain manager %s...", dev->name);
        return -1;
    }

    return 0;
}

const char *zmk_pm_action_str(enum zmk_power_domain_action action) {
	switch (action) {
        case ZMK_PD_ACTION_TURN_OFF:
            return "turn_off";
        case ZMK_PD_ACTION_TURN_ON:
            return "turn_on";
        case ZMK_PD_ACTION_TOGGLE:
            return "toggle";
        default:
            return "unknown_action";
	}
}

struct zmk_power_domain_data *zmk_power_domain_get_pd_data_for_pd(const struct device *pd_dev) {
    LOG_DBG("");

    struct zmk_power_domain_data *pd_data = NULL;

    for(int i=0; i < POWER_DOMAIN_COUNT; i++) {
        if(pd_dev == pd_data_list[i].pd) {
            pd_data = &pd_data_list[i];
            LOG_DBG("Found desired pd_data for power domain: %s", pd_data->pd->name);

            break;
        }
    }

    return pd_data;
}

struct zmk_power_domain_data *zmk_power_domain_get_pd_data_by_name(const char *name) {

    for(int i=0; i < POWER_DOMAIN_COUNT; i++) {
        if(strcmp(name, pd_data_list[i].pd->name) == 0) {
            LOG_DBG("Found PD for name `%s`: %p", name, &pd_data_list[i]);
            return &pd_data_list[i];
        }
    }

    return NULL;
}

void zmk_power_domain_set_pd_data_default() {
    const struct device *pd_dev_default = DEVICE_DT_GET(DEFAULT_POWER_DOMAIN);

    if(pd_dev_default != NULL) {
        pd_data_default = zmk_power_domain_get_pd_data_for_pd(pd_dev_default);
    } else {
        pd_data_default = &pd_data_list[0];

        LOG_ERR("Default power domain chosen node is not set. Setting to first power domain.");
    }

    LOG_INF("Set default power domain to: %s", pd_data_default->pd->name);
}
