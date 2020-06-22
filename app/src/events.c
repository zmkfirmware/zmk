
#include <zephyr.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/events.h>
#include <sys/util.h>

#define DT_DRV_COMPAT zmk_behavior_global
#define GLOBAL_BEHAVIOR_LEN DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

#define LABEL_ENTRY(i) DT_INST_LABEL(i),
static const char *global_behaviors[] = {
    DT_INST_FOREACH_STATUS_OKAY(LABEL_ENTRY)
};

int zmk_events_position_pressed(u32_t position)
{
    for (int i = 0; i < GLOBAL_BEHAVIOR_LEN; i++) {
        const char* label = global_behaviors[i];
        struct device *dev = device_get_binding(label);
        behavior_position_pressed(dev, position);
    }
    return 0;
};

int zmk_events_position_released(u32_t position)
{
    for (int i = 0; i < GLOBAL_BEHAVIOR_LEN; i++) {
        const char* label = global_behaviors[i];
        struct device *dev = device_get_binding(label);
        behavior_position_released(dev, position);
    }
    return 0;
};

int zmk_events_keycode_pressed(u32_t keycode)
{
    for (int i = 0; i < GLOBAL_BEHAVIOR_LEN; i++) {
        const char* label = global_behaviors[i];
        struct device *dev = device_get_binding(label);
        behavior_keycode_pressed(dev, keycode);
    }
    return 0;
};

int zmk_events_keycode_released(u32_t keycode)
{
    for (int i = 0; i < GLOBAL_BEHAVIOR_LEN; i++) {
        const char* label = global_behaviors[i];
        struct device *dev = device_get_binding(label);
        behavior_keycode_released(dev, keycode);
    }
    return 0;
};

int zmk_events_modifiers_pressed(zmk_mod_flags modifiers)
{
    for (int i = 0; i < GLOBAL_BEHAVIOR_LEN; i++) {
        const char* label = global_behaviors[i];
        struct device *dev = device_get_binding(label);
        behavior_modifiers_pressed(dev, modifiers);
    }
    return 0;
};

int zmk_events_modifiers_released(zmk_mod_flags modifiers)
{
    for (int i = 0; i < GLOBAL_BEHAVIOR_LEN; i++) {
        const char* label = global_behaviors[i];
        struct device *dev = device_get_binding(label);
        behavior_modifiers_released(dev, modifiers);
    }
    return 0;
};

int zmk_events_consumer_key_pressed(u32_t usage)
{
    return 0;
};
int zmk_events_consumer_key_released(u32_t usage)
{
    return 0;
};
