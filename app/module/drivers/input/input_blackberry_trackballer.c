/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT sparkfun_blackberry_trackballer

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define TRACKBALLER_DIRECTION_COUNT 4U
#define TRACKBALLER_AXIS_COUNT 2U
#define TRACKBALLER_AXIS_X 0U
#define TRACKBALLER_AXIS_Y 1U

#define TRACKBALLER_LEFT 0U
#define TRACKBALLER_RIGHT 1U
#define TRACKBALLER_UP 2U
#define TRACKBALLER_DOWN 3U

#define TRACKBALLER_STEP_FRAC_BITS 8U
#define TRACKBALLER_STEP_ONE (1U << TRACKBALLER_STEP_FRAC_BITS)
#define TRACKBALLER_CURVE_FRAC_BITS 16U
#define TRACKBALLER_CURVE_ONE (1ULL << TRACKBALLER_CURVE_FRAC_BITS)
#define TRACKBALLER_MAX_ACCELERATION_EXPONENT 4U

struct trackballer_config {
    struct gpio_dt_spec movement_gpios[TRACKBALLER_DIRECTION_COUNT];
    struct gpio_dt_spec button_gpio;
    uint16_t axis_x;
    uint16_t axis_y;
    uint16_t button_code;
    uint16_t report_period_ms;
    uint16_t debounce_interval_ms;
    uint8_t interpolation_frames;
    uint32_t minimum_pulse_interval_us;
    uint16_t acceleration_start_hz;
    uint16_t acceleration_max_hz;
    uint8_t acceleration_exponent;
    uint16_t minimum_step;
    uint16_t maximum_step;
};

struct trackballer_movement_callback {
    struct gpio_callback callback;
    const struct device *trackballer;
    uint8_t direction;
};

struct trackballer_data {
    const struct device *dev;
    struct trackballer_movement_callback movement_callbacks[TRACKBALLER_DIRECTION_COUNT];
    struct gpio_callback button_callback;
    struct k_work_delayable report_work;
    struct k_work_delayable button_work;
#ifdef CONFIG_PM_DEVICE
    struct k_work_sync report_work_sync;
    struct k_work_sync button_work_sync;
#endif
    struct k_spinlock lock;
    int64_t movement_q8[TRACKBALLER_AXIS_COUNT];
    uint8_t interpolation_frames_remaining[TRACKBALLER_AXIS_COUNT];
    int64_t last_direction_pulse_ticks[TRACKBALLER_DIRECTION_COUNT];
    int64_t last_pulse_ticks[TRACKBALLER_AXIS_COUNT];
    bool have_last_direction_pulse[TRACKBALLER_DIRECTION_COUNT];
    bool have_last_pulse[TRACKBALLER_AXIS_COUNT];
    bool button_pressed;
#ifdef CONFIG_PM_DEVICE
    atomic_t suspended;
#endif
};

static uint8_t trackballer_direction_axis(uint8_t direction) {
    return direction <= TRACKBALLER_RIGHT ? TRACKBALLER_AXIS_X : TRACKBALLER_AXIS_Y;
}

static int8_t trackballer_direction_sign(uint8_t direction) {
    return direction == TRACKBALLER_RIGHT || direction == TRACKBALLER_DOWN ? 1 : -1;
}

static uint32_t trackballer_step_q8(const struct trackballer_config *config,
                                    struct trackballer_data *data, uint8_t axis,
                                    int64_t now_ticks) {
    uint32_t minimum_q8 = (uint32_t)config->minimum_step << TRACKBALLER_STEP_FRAC_BITS;
    uint32_t maximum_q8 = (uint32_t)config->maximum_step << TRACKBALLER_STEP_FRAC_BITS;

    if (!data->have_last_pulse[axis]) {
        data->have_last_pulse[axis] = true;
        data->last_pulse_ticks[axis] = now_ticks;
        return minimum_q8;
    }

    int64_t delta_ticks = now_ticks - data->last_pulse_ticks[axis];
    data->last_pulse_ticks[axis] = now_ticks;

    if (delta_ticks <= 0) {
        return maximum_q8;
    }

    uint64_t delta_us = k_ticks_to_us_floor64((uint64_t)delta_ticks);
    if (delta_us == 0U) {
        return maximum_q8;
    }

    uint32_t frequency_hz = (uint32_t)(USEC_PER_SEC / delta_us);
    if (frequency_hz <= config->acceleration_start_hz) {
        return minimum_q8;
    }

    if (frequency_hz >= config->acceleration_max_hz) {
        return maximum_q8;
    }

    uint32_t frequency_span = config->acceleration_max_hz - config->acceleration_start_hz;
    uint32_t frequency_offset = frequency_hz - config->acceleration_start_hz;
    uint32_t step_span_q8 = maximum_q8 - minimum_q8;

    if (config->acceleration_exponent == 1U) {
        return minimum_q8 +
               (uint32_t)(((uint64_t)frequency_offset * step_span_q8) / frequency_span);
    }

    uint64_t normalized_q16 = DIV_ROUND_CLOSEST(
        (uint64_t)frequency_offset << TRACKBALLER_CURVE_FRAC_BITS, frequency_span);
    uint64_t curve_q16 = TRACKBALLER_CURVE_ONE;

    for (uint8_t i = 0U; i < config->acceleration_exponent; i++) {
        curve_q16 = DIV_ROUND_CLOSEST(curve_q16 * normalized_q16, TRACKBALLER_CURVE_ONE);
    }

    return minimum_q8 +
           (uint32_t)DIV_ROUND_CLOSEST(curve_q16 * step_span_q8, TRACKBALLER_CURVE_ONE);
}

