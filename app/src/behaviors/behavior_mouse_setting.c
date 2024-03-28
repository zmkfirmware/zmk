#define DT_DRV_COMPAT zmk_behavior_mouse_setting

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <dt-bindings/zmk/mouse_settings.h>
#include <zmk/input_mouse_ps2.h>
#include <zmk/mouse.h>

#define INCREMENT_TP_SENSITIVITY 10
#define INCREMENT_TP_NEG_INERTIA 1
#define INCREMENT_TP_VALUE6 5
#define INCREMENT_TP_PTS_THRESHOLD 1

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {

    case MS_LOG:
        return zmk_mouse_ps2_settings_log();
    case MS_RESET:
        return zmk_mouse_ps2_settings_reset();
    case MS_TP_SENSITIVITY_INCR:
        return zmk_mouse_ps2_tp_sensitivity_change(INCREMENT_TP_SENSITIVITY);
    case MS_TP_SENSITIVITY_DECR:
        return zmk_mouse_ps2_tp_sensitivity_change(-INCREMENT_TP_SENSITIVITY);

    case MS_TP_NEG_INERTIA_INCR:
        return zmk_mouse_ps2_tp_neg_inertia_change(INCREMENT_TP_NEG_INERTIA);
    case MS_TP_NEG_INERTIA_DECR:
        return zmk_mouse_ps2_tp_neg_inertia_change(-INCREMENT_TP_NEG_INERTIA);

    case MS_TP_VALUE6_INCR:
        return zmk_mouse_ps2_tp_value6_upper_plateau_speed_change(INCREMENT_TP_VALUE6);
    case MS_TP_VALUE6_DECR:
        return zmk_mouse_ps2_tp_value6_upper_plateau_speed_change(-INCREMENT_TP_VALUE6);

    case MS_TP_PTS_THRESHOLD_INCR:
        return zmk_mouse_ps2_tp_pts_threshold_change(INCREMENT_TP_PTS_THRESHOLD);
    case MS_TP_PTS_THRESHOLD_DECR:
        return zmk_mouse_ps2_tp_pts_threshold_change(-INCREMENT_TP_PTS_THRESHOLD);
    }

    return -ENOTSUP;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

// Initialization Function
static int zmk_behavior_mouse_setting_init(const struct device *dev) { return 0; };

static const struct behavior_driver_api zmk_behavior_mouse_setting_driver_api = {
    .binding_pressed = on_keymap_binding_pressed, .binding_released = on_keymap_binding_released};

BEHAVIOR_DT_INST_DEFINE(0, zmk_behavior_mouse_setting_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &zmk_behavior_mouse_setting_driver_api);
