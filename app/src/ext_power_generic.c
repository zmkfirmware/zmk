/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_ext_power_generic

#include <stdio.h>
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <settings/settings.h>
#include <drivers/gpio.h>
#include <drivers/ext_power.h>

#include <zmk/activity.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct ext_power_generic_config {
    const char *label;
    const uint8_t pin;
    const uint8_t flags;
    const uint16_t init_delay_ms;
};

struct ext_power_generic_data {
    const struct device *gpio;
    bool status;
#if IS_ENABLED(CONFIG_SETTINGS)
    bool settings_init;
#endif
#ifdef CONFIG_PM_DEVICE
    uint32_t pm_state;
#endif
};

#if IS_ENABLED(CONFIG_SETTINGS)
static void ext_power_save_state_work(struct k_work *work) {
    char setting_path[40];
    const struct device *ext_power = device_get_binding(DT_INST_LABEL(0));
    struct ext_power_generic_data *data = ext_power->data;

    snprintf(setting_path, 40, "ext_power/state/%s", DT_INST_LABEL(0));
    settings_save_one(setting_path, &data->status, sizeof(data->status));
}

static struct k_delayed_work ext_power_save_work;
#endif

int ext_power_save_state() {
#if IS_ENABLED(CONFIG_SETTINGS)
    k_delayed_work_cancel(&ext_power_save_work);
    return k_delayed_work_submit(&ext_power_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#else
    return 0;
#endif
}

static int ext_power_generic_enable(const struct device *dev) {
    struct ext_power_generic_data *data = dev->data;
    const struct ext_power_generic_config *config = dev->config;

    if (gpio_pin_set(data->gpio, config->pin, 1)) {
        LOG_WRN("Failed to set ext-power control pin");
        return -EIO;
    }
    data->status = true;
    return ext_power_save_state();
}

static int ext_power_generic_disable(const struct device *dev) {
    struct ext_power_generic_data *data = dev->data;
    const struct ext_power_generic_config *config = dev->config;

    if (gpio_pin_set(data->gpio, config->pin, 0)) {
        LOG_WRN("Failed to clear ext-power control pin");
        return -EIO;
    }
    data->status = false;
    return ext_power_save_state();
}

static int ext_power_generic_get(const struct device *dev) {
    struct ext_power_generic_data *data = dev->data;
    return data->status;
}

#if IS_ENABLED(CONFIG_SETTINGS)
static int ext_power_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                  void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, DT_INST_LABEL(0), &next) && !next) {
        const struct device *ext_power = device_get_binding(DT_INST_LABEL(0));
        struct ext_power_generic_data *data = ext_power->data;

        if (len != sizeof(data->status)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &data->status, sizeof(data->status));
        if (rc >= 0) {
            data->settings_init = true;

            if (ext_power == NULL) {
                LOG_ERR("Unable to retrieve ext_power device: %s", DT_INST_LABEL(0));
                return -EIO;
            }

            if (data->status) {
                ext_power_generic_enable(ext_power);
            } else {
                ext_power_generic_disable(ext_power);
            }

            return 0;
        }
        return rc;
    }
    return -ENOENT;
}

struct settings_handler ext_power_conf = {.name = "ext_power/state",
                                          .h_set = ext_power_settings_set};
#endif

#if IS_ENABLED(CONFIG_ZMK_USB_EXT_POWER_ONLY)
static int ext_power_toggle(const struct device *dev, bool enable) {
    // If our new state matches the power state, do nothing
    if (!enable == (ext_power_get(dev) > 0)) {
        return 0;
    }
    if (enable) {
        return ext_power_enable(dev);
    } else {
        return ext_power_disable(dev);
    }
}

static int ext_power_event_listener(const zmk_event_t *eh) {
    if (as_zmk_usb_conn_state_changed(eh)) {
        const struct device *ext_power = device_get_binding("EXT_POWER");
        return ext_power_toggle(ext_power, zmk_usb_is_powered());
    }

    return -ENOTSUP;
}

ZMK_LISTENER(ext_power, ext_power_event_listener);

ZMK_SUBSCRIPTION(ext_power, zmk_usb_conn_state_changed);
#endif

static int ext_power_generic_init(const struct device *dev) {
    struct ext_power_generic_data *data = dev->data;
    const struct ext_power_generic_config *config = dev->config;

    data->gpio = device_get_binding(config->label);
    if (data->gpio == NULL) {
        LOG_ERR("Failed to get ext-power control device");
        return -EINVAL;
    }

    if (gpio_pin_configure(data->gpio, config->pin, config->flags | GPIO_OUTPUT)) {
        LOG_ERR("Failed to configure ext-power control pin");
        return -EIO;
    }

#ifdef CONFIG_PM_DEVICE
    data->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif

#if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();

    int err = settings_register(&ext_power_conf);
    if (err) {
        LOG_ERR("Failed to register the ext_power settings handler (err %d)", err);
        return err;
    }

    k_delayed_work_init(&ext_power_save_work, ext_power_save_state_work);

    // Set default value (on) if settings isn't set
    settings_load_subtree("ext_power");
    if (!data->settings_init) {

        data->status = true;
        k_delayed_work_submit(&ext_power_save_work, K_NO_WAIT);

        ext_power_enable(dev);
    }
#elif IS_ENABLED(CONFIG_ZMK_USB_EXT_POWER_ONLY)
    ext_power_toggle(dev, zmk_usb_is_powered());
#else
    // Default to the ext_power being open when no settings
    ext_power_enable(dev);
#endif

    if (config->init_delay_ms) {
        k_msleep(config->init_delay_ms);
    }

    return 0;
}

#ifdef CONFIG_PM_DEVICE
static int ext_power_generic_pm_control(const struct device *dev, uint32_t ctrl_command,
                                        void *context, device_pm_cb cb, void *arg) {
    int rc;
    struct ext_power_generic_data *data = dev->data;

    switch (ctrl_command) {
    case DEVICE_PM_SET_POWER_STATE:
        if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
            data->pm_state = DEVICE_PM_ACTIVE_STATE;
            rc = 0;
        } else {
            ext_power_generic_disable(dev);
            data->pm_state = DEVICE_PM_LOW_POWER_STATE;
            rc = 0;
        }
        break;
    case DEVICE_PM_GET_POWER_STATE:
        *((uint32_t *)context) = data->pm_state;
        rc = 0;
        break;
    default:
        rc = -EINVAL;
    }

    if (cb != NULL) {
        cb(dev, rc, context, arg);
    }

    return rc;
}
#endif /* CONFIG_PM_DEVICE */

static const struct ext_power_generic_config config = {
    .label = DT_INST_GPIO_LABEL(0, control_gpios),
    .pin = DT_INST_GPIO_PIN(0, control_gpios),
    .flags = DT_INST_GPIO_FLAGS(0, control_gpios),
    .init_delay_ms = DT_INST_PROP_OR(0, init_delay_ms, 0)};

static struct ext_power_generic_data data = {
    .status = false,
#if IS_ENABLED(CONFIG_SETTINGS)
    .settings_init = false,
#endif
};

static const struct ext_power_api api = {.enable = ext_power_generic_enable,
                                         .disable = ext_power_generic_disable,
                                         .get = ext_power_generic_get};

#define ZMK_EXT_POWER_INIT_PRIORITY 81

DEVICE_DT_INST_DEFINE(0, ext_power_generic_init, &ext_power_generic_pm_control, &data, &config,
                      POST_KERNEL, ZMK_EXT_POWER_INIT_PRIORITY, &api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
