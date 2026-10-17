/* SPDX-License-Identifier: MIT */
/* The entire composite driver is compiled unchanged, including child validation,
 * frame routing, scalar clearing, runtime power switching, and failure rollback.
 * Only hardware driver callbacks are simulated. */
#include "composite_test_fakes.h"
#include "module/drivers/led/led_composite.c"

#define MAX_CHILDREN 5
#define CHANNELS 9
struct fake_led {
    uint8_t values[CHANNELS];
    bool hard_on, blanked;
    bool fail_write, fail_blank, fail_off, fail_resume;
    int fail_led;
    unsigned writes, writes_after_off, off_calls, blank_calls, resume_calls;
};
struct fake_rail {
    bool enabled, persisted;
    bool fail_enable, fail_disable;
    unsigned enables, disables;
};
struct fixture {
    struct fake_led leds[MAX_CHILDREN];
    struct fake_rail rail;
    struct device children[MAX_CHILDREN], ext, gpio, composite;
    struct zmk_led_composite_child_config child_config[MAX_CHILDREN];
    struct zmk_led_composite_child_state states[MAX_CHILDREN];
    struct zmk_led_composite_config config;
    struct zmk_led_composite_data data;
    uint8_t buffers[MAX_CHILDREN][CHANNELS];
    struct led_rgb pixels[MAX_CHILDREN][CHANNELS / 3];
};
static struct fixture f;

static int fake_write(const struct device *dev, uint32_t start, uint32_t count,
                      const uint8_t *buf) {
    struct fake_led *led = dev->data;
    assert(f.data.lock.depth == 1);
    led->writes++;
    if (!led->hard_on) {
        led->writes_after_off++;
        return -EHOSTDOWN;
    }
    if (led->fail_write)
        return -EIO;
    assert(start + count <= CHANNELS);
    memcpy(led->values + start, buf, count);
    return 0;
}
static int fake_brightness(const struct device *dev, uint32_t index, uint8_t value) {
    struct fake_led *led = dev->data;
    assert(f.data.lock.depth == 1);
    led->writes++;
    if (led->fail_write || (int)index == led->fail_led)
        return -EIO;
    assert(index < CHANNELS / 3);
    led->values[index] = value;
    return 0;
}
static int fake_on(const struct device *dev, uint32_t index) {
    return fake_brightness(dev, index, 100);
}
static int fake_off(const struct device *dev, uint32_t index) {
    return fake_brightness(dev, index, 0);
}
static int fake_rgb(const struct device *dev, struct led_rgb *pixels, size_t count) {
    uint8_t bytes[CHANNELS];
    assert(count == CHANNELS / 3);
    for (size_t i = 0; i < count; i++) {
        bytes[3 * i] = pixels[i].r;
        bytes[3 * i + 1] = pixels[i].g;
        bytes[3 * i + 2] = pixels[i].b;
    }
    return fake_write(dev, 0, CHANNELS, bytes);
}
static size_t fake_length(const struct device *dev) {
    (void)dev;
    return CHANNELS / 3;
}
static const struct led_driver_api channel_api = {.write_channels = fake_write};
static const struct led_driver_api brightness_api = {.set_brightness = fake_brightness};
static const struct led_driver_api binary_api = {.on = fake_on, .off = fake_off};
static const struct led_strip_driver_api strip_api = {.update_rgb = fake_rgb,
                                                      .length = fake_length};

int zmk_is31_blank(const struct zmk_is31_power_config *config, bool blank) {
    struct fake_led *led = config->dev->data;
    assert(f.data.lock.depth == 1);
    led->blank_calls++;
    if (led->fail_blank)
        return -EIO;
    led->blanked = blank;
    return 0;
}
int zmk_is31_enter_hard_off(const struct zmk_is31_power_config *config) {
    struct fake_led *led = config->dev->data;
    int err = zmk_is31_blank(config, true);
    if (config->sdb.port == NULL)
        return err;
    led->off_calls++;
    if (led->fail_off)
        return -EIO;
    led->hard_on = false;
    return 0;
}
int zmk_is31_resume_from_hard_off(const struct zmk_is31_power_config *config) {
    struct fake_led *led = config->dev->data;
    assert(f.data.lock.depth == 1);
    led->resume_calls++;
    led->hard_on = true;
    if (led->fail_resume)
        return -ENOMEM;
    memset(led->values, 0, sizeof(led->values));
    led->blanked = false;
    return 0;
}
static int ext_power_disable_runtime(const struct device *dev) {
    struct fake_rail *rail = dev->data;
    assert(f.data.lock.depth == 1);
    rail->disables++;
    if (rail->fail_disable)
        return -EIO;
    rail->enabled = false;
    return 0;
}
static int ext_power_enable_runtime(const struct device *dev) {
    struct fake_rail *rail = dev->data;
    assert(f.data.lock.depth == 1);
    rail->enables++;
    /* Failure can occur after the supply has already changed. */
    rail->enabled = true;
    return rail->fail_enable ? -EIO : 0;
}

