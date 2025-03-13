#define DT_DRV_COMPAT zmk_behavior_indicators

#include <drivers/behavior.h>
#include <dt-bindings/zmk/indicators.h>
#include <zmk/behavior.h>
#include <zmk/events/indicators_changed.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zmk/split/bluetooth/central.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata std_values[] = {
    {
        .display_name = "Battery Status",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = BAT_ST,
    },
    {
        .display_name = "Realtime Indication ON",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = IND_ON,
    },
    {
        .display_name = "Realtime Indication OFF",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value = IND_OFF,
    },
};

static const struct behavior_parameter_metadata_set std_set = {
    .param1_values = std_values,
    .param1_values_len = ARRAY_SIZE(std_values),
};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = 1,
    .sets = &std_set,
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case BAT_ST:
#if (!CONFIG_ZMK_SPLIT || (CONFIG_ZMK_SPLIT && CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
        zmk_split_bt_call_bat_st_asked();
#endif
        uint8_t bat_level = bt_bas_get_battery_level();
        raise_zmk_indicators_battery_status_asked(
            (struct zmk_indicators_battery_status_asked){.level = bat_level});
        return 0;
    case IND_ON:
        raise_zmk_indicators_state_changed((struct zmk_indicators_state_changed){.state = 1});
        return 0;
    case IND_OFF:
        raise_zmk_indicators_state_changed((struct zmk_indicators_state_changed){.state = 0});
        return 0;
    }

    return -ENOTSUP;
}

static int behavior_indicators_init(const struct device *dev) { return 0; };

static const struct behavior_driver_api behavior_indicators_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_indicators_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_indicators_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
