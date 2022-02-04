#pragma once

#include <zephyr/devicetree.h>
#include <stdint.h>

typedef enum {
    LED_EVENT_BOOT = 0,
    LED_EVENT_BATTERY,
    LED_EVENT_LAYER,
    LED_EVENT_OUTPUT,
    LED_EVENT_PROFILE,
    LED_EVENT_CONN,
    LED_EVENT_SIZE,
} led_event_type_t;

typedef enum {
    LED_ENDPOINT_CONN = 0,
    LED_ENDPOINT_DISCONN,
} led_endpoint_connected_t;

typedef enum {
    LED_STATE_IDLE = 0,
    LED_STATE_PAUSE,
    LED_STATE_ACTIVE,
} led_state_t;

#define _NARG(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, N, ...) N
#define NARG(...) _NARG(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
/* #define _F(x) 1 */
/* #define NARG(x) FOR_EACH(_F, (+), x) */
#define LEDS LIST_DROP_EMPTY(DT_SUPPORTS_DEP_ORDS(DT_CHOSEN(zmk_led_widgets_dev)))
#define NUM_LEDS UTIL_EVAL(NARG(LEDS))

typedef struct {
    uint8_t brightness[NUM_LEDS];
    uint16_t timeout;
} led_cmd_t;

typedef struct {
    uint8_t arg;
    uint8_t priority;
    uint32_t period;
    uint8_t cmd_len;
    led_cmd_t commands[5];
} led_widget_t;

#define _ZERO(a) 0
#define WAIT(t)                                                                                    \
    { {FOR_EACH(_ZERO, (, ), LEDS)}, t }
#define CMD(t, ...)                                                                                \
    { {__VA_ARGS__}, t }
#define WIDGET(a, b, c, d, ...)                                                                    \
    {                                                                                              \
        .arg = a, .b, .c, .d, { __VA_ARGS__ }                                                      \
    }
