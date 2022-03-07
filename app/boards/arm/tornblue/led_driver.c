#include <init.h>
#include <device.h>
#include <devicetree.h>
//#include <drivers/led.h>
#include <drivers/gpio.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

#if DT_NODE_HAS_STATUS(LED1_NODE, okay)
#define LED1    DT_GPIO_LABEL(LED1_NODE, gpios)
#define PIN1    DT_GPIO_PIN(LED1_NODE, gpios)
#define FLAGS1  DT_GPIO_FLAGS(LED1_NODE, gpios)
#else
#error "Unsupported board: led1 devicetree alias is not defined"
#endif

#if DT_NODE_HAS_STATUS(LED2_NODE, okay)
#define LED2    DT_GPIO_LABEL(LED2_NODE, gpios)
#define PIN2    DT_GPIO_PIN(LED2_NODE, gpios)
#define FLAGS2  DT_GPIO_FLAGS(LED2_NODE, gpios)
#else
#error "Unsupported board: led2 devicetree alias is not defined"
#endif

#if DT_NODE_HAS_STATUS(LED3_NODE, okay)
#define LED3    DT_GPIO_LABEL(LED3_NODE, gpios)
#define PIN3    DT_GPIO_PIN(LED3_NODE, gpios)
#define FLAGS3  DT_GPIO_FLAGS(LED3_NODE, gpios)
#else
#error "Unsupported board: led3 devicetree alias is not defined"
#endif

static void led_pin_init(const char *name, gpio_pin_t pin, gpio_dt_flags_t dt_flags) {
    const struct device *dev = device_get_binding(name);
    gpio_pin_configure(dev, pin, GPIO_OUTPUT_ACTIVE | dt_flags);
    gpio_pin_set(dev, pin, 0);
}

static void led_pin_set(const char *name, gpio_pin_t pin, int value) {
    const struct device *dev = device_get_binding(name);
    gpio_pin_set(dev, pin, value);
}

static int led_event_handler(const zmk_event_t *eh) {
    const uint8_t layer_active = zmk_keymap_highest_layer_active();
    led_pin_set(LED3, PIN3, layer_active == 3 /* Nav */);
    led_pin_set(LED2, PIN2, layer_active == 7 /* Symbol */);
    led_pin_set(LED1, PIN1, layer_active == 6 /* Num */);

    return 0;
}

static int led_init(const struct device *port) {
    led_pin_init(LED1, PIN1, FLAGS1);
    led_pin_init(LED2, PIN2, FLAGS2);
    led_pin_init(LED3, PIN3, FLAGS3);

    return 0;
}

ZMK_LISTENER(led, led_event_handler);

#ifdef CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL
ZMK_SUBSCRIPTION(led, zmk_layer_state_changed);
#endif

SYS_INIT(led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