enum child_kind { CHANNEL, PWM, GPIO, HARD_IS31, SOFT_IS31, STRIP, POWERED_STRIP };
static void fixture_init(const enum child_kind *kinds, size_t count) {
    memset(&f, 0, sizeof(f));
    f.rail.enabled = f.rail.persisted = true;
    f.ext = (struct device){.data = &f.rail, .name = "rail", .ready = true};
    f.gpio = (struct device){.name = "gpio", .ready = true};
    f.config = (struct zmk_led_composite_config){.children = f.child_config, .child_count = count};
    f.data.child_states = f.states;
    f.composite = (struct device){.config = &f.config,
                                  .data = &f.data,
                                  .api = &zmk_led_composite_api,
                                  .name = "composite",
                                  .ready = true};
    for (size_t i = 0; i < count; i++) {
        f.leds[i].hard_on = true;
        f.leds[i].fail_led = -1;
        memset(f.leds[i].values, 100, sizeof(f.leds[i].values));
        f.children[i] = (struct device){
            .data = &f.leds[i], .api = &channel_api, .name = "child", .ready = true};
        f.child_config[i] =
            (struct zmk_led_composite_child_config){.type = ZMK_LED_COMPOSITE_CHILD_MATRIX,
                                                    .dev = &f.children[i],
                                                    .channel_offset = i * CHANNELS,
                                                    .channel_count = CHANNELS,
                                                    .led_count = CHANNELS / 3,
                                                    .pixel_count = CHANNELS / 3,
                                                    .scalar_channels = f.buffers[i],
                                                    .strip_channels = f.buffers[i],
                                                    .strip_pixels = f.pixels[i]};
        if (kinds[i] == PWM)
            f.children[i].api = &brightness_api;
        if (kinds[i] == GPIO)
            f.children[i].api = &binary_api;
        if (kinds[i] == HARD_IS31 || kinds[i] == SOFT_IS31) {
            f.child_config[i].matrix_kind = ZMK_LED_COMPOSITE_MATRIX_IS31FL3737;
            if (kinds[i] == HARD_IS31)
                f.child_config[i].sdb.port = &f.gpio;
            f.child_config[i].is31_power =
                (struct zmk_is31_power_config){.chip = ZMK_IS31_CHIP_IS31FL3737,
                                               .dev = &f.children[i],
                                               .sdb = f.child_config[i].sdb};
        }
        if (kinds[i] == STRIP || kinds[i] == POWERED_STRIP) {
            f.children[i].api = &strip_api;
            f.child_config[i].type = ZMK_LED_COMPOSITE_CHILD_STRIP;
            if (kinds[i] == POWERED_STRIP)
                f.child_config[i].ext_power_dev = &f.ext;
        }
    }
    assert(zmk_led_composite_init(&f.composite) == 0);
}
static bool dark(size_t i) {
    struct zmk_led_composite_child_config *cfg = &f.child_config[i];
    if (!f.leds[i].hard_on || f.leds[i].blanked)
        return true;
    if (cfg->ext_power_dev && !f.rail.enabled)
        return true;
    size_t count = (f.states[i].matrix_api == ZMK_LED_COMPOSITE_MATRIX_API_BRIGHTNESS ||
                    f.states[i].matrix_api == ZMK_LED_COMPOSITE_MATRIX_API_BINARY)
                       ? cfg->led_count
                       : cfg->channel_count;
    for (size_t j = 0; j < count; j++)
        if (f.leds[i].values[j])
            return false;
    return true;
}
static void assert_all_dark(void) {
    for (size_t i = 0; i < f.config.child_count; i++)
        assert(dark(i));
}
static void light_all(void) {
    uint8_t frame[MAX_CHILDREN * CHANNELS];
    memset(frame, 255, sizeof(frame));
    assert(zmk_led_composite_write_channels(&f.composite, 0, f.config.child_count * CHANNELS,
                                            frame) == 0);
}
static void scalar_and_channel_failures(void) {
    const enum child_kind kinds[] = {GPIO, PWM, CHANNEL, STRIP};
    for (size_t failed = 0; failed < ARRAY_SIZE(kinds); failed++) {
        fixture_init(kinds, ARRAY_SIZE(kinds));
        assert(f.data.capabilities == 0);
        f.leds[failed].fail_write = true;
        assert(zmk_led_composite_power(&f.composite, false) == -EIO);
        for (size_t i = 0; i < ARRAY_SIZE(kinds); i++)
            assert(dark(i) == (i != failed));
        unsigned successful_writes[MAX_CHILDREN];
        for (size_t i = 0; i < ARRAY_SIZE(kinds); i++)
            successful_writes[i] = f.leds[i].writes;
        f.leds[failed].fail_write = false;
        assert(zmk_led_composite_power(&f.composite, false) == 0);
        assert_all_dark();
        for (size_t i = 0; i < ARRAY_SIZE(kinds); i++)
            if (i != failed)
                assert(f.leds[i].writes == successful_writes[i]);
        assert(zmk_led_composite_power(&f.composite, true) == 0);
        light_all();
        assert(zmk_led_composite_power(&f.composite, false) == 0);
        assert_all_dark();
    }
    const enum child_kind scalar[] = {PWM, GPIO};
    fixture_init(scalar, ARRAY_SIZE(scalar));
    for (size_t i = 0; i < ARRAY_SIZE(scalar); i++)
        f.leds[i].fail_led = 1;
    assert(zmk_led_composite_power(&f.composite, false) == -EIO);
    for (size_t i = 0; i < ARRAY_SIZE(scalar); i++) {
        assert(f.leds[i].values[0] == 0 && f.leds[i].values[1] == 100 && f.leds[i].values[2] == 0);
        f.leds[i].fail_led = -1;
    }
    assert(zmk_led_composite_power(&f.composite, false) == 0);
    assert_all_dark();
    puts("PASS composite: unpowered GPIO/PWM/channel/strip failures propagate; remaining LEDs "
         "clear; retries skip completed children");
}
static void mixed_shutdown(void) {
    const enum child_kind kinds[] = {HARD_IS31, PWM, SOFT_IS31, POWERED_STRIP, POWERED_STRIP};
    fixture_init(kinds, ARRAY_SIZE(kinds));
    assert(f.data.capabilities == ZMK_LED_COMPOSITE_CAP_MANAGED_POWER);
    f.leds[0].fail_write = true;
    f.leds[1].fail_led = 1;
    f.leds[2].fail_write = true;
    f.leds[3].fail_write = true;
    assert(zmk_led_composite_power(&f.composite, false) == -EIO);
    assert(!f.leds[0].hard_on && f.leds[2].blanked && !f.rail.enabled);
    assert(f.rail.disables == 1 && f.rail.persisted);
    unsigned hard_writes = f.leds[0].writes, soft_writes = f.leds[2].writes,
             strip_writes = f.leds[3].writes;
    f.leds[1].fail_led = -1;
    assert(zmk_led_composite_power(&f.composite, false) == 0);
    assert_all_dark();
    assert(f.leds[0].writes == hard_writes && f.leds[0].writes_after_off == 0);
    assert(f.leds[2].writes == soft_writes && f.leds[3].writes == strip_writes);
    assert(f.rail.disables == 1);

    const enum child_kind powered[] = {HARD_IS31, SOFT_IS31, POWERED_STRIP};
    for (size_t failed = 0; failed < ARRAY_SIZE(powered); failed++) {
        fixture_init(powered, ARRAY_SIZE(powered));
        if (failed == 0)
            f.leds[0].fail_off = true;
        if (failed == 1)
            f.leds[1].fail_blank = true;
        if (failed == 2)
            f.rail.fail_disable = true;
        assert(zmk_led_composite_power(&f.composite, false) == -EIO);
        if (failed != 0)
            assert(!f.leds[0].hard_on);
        if (failed != 1)
            assert(f.leds[1].blanked);
        if (failed != 2)
            assert(!f.rail.enabled);
        f.leds[0].fail_off = f.leds[1].fail_blank = f.rail.fail_disable = false;
        assert(zmk_led_composite_power(&f.composite, false) == 0);
        assert_all_dark();
        assert(f.leds[0].writes_after_off == 0);
    }
    puts("PASS composite: hard/soft/rail failures do not strand later children; confirmed shutdown "
         "covers clear errors only for that child");
}
static void recovery_and_rollback(void) {
    const enum child_kind kinds[] = {HARD_IS31, HARD_IS31, POWERED_STRIP};
    fixture_init(kinds, ARRAY_SIZE(kinds));
    assert(f.data.capabilities ==
           (ZMK_LED_COMPOSITE_CAP_MANAGED_POWER | ZMK_LED_COMPOSITE_CAP_FULL_HARD_OFF));
    assert(zmk_led_composite_power(&f.composite, false) == 0);
    f.leds[1].fail_resume = true;
    assert(zmk_led_composite_power(&f.composite, true) == -ENOMEM);
    assert_all_dark();
    assert(!f.leds[0].hard_on && !f.leds[1].hard_on && !f.rail.enabled);
    assert(f.rail.enables == 0);
    f.leds[1].fail_resume = false;
    assert(zmk_led_composite_power(&f.composite, true) == 0);
    assert(f.leds[0].resume_calls == 2 && f.leds[1].resume_calls == 2 && f.rail.enabled);
    light_all();
    assert(zmk_led_composite_power(&f.composite, false) == 0);
    f.rail.fail_enable = true;
    assert(zmk_led_composite_power(&f.composite, true) == -EIO);
    assert_all_dark();
    assert(!f.rail.enabled && !f.leds[0].hard_on && !f.leds[1].hard_on);
    f.rail.fail_enable = false;
    assert(zmk_led_composite_power(&f.composite, true) == 0);
    light_all();

    /* A failed SDB shutdown can still have blanked the chip. Recovery must
     * reinitialize unknown state rather than report success without unblanking. */
    f.leds[0].fail_off = true;
    assert(zmk_led_composite_power(&f.composite, false) == -EIO);
    unsigned resumes = f.leds[0].resume_calls;
    f.leds[0].fail_off = false;
    assert(zmk_led_composite_power(&f.composite, true) == 0);
    assert(f.leds[0].resume_calls == resumes + 1 && !f.leds[0].blanked);
    light_all();

    /* Rollback may itself fail; retain the initiating error and retry unknown
     * children with a complete reinitialization on the next resume. */
    assert(zmk_led_composite_power(&f.composite, false) == 0);
    f.leds[0].fail_off = true;
    f.leds[1].fail_resume = true;
    assert(zmk_led_composite_power(&f.composite, true) == -ENOMEM);
    assert(!f.states[0].power_state_known && !f.rail.enabled);
    f.leds[0].fail_off = f.leds[1].fail_resume = false;
    resumes = f.leds[0].resume_calls;
    assert(zmk_led_composite_power(&f.composite, true) == 0);
    assert(f.leds[0].resume_calls == resumes + 1);
    light_all();
    /* The rail may still be on after an earlier failed shutdown. A later
     * matrix resume failure must try it again rather than return early. */
    fixture_init(kinds, ARRAY_SIZE(kinds));
    f.leds[1].fail_off = f.rail.fail_disable = true;
    assert(zmk_led_composite_power(&f.composite, false) == -EIO);
    assert(f.rail.enabled);
    f.leds[1].fail_off = f.rail.fail_disable = false;
    f.leds[1].fail_resume = true;
    assert(zmk_led_composite_power(&f.composite, true) == -ENOMEM);
    assert_all_dark();
    assert(!f.rail.enabled && f.rail.disables == 2);

    const enum child_kind soft[] = {SOFT_IS31, POWERED_STRIP};
    fixture_init(soft, ARRAY_SIZE(soft));
    assert(zmk_led_composite_power(&f.composite, false) == 0);
    f.leds[0].fail_blank = true;
    assert(zmk_led_composite_power(&f.composite, true) == -EIO);
    assert(!f.states[0].power_state_known && !f.rail.enabled);
    f.leds[0].fail_blank = false;
    assert(zmk_led_composite_power(&f.composite, true) == 0);
    assert(f.leds[0].resume_calls == 1 && !f.leds[0].blanked);
    light_all();
    puts("PASS composite: resume errors roll back earlier children and shared rail; unknown power "
         "reinitializes; original error retained");
}
static void repeated_cycles(void) {
    const enum child_kind kinds[] = {HARD_IS31, SOFT_IS31, GPIO, PWM, POWERED_STRIP};
    fixture_init(kinds, ARRAY_SIZE(kinds));
    for (unsigned i = 0; i < 100; i++) {
        assert(zmk_led_composite_power(&f.composite, false) == 0);
        assert_all_dark();
        unsigned writes = f.leds[0].writes, off_calls = f.leds[0].off_calls;
        assert(zmk_led_composite_power(&f.composite, false) == 0);
        assert(f.leds[0].writes == writes && f.leds[0].off_calls == off_calls);
        assert(zmk_led_composite_power(&f.composite, true) == 0);
        light_all();
        assert(f.rail.persisted && f.rail.enabled);
    }
    assert(f.rail.enables == 100 && f.rail.disables == 100);
    assert(f.leds[0].writes_after_off == 0);
    assert(zmk_led_composite_power(&f.composite, false) == 0);
    assert_all_dark();
    puts("PASS composite: 100 mixed shutdown/resume cycles; duplicate OFF is silent; only runtime "
         "rail API is available");
}
int main(void) {
    scalar_and_channel_failures();
    mixed_shutdown();
    recovery_and_rollback();
    repeated_cycles();
    return 0;
}
