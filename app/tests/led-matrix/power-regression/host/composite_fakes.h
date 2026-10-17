/* SPDX-License-Identifier: MIT */
/* Only child routing is faked. Production composite sequencing and IS31 driver run verbatim. */
#define ZMK_LED_COMPOSITE_CAP_MANAGED_POWER 1
#define ZMK_LED_COMPOSITE_CHILD_STRIP 1
struct zmk_led_composite_child_config {
    struct zmk_is31_power_config is31_power;
    const struct device *dev, *ext_power_dev;
    int type;
};
struct zmk_led_composite_child_state {
    bool hard_powered_off, soft_blanked, power_state_known, shutdown_complete;
};
struct zmk_led_composite_config {
    const struct zmk_led_composite_child_config *children;
    size_t child_count;
};
struct zmk_led_composite_data {
    uint32_t capabilities;
    struct zmk_led_composite_child_state *child_states;
    const struct device *shared_ext_power_dev;
    bool shared_ext_power_is_off, shared_ext_power_state_known;
};
static bool
zmk_led_composite_child_supports_soft_power(const struct zmk_led_composite_child_config *c) {
    return c->type != ZMK_LED_COMPOSITE_CHILD_STRIP;
}
static bool
zmk_led_composite_child_supports_hard_power(const struct zmk_led_composite_child_config *c) {
    return c->is31_power.sdb.port != NULL;
}
static int zmk_led_composite_child_enter_hard_off(const struct zmk_led_composite_child_config *c) {
    return zmk_is31_enter_hard_off(&c->is31_power);
}
static int
zmk_led_composite_child_resume_from_hard_off(const struct zmk_led_composite_child_config *c) {
    return zmk_is31_resume_from_hard_off(&c->is31_power);
}
static int zmk_led_composite_child_blank(const struct zmk_led_composite_child_config *c,
                                         bool blank) {
    return zmk_is31_blank(&c->is31_power, blank);
}
static int ext_power_disable_runtime(const struct device *dev) {
    (void)dev;
    return 0;
}
static int ext_power_enable_runtime(const struct device *dev) {
    (void)dev;
    return 0;
}
/* The standalone composite suite compiles the real clear implementation for
 * channels/scalars/strips. This single-IS31 fixture keeps the actual chip driver. */
static int zmk_led_composite_clear_child_locked(const struct zmk_led_composite_child_config *child,
                                                const struct zmk_led_composite_child_state *state) {
    (void)state;
    const uint8_t zeros[IS31FL3737_DENSE_CHANNEL_COUNT] = {0};
    return is31fl3737_led_write_channels(child->dev, 0, sizeof(zeros), zeros);
}
#include "composite.inc"
static void test_composite(void) {
    reset();
    struct zmk_led_composite_child_config child = {.is31_power = power, .dev = &led_dev};
    struct zmk_led_composite_child_state child_state = {0};
    struct zmk_led_composite_config cfg = {.children = &child, .child_count = 1};
    struct zmk_led_composite_data comp_data = {.capabilities = 1, .child_states = &child_state};
    struct device comp = {.config = &cfg, .data = &comp_data, .ready = true};
    fail_bus_all = true;
    assert(zmk_led_composite_power_locked(&comp, false) == 0 && sdb == 0);
    assert(child_state.hard_powered_off && child_state.soft_blanked);
    assert(zmk_led_composite_power_locked(&comp, true) == -EIO && sdb == 0);
    assert(child_state.hard_powered_off);
    fail_bus_all = false;
    assert(zmk_led_composite_power_locked(&comp, true) == 0 && sdb == 1);
    assert(!child_state.hard_powered_off && !child_state.soft_blanked);
    fail_gpio_value = 0;
    assert(zmk_led_composite_power_locked(&comp, false) == -EIO);
    assert(!child_state.hard_powered_off);
    puts("PASS composite: real IS31 shutdown fallback, failed resume rollback, recovery, GPIO "
         "failure propagation");
}
