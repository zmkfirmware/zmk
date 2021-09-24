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

int led_event_handler(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    const struct device *dev = device_get_binding(LED1);
    gpio_pin_configure(dev, PIN1, GPIO_OUTPUT_ACTIVE | FLAGS1);
    gpio_pin_configure(dev, PIN2, GPIO_OUTPUT_ACTIVE | FLAGS2);
    gpio_pin_configure(dev, PIN3, GPIO_OUTPUT_ACTIVE | FLAGS3);

    const uint8_t layer_active = zmk_keymap_highest_layer_active();
    gpio_pin_set(dev, PIN3, layer_active == 3 /* Nav */);
    gpio_pin_set(dev, PIN2, layer_active == 7 /* Symbol */);
    gpio_pin_set(dev, PIN1, layer_active == 6 /* Num */);

    return 0;
}

ZMK_LISTENER(led_foo, led_event_handler);
ZMK_SUBSCRIPTION(led_foo, zmk_layer_state_changed);