static bool trackballer_is_suspended(struct trackballer_data *data) {
#ifdef CONFIG_PM_DEVICE
    return atomic_get(&data->suspended) != 0;
#else
    ARG_UNUSED(data);
    return false;
#endif
}

static bool trackballer_accept_direction_pulse(const struct trackballer_config *config,
                                               struct trackballer_data *data, uint8_t direction,
                                               int64_t now_ticks) {
    if (config->minimum_pulse_interval_us == 0U) {
        return true;
    }

    if (data->have_last_direction_pulse[direction]) {
        int64_t delta_ticks = now_ticks - data->last_direction_pulse_ticks[direction];

        if (delta_ticks <= 0 ||
            k_ticks_to_us_floor64((uint64_t)delta_ticks) < config->minimum_pulse_interval_us) {
            return false;
        }
    }

    data->have_last_direction_pulse[direction] = true;
    data->last_direction_pulse_ticks[direction] = now_ticks;
    return true;
}

static void trackballer_movement_interrupt(const struct device *gpio_dev,
                                           struct gpio_callback *callback, uint32_t pins) {
    struct trackballer_movement_callback *movement_callback =
        CONTAINER_OF(callback, struct trackballer_movement_callback, callback);
    const struct device *dev = movement_callback->trackballer;
    const struct trackballer_config *config = dev->config;
    struct trackballer_data *data = dev->data;
    uint8_t axis = trackballer_direction_axis(movement_callback->direction);
    int8_t sign = trackballer_direction_sign(movement_callback->direction);
    int64_t now_ticks = k_uptime_ticks();

    ARG_UNUSED(gpio_dev);
    ARG_UNUSED(pins);

    if (trackballer_is_suspended(data)) {
        return;
    }

    k_spinlock_key_t key = k_spin_lock(&data->lock);
    if (!trackballer_accept_direction_pulse(config, data, movement_callback->direction,
                                            now_ticks)) {
        k_spin_unlock(&data->lock, key);
        return;
    }

    uint32_t step_q8 = trackballer_step_q8(config, data, axis, now_ticks);
    data->movement_q8[axis] += (int64_t)sign * step_q8;
    data->interpolation_frames_remaining[axis] = config->interpolation_frames;
    k_spin_unlock(&data->lock, key);

    k_work_schedule(&data->report_work, K_MSEC(config->report_period_ms));
}

static bool trackballer_has_whole_step(int64_t value_q8) {
    return value_q8 <= -(int64_t)TRACKBALLER_STEP_ONE || value_q8 >= (int64_t)TRACKBALLER_STEP_ONE;
}

static int32_t trackballer_take_axis_value(struct trackballer_data *data, uint8_t axis) {
    int64_t value = data->movement_q8[axis] / TRACKBALLER_STEP_ONE;
    if (value == 0) {
        data->interpolation_frames_remaining[axis] = 0U;
        return 0;
    }

    uint8_t frames = MAX(data->interpolation_frames_remaining[axis], 1U);
    uint64_t magnitude = value < 0 ? (uint64_t)(-(value + 1)) + 1U : (uint64_t)value;
    int64_t interpolated = (int64_t)DIV_ROUND_UP(magnitude, frames);

    if (value < 0) {
        interpolated = -interpolated;
    }

    int32_t report_value = (int32_t)CLAMP(interpolated, (int64_t)INT16_MIN, (int64_t)INT16_MAX);

    data->movement_q8[axis] -= (int64_t)report_value * TRACKBALLER_STEP_ONE;
    if (data->interpolation_frames_remaining[axis] > 0U) {
        data->interpolation_frames_remaining[axis]--;
    }
    if (!trackballer_has_whole_step(data->movement_q8[axis])) {
        data->interpolation_frames_remaining[axis] = 0U;
    }

    return report_value;
}

static void trackballer_report_work(struct k_work *work) {
    struct k_work_delayable *delayable = k_work_delayable_from_work(work);
    struct trackballer_data *data = CONTAINER_OF(delayable, struct trackballer_data, report_work);
    const struct device *dev = data->dev;
    const struct trackballer_config *config = dev->config;
    int32_t x;
    int32_t y;

    if (trackballer_is_suspended(data)) {
        return;
    }

    k_spinlock_key_t key = k_spin_lock(&data->lock);
    x = trackballer_take_axis_value(data, TRACKBALLER_AXIS_X);
    y = trackballer_take_axis_value(data, TRACKBALLER_AXIS_Y);
    k_spin_unlock(&data->lock, key);

    if (x != 0 || y != 0) {
        int err = input_report_rel(dev, config->axis_x, x, false, K_FOREVER);
        if (err == 0) {
            err = input_report_rel(dev, config->axis_y, y, true, K_FOREVER);
        }

        if (err != 0) {
            LOG_ERR("Failed reporting trackball movement (%d)", err);
        }
    }

    key = k_spin_lock(&data->lock);
    bool more_movement = trackballer_has_whole_step(data->movement_q8[TRACKBALLER_AXIS_X]) ||
                         trackballer_has_whole_step(data->movement_q8[TRACKBALLER_AXIS_Y]);
    k_spin_unlock(&data->lock, key);

    if (more_movement && !trackballer_is_suspended(data)) {
        k_work_schedule(&data->report_work, K_MSEC(config->report_period_ms));
    }
}

static void trackballer_button_work(struct k_work *work) {
    struct k_work_delayable *delayable = k_work_delayable_from_work(work);
    struct trackballer_data *data = CONTAINER_OF(delayable, struct trackballer_data, button_work);
    const struct device *dev = data->dev;
    const struct trackballer_config *config = dev->config;

    if (trackballer_is_suspended(data)) {
        return;
    }

    int pressed = gpio_pin_get_dt(&config->button_gpio);
    if (pressed < 0) {
        LOG_ERR("Failed reading trackball button (%d)", pressed);
        return;
    }

    if ((bool)pressed == data->button_pressed) {
        return;
    }

    data->button_pressed = pressed;
    int err = input_report_key(dev, config->button_code, pressed, true, K_FOREVER);
    if (err != 0) {
        LOG_ERR("Failed reporting trackball button (%d)", err);
    }
}

static void trackballer_button_interrupt(const struct device *gpio_dev,
                                         struct gpio_callback *callback, uint32_t pins) {
    struct trackballer_data *data =
        CONTAINER_OF(callback, struct trackballer_data, button_callback);
    const struct trackballer_config *config = data->dev->config;

    ARG_UNUSED(gpio_dev);
    ARG_UNUSED(pins);

    if (!trackballer_is_suspended(data)) {
        k_work_reschedule(&data->button_work, K_MSEC(config->debounce_interval_ms));
    }
}

