
#include <zephyr.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/events.h>
#include <sys/util.h>

#define BINDINGS_NODE DT_CHOSEN(zmk_global_bindings)
#define BINDING_COUNT DT_PROP_LEN(BINDINGS_NODE, bindings)

#define BINDING_GEN(idx,_) \
    { .behavior_dev = DT_LABEL(DT_PHANDLE_BY_IDX(BINDINGS_NODE, bindings, idx)), \
	  .param1 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(BINDINGS_NODE, bindings, idx, param1), (0), (DT_PHA_BY_IDX(BINDINGS_NODE, bindings, idx, param1))), \
	  .param2 = COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(BINDINGS_NODE, bindings, idx, param2), (0), (DT_PHA_BY_IDX(BINDINGS_NODE, bindings, idx, param2))), \
	},

static const struct zmk_behavior_binding bindings[] =
    { UTIL_LISTIFY(BINDING_COUNT, BINDING_GEN, 0) };

int zmk_events_position_pressed(u32_t position)
{
    for (int i = 0; i < BINDING_COUNT; i++) {
        const struct zmk_behavior_binding *b = &bindings[i];
        struct device *dev = device_get_binding(b->behavior_dev);
        behavior_position_pressed(dev, position, 0);
    }
    return 0;
};

int zmk_events_position_released(u32_t position)
{
    for (int i = 0; i < BINDING_COUNT; i++) {
        const struct zmk_behavior_binding *b = &bindings[i];
        struct device *dev = device_get_binding(b->behavior_dev);
        behavior_position_released(dev, position, 0);
    }
    return 0;
}
int zmk_events_keycode_pressed(u32_t keycode)
{
    for (int i = 0; i < BINDING_COUNT; i++) {
        const struct zmk_behavior_binding *b = &bindings[i];
        struct device *dev = device_get_binding(b->behavior_dev);
        behavior_keycode_pressed(dev, keycode);
    }
    return 0;
}
int zmk_events_keycode_released(u32_t keycode)
{
    for (int i = 0; i < BINDING_COUNT; i++) {
        const struct zmk_behavior_binding *b = &bindings[i];
        struct device *dev = device_get_binding(b->behavior_dev);
        behavior_keycode_released(dev, keycode);
    }
    return 0;
};
int zmk_events_mod_pressed(u32_t modifier)
{
    return 0;
};
int zmk_events_mod_released(u32_t modifier)
{
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