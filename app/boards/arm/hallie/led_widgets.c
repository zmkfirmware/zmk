#include <zmk/led_widgets.h>
#include <zmk/endpoints_types.h>

/**
 * PERIOD:
 * Light leds once per time specified in a .period option
 * 
 * CMD:
 * 1 arg - timeout (how long led should light; 0 - unlimited)
 * 2-N - leds which should light (0-100 is brightness)
 * 
 * LEDS:
 * 1 - Blue
 * 2,3,4 - White from left to right
 */

#if (CONFIG_TEST_MODE)

    const led_widget_t led_widgets[LED_EVENT_SIZE][CONFIG_ZMK_LED_WIDGETS_MAX_WIDGET_NUM] = {
        [LED_EVENT_BOOT] = {
            {.arg = 1, .priority = 100, .period = 0, .cmd_len = 1, {CMD(20000, 100, 100, 100, 100)}},
        },
    };


#elif (!CONFIG_ZMK_SPLIT || (CONFIG_ZMK_SPLIT && CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

    const led_widget_t led_widgets[LED_EVENT_SIZE][CONFIG_ZMK_LED_WIDGETS_MAX_WIDGET_NUM] = {

        [LED_EVENT_BOOT] = {
            {.arg = 1, .priority = 100, .period = 0, .cmd_len = 4, {CMD(200, 0, 100, 0, 0), CMD(200, 0, 0, 100, 0), CMD(200, 0, 0, 0, 100), CMD(200, 100, 0, 0, 0)}},
        },

        // Show battery level on keypress
        [LED_EVENT_BATTERY_ST] = {
            {.arg = 30, .priority = 98, .period = 0, .cmd_len = 1,  { CMD(1500, 0, 100, 0, 0) }},
            {.arg = 60, .priority = 98, .period = 0, .cmd_len = 2,  { CMD(300, 0, 100, 0, 0), CMD(1500, 0, 100, 100, 0) }},
            {.arg = 80, .priority = 98, .period = 0, .cmd_len = 3,  { CMD(300, 0, 100, 0, 0), CMD(300, 0, 100, 100, 0), CMD(1500, 0, 100, 100, 100) }},
            {.arg = 101, .priority = 98, .period = 0, .cmd_len = 4, { CMD(300, 0, 100, 0, 0), CMD(300, 0, 100, 100, 0), CMD(300, 0, 100, 100, 100), CMD(1500, 100, 100, 100, 100) }},
        },

        // Light leds for each layer
        [LED_EVENT_LAYER] = {
            {.arg = 1, .priority = 20, .period = 0, .cmd_len = 1, {CMD(0, 0, 100, 0, 0)}},
            {.arg = 2, .priority = 20, .period = 0, .cmd_len = 1, {CMD(0, 0, 100, 100, 0 )}},
            {.arg = 3, .priority = 20, .period = 0, .cmd_len = 1, {CMD(0, 0, 100, 100, 100)}},
        },

        // Show which output is active (USB/BLE)
        [LED_EVENT_OUTPUT] = {
            {.arg = 0, .priority = 90, .period = 0, .cmd_len = 3, { CMD(200, 100, 0, 0, 0), WAIT(100), CMD(200, 100, 0, 0, 0) }}, // USB
            {.arg = 1, .priority = 90, .period = 0, .cmd_len = 3, { CMD(200, 0, 100, 100, 100), WAIT(100), CMD(200, 0, 100, 100, 100) }}, // BLE
        },

        // Show BLE profile / connection / disconnection / clearing pairing info
        [LED_EVENT_PROFILE] = {
            {.arg = 0, .priority = 40, .period = 0, .cmd_len = 3, {CMD(200, 0, 100, 0, 0), WAIT(100), CMD(200, 0, 100, 0, 0)}},
            {.arg = 1, .priority = 40, .period = 0, .cmd_len = 3, {CMD(200, 0, 0, 100, 0), WAIT(100), CMD(200, 0, 0, 100, 0)}},
            {.arg = 2, .priority = 40, .period = 0, .cmd_len = 3, {CMD(200, 0, 0, 0, 100), WAIT(100), CMD(200, 0, 0, 0, 100)}},
        },

        // Show idle/sleep animation
        [LED_EVENT_ACTIV] = {
            {.arg = 1, .priority = 40, .period = 0, .cmd_len = 3, {CMD(100, 0, 100, 0, 0), WAIT(100), CMD(100, 0, 100, 0, 0)}},
            {.arg = 2, .priority = 40, .period = 0, .cmd_len = 1, {CMD(1000, 0, 100, 0, 0)}},
        },
        
        // USB Connection State
        // [LED_EVENT_CONN] = {
        //     {.arg = 0, .priority = 50, .period = 0, .cmd_len = 3, {CMD(200, 80, 0, 0, 0), WAIT(80), CMD(200, 80, 0, 0, 0)}},
        //     {.arg = 1, .priority = 50, .period = 0, .cmd_len = 1, {CMD(1000, 100, 0, 0, 0)}},
        // },
    };

#else // IS PERIPHERAL

    const led_widget_t led_widgets[LED_EVENT_SIZE][CONFIG_ZMK_LED_WIDGETS_MAX_WIDGET_NUM] = {
        // Show battery level on keypress
        [LED_EVENT_BATTERY_ST] = {
            {.arg = 30, .priority = 98, .period = 0, .cmd_len = 1,  { CMD(1500, 0, 100, 0, 0) }},
            {.arg = 60, .priority = 98, .period = 0, .cmd_len = 2,  { CMD(300, 0, 100, 0, 0), CMD(1500, 0, 100, 100, 0) }},
            {.arg = 80, .priority = 98, .period = 0, .cmd_len = 3,  { CMD(300, 0, 100, 0, 0), CMD(300, 0, 100, 100, 0), CMD(1500, 0, 100, 100, 100) }},
            {.arg = 101, .priority = 98, .period = 0, .cmd_len = 4, { CMD(300, 0, 100, 0, 0), CMD(300, 0, 100, 100, 0), CMD(300, 0, 100, 100, 100), CMD(1500, 100, 100, 100, 100) }},
        },
        
        [LED_EVENT_PERIF] = {
            {.arg = 0, .priority = 90, .period = 0, .cmd_len = 1,  { CMD(300, 100, 0, 0, 0) }},
            {.arg = 1, .priority = 90, .period = 0, .cmd_len = 1,  { CMD(300, 0, 100, 100, 100) }},
        }
    };

#endif