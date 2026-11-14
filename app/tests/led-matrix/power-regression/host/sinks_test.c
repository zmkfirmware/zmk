/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define LOGICAL_LED_COUNT 3
#define RGB_CHANNEL_COUNT_PER_LED 3
#define OUTPUTS (LOGICAL_LED_COUNT * RGB_CHANNEL_COUNT_PER_LED)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LOG_ERR(...) ((void)0)
struct device {
    int unused;
};
struct led_rgb8 {
    uint8_t r, g, b;
};
struct led_matrix_rgb_sink_entry {
    uint16_t led;
    uint8_t red_slot, green_slot, blue_slot;
};
static struct {
    struct led_matrix_rgb_sink_entry rgb_leds[LOGICAL_LED_COUNT];
    uint16_t scalar_leds[LOGICAL_LED_COUNT];
    uint16_t scalar_channels[OUTPUTS];
} matrix_output_sink;
static const struct device output_device;
static const struct device *output_dev = &output_device;
static struct {
    uint8_t brightness;
} state = {.brightness = 100};
static struct led_rgb8 logical_leds[LOGICAL_LED_COUNT];
static int injected_error[OUTPUTS];
static uint8_t hardware_values[OUTPUTS];
static unsigned calls[OUTPUTS];

static int led_set_brightness(const struct device *dev, uint32_t led, uint8_t value) {
    assert(dev == output_dev && led < OUTPUTS && value <= 100);
    calls[led]++;
    if (injected_error[led])
        return injected_error[led];
    hardware_values[led] = value;
    return 0;
}
static int led_on(const struct device *dev, uint32_t led) {
    return led_set_brightness(dev, led, 100);
}
static int led_off(const struct device *dev, uint32_t led) {
    return led_set_brightness(dev, led, 0);
}
static int led_set_color(const struct device *dev, uint32_t led, uint8_t count,
                         const uint8_t *values) {
    assert(count == RGB_CHANNEL_COUNT_PER_LED);
    return led_set_brightness(dev, led, (values[0] || values[1] || values[2]) ? 100 : 0);
}
#include "sinks.inc"

enum sink { RGB, SCALAR_PWM, SCALAR_GPIO, TRIPLET_PWM, TRIPLET_GPIO };
static int flush(enum sink sink) {
    switch (sink) {
    case RGB:
        return flush_matrix_rgb_sink();
    case SCALAR_PWM:
        return flush_matrix_scalar_led_sink(false);
    case SCALAR_GPIO:
        return flush_matrix_scalar_led_sink(true);
    case TRIPLET_PWM:
        return flush_matrix_scalar_triplet_sink(false);
    case TRIPLET_GPIO:
        return flush_matrix_scalar_triplet_sink(true);
    }
    assert(false);
    return -EINVAL;
}
static void reset(void) {
    memset(injected_error, 0, sizeof(injected_error));
    memset(calls, 0, sizeof(calls));
    memset(hardware_values, 100, sizeof(hardware_values));
    memset(logical_leds, 0, sizeof(logical_leds));
    for (uint16_t i = 0; i < LOGICAL_LED_COUNT; i++) {
        matrix_output_sink.rgb_leds[i] = (struct led_matrix_rgb_sink_entry){
            .led = i, .red_slot = 2, .green_slot = 0, .blue_slot = 1};
        matrix_output_sink.scalar_leds[i] = i;
    }
    for (uint16_t i = 0; i < OUTPUTS; i++)
        matrix_output_sink.scalar_channels[i] = i;
}
int main(void) {
    for (enum sink sink = RGB; sink <= TRIPLET_GPIO; sink++) {
        uint16_t count = sink >= TRIPLET_PWM ? OUTPUTS : LOGICAL_LED_COUNT;
        uint16_t failure_positions[] = {0, count / 2, count - 1};
        for (unsigned fault = 0; fault < 3; fault++) {
            reset();
            uint16_t failed = failure_positions[fault];
            injected_error[failed] = -EIO;
            assert(flush(sink) == -EIO);
            for (uint16_t i = 0; i < count; i++) {
                assert(calls[i] == 1);
                assert(hardware_values[i] == (i == failed ? 100 : 0));
            }
            injected_error[failed] = 0;
            assert(flush(sink) == 0);
            for (uint16_t i = 0; i < count; i++)
                assert(hardware_values[i] == 0 && calls[i] == 2);
            memset(logical_leds, 255, sizeof(logical_leds));
            assert(flush(sink) == 0);
            for (uint16_t i = 0; i < count; i++)
                assert(hardware_values[i] == 100);
        }
        reset();
        injected_error[0] = -EIO;
        injected_error[count / 2] = -EBUSY;
        assert(flush(sink) == -EIO);
        for (uint16_t i = 0; i < count; i++) {
            assert(calls[i] == 1);
            if (!injected_error[i])
                assert(hardware_values[i] == 0);
        }
    }
    puts("PASS direct sinks: RGB, scalar PWM/GPIO, triplet PWM/GPIO clear every healthy output "
         "after first/middle/last faults, preserve first error, retry and relight");
    return 0;
}