static int trackballer_configure_gpio(const struct device *dev, bool configure_callbacks) {
    const struct trackballer_config *config = dev->config;
    struct trackballer_data *data = dev->data;
    int err;

    for (uint8_t i = 0; i < TRACKBALLER_DIRECTION_COUNT; i++) {
        const struct gpio_dt_spec *gpio = &config->movement_gpios[i];

        if (!gpio_is_ready_dt(gpio)) {
            LOG_ERR("Movement GPIO controller %s is not ready", gpio->port->name);
            return -ENODEV;
        }

        err = gpio_pin_configure_dt(gpio, GPIO_INPUT);
        if (err != 0) {
            LOG_ERR("Failed configuring movement GPIO %u (%d)", i, err);
            return err;
        }

        if (configure_callbacks) {
            data->movement_callbacks[i].trackballer = dev;
            data->movement_callbacks[i].direction = i;
            gpio_init_callback(&data->movement_callbacks[i].callback,
                               trackballer_movement_interrupt, BIT(gpio->pin));
            err = gpio_add_callback(gpio->port, &data->movement_callbacks[i].callback);
            if (err != 0) {
                LOG_ERR("Failed adding movement GPIO callback %u (%d)", i, err);
                return err;
            }
        }

        err = gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_TO_ACTIVE);
        if (err != 0) {
            LOG_ERR("Failed enabling movement GPIO interrupt %u (%d)", i, err);
            return err;
        }
    }

    if (!gpio_is_ready_dt(&config->button_gpio)) {
        LOG_ERR("Button GPIO controller %s is not ready", config->button_gpio.port->name);
        return -ENODEV;
    }

    err = gpio_pin_configure_dt(&config->button_gpio, GPIO_INPUT);
    if (err != 0) {
        LOG_ERR("Failed configuring button GPIO (%d)", err);
        return err;
    }

    int pressed = gpio_pin_get_dt(&config->button_gpio);
    if (pressed < 0) {
        LOG_ERR("Failed reading initial button state (%d)", pressed);
        return pressed;
    }
    data->button_pressed = pressed;

    if (configure_callbacks) {
        gpio_init_callback(&data->button_callback, trackballer_button_interrupt,
                           BIT(config->button_gpio.pin));
        err = gpio_add_callback(config->button_gpio.port, &data->button_callback);
        if (err != 0) {
            LOG_ERR("Failed adding button GPIO callback (%d)", err);
            return err;
        }
    }

    err = gpio_pin_interrupt_configure_dt(&config->button_gpio, GPIO_INT_EDGE_BOTH);
    if (err != 0) {
        LOG_ERR("Failed enabling button GPIO interrupt (%d)", err);
        return err;
    }

    return 0;
}

#ifdef CONFIG_PM_DEVICE
static int trackballer_disable_interrupts(const struct device *dev) {
    const struct trackballer_config *config = dev->config;

    for (uint8_t i = 0; i < TRACKBALLER_DIRECTION_COUNT; i++) {
        int err = gpio_pin_interrupt_configure_dt(&config->movement_gpios[i], GPIO_INT_DISABLE);
        if (err != 0) {
            LOG_ERR("Failed disabling movement GPIO interrupt %u (%d)", i, err);
            return err;
        }
    }

    int err = gpio_pin_interrupt_configure_dt(&config->button_gpio, GPIO_INT_DISABLE);
    if (err != 0) {
        LOG_ERR("Failed disabling button GPIO interrupt (%d)", err);
    }
    return err;
}

static void trackballer_clear_motion(struct trackballer_data *data) {
    k_spinlock_key_t key = k_spin_lock(&data->lock);
    memset(data->movement_q8, 0, sizeof(data->movement_q8));
    memset(data->interpolation_frames_remaining, 0, sizeof(data->interpolation_frames_remaining));
    memset(data->last_direction_pulse_ticks, 0, sizeof(data->last_direction_pulse_ticks));
    memset(data->last_pulse_ticks, 0, sizeof(data->last_pulse_ticks));
    memset(data->have_last_direction_pulse, 0, sizeof(data->have_last_direction_pulse));
    memset(data->have_last_pulse, 0, sizeof(data->have_last_pulse));
    k_spin_unlock(&data->lock, key);
}
#endif

static int trackballer_init(const struct device *dev) {
    struct trackballer_data *data = dev->data;

    data->dev = dev;
    k_work_init_delayable(&data->report_work, trackballer_report_work);
    k_work_init_delayable(&data->button_work, trackballer_button_work);
#ifdef CONFIG_PM_DEVICE
    atomic_clear(&data->suspended);
#endif

    int err = trackballer_configure_gpio(dev, true);
    if (err != 0) {
        return err;
    }

    return 0;
}

#ifdef CONFIG_PM_DEVICE
static int trackballer_pm_action(const struct device *dev, enum pm_device_action action) {
    struct trackballer_data *data = dev->data;

    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND: {
        atomic_set(&data->suspended, 1);
        int err = trackballer_disable_interrupts(dev);
        k_work_cancel_delayable_sync(&data->report_work, &data->report_work_sync);
        k_work_cancel_delayable_sync(&data->button_work, &data->button_work_sync);
        trackballer_clear_motion(data);

        if (data->button_pressed) {
            const struct trackballer_config *config = dev->config;
            int release_err = input_report_key(dev, config->button_code, 0, true, K_FOREVER);

            data->button_pressed = false;
            if (release_err != 0) {
                LOG_ERR("Failed releasing trackball button during suspend (%d)", release_err);
                if (err == 0) {
                    err = release_err;
                }
            }
        }

        return err;
    }
    case PM_DEVICE_ACTION_RESUME: {
        trackballer_clear_motion(data);
        int err = trackballer_configure_gpio(dev, false);
        if (err != 0) {
            return err;
        }
        atomic_clear(&data->suspended);
        return 0;
    }
    default:
        return -ENOTSUP;
    }
}
#endif

#define TRACKBALLER_DEFINE(inst)                                                                   \
    BUILD_ASSERT(DT_INST_PROP(inst, report_period_ms) > 0,                                         \
                 "report-period-ms must be greater than zero");                                    \
    BUILD_ASSERT(DT_INST_PROP(inst, debounce_interval_ms) > 0,                                     \
                 "debounce-interval-ms must be greater than zero");                                \
    BUILD_ASSERT(DT_INST_PROP(inst, interpolation_frames) > 0,                                     \
                 "interpolation-frames must be greater than zero");                                \
    BUILD_ASSERT(DT_INST_PROP(inst, interpolation_frames) <= UINT8_MAX,                            \
                 "interpolation-frames must not exceed 255");                                      \
    BUILD_ASSERT(DT_INST_PROP(inst, minimum_pulse_interval_us) >= 0,                               \
                 "minimum-pulse-interval-us must not be negative");                                \
    BUILD_ASSERT(DT_INST_PROP(inst, acceleration_start_hz) > 0,                                    \
                 "acceleration-start-hz must be greater than zero");                               \
    BUILD_ASSERT(DT_INST_PROP(inst, acceleration_max_hz) >                                         \
                     DT_INST_PROP(inst, acceleration_start_hz),                                    \
                 "acceleration-max-hz must be greater than acceleration-start-hz");                \
    BUILD_ASSERT(DT_INST_PROP(inst, acceleration_exponent) > 0,                                    \
                 "acceleration-exponent must be greater than zero");                               \
    BUILD_ASSERT(DT_INST_PROP(inst, acceleration_exponent) <=                                      \
                     TRACKBALLER_MAX_ACCELERATION_EXPONENT,                                        \
                 "acceleration-exponent must not exceed 4");                                       \
    BUILD_ASSERT(DT_INST_PROP(inst, minimum_step) > 0, "minimum-step must be greater than zero");  \
    BUILD_ASSERT(DT_INST_PROP(inst, maximum_step) >= DT_INST_PROP(inst, minimum_step),             \
                 "maximum-step must be greater than or equal to minimum-step");                    \
    BUILD_ASSERT(DT_INST_PROP(inst, maximum_step) <= INT16_MAX,                                    \
                 "maximum-step must fit in a signed 16-bit HID movement value");                   \
    static const struct trackballer_config trackballer_config_##inst = {                           \
        .movement_gpios =                                                                          \
            {                                                                                      \
                GPIO_DT_SPEC_INST_GET(inst, left_gpios),                                           \
                GPIO_DT_SPEC_INST_GET(inst, right_gpios),                                          \
                GPIO_DT_SPEC_INST_GET(inst, up_gpios),                                             \
                GPIO_DT_SPEC_INST_GET(inst, down_gpios),                                           \
            },                                                                                     \
        .button_gpio = GPIO_DT_SPEC_INST_GET(inst, button_gpios),                                  \
        .axis_x = DT_INST_PROP(inst, zephyr_axis_x),                                               \
        .axis_y = DT_INST_PROP(inst, zephyr_axis_y),                                               \
        .button_code = DT_INST_PROP(inst, button_code),                                            \
        .report_period_ms = DT_INST_PROP(inst, report_period_ms),                                  \
        .debounce_interval_ms = DT_INST_PROP(inst, debounce_interval_ms),                          \
        .interpolation_frames = DT_INST_PROP(inst, interpolation_frames),                          \
        .minimum_pulse_interval_us = DT_INST_PROP(inst, minimum_pulse_interval_us),                \
        .acceleration_start_hz = DT_INST_PROP(inst, acceleration_start_hz),                        \
        .acceleration_max_hz = DT_INST_PROP(inst, acceleration_max_hz),                            \
        .acceleration_exponent = DT_INST_PROP(inst, acceleration_exponent),                        \
        .minimum_step = DT_INST_PROP(inst, minimum_step),                                          \
        .maximum_step = DT_INST_PROP(inst, maximum_step),                                          \
    };                                                                                             \
    static struct trackballer_data trackballer_data_##inst;                                        \
    PM_DEVICE_DT_INST_DEFINE(inst, trackballer_pm_action);                                         \
    DEVICE_DT_INST_DEFINE(inst, trackballer_init, PM_DEVICE_DT_INST_GET(inst),                     \
                          &trackballer_data_##inst, &trackballer_config_##inst, POST_KERNEL,       \
                          CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TRACKBALLER_DEFINE)
