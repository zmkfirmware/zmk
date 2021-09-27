
#include <init.h>
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <sys/__assert.h>
#include <drivers/kscan.h>
#include <zmk/matrix_transform.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <drivers/led.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
#define ZMK_KSCAN_EVENT_STATE_PRESSED 0
#define ZMK_KSCAN_EVENT_STATE_RELEASED 1
#define BASE_RED 255
#define BASE_GREEN 0
#define BASE_BLUE 255
uint8_t led_base_color[3] = {BASE_RED, BASE_GREEN, BASE_BLUE};
#define PRESSED_RED 250
#define PRESSED_GREEN 0
#define PRESSED_BLUE 0
uint8_t pressed_color[3] = {PRESSED_RED, PRESSED_GREEN, PRESSED_BLUE};
struct zmk_kscan_event {
    uint32_t row;
    uint32_t column;
    uint32_t state;
};
K_MSGQ_DEFINE(zmk_kscan_msgxq, sizeof(struct zmk_kscan_event), CONFIG_ZMK_KSCAN_EVENT_QUEUE_SIZE, 4);
struct zmk_kscan_msg_processor {
    struct k_work work;
} led_processor;

static void zmk_kscan_callback_led(const struct device *dev, uint32_t row, uint32_t column,
                               bool pressed) {
    struct zmk_kscan_event evx = {
        .row = row,
        .column = column,
        .state = (pressed ? ZMK_KSCAN_EVENT_STATE_PRESSED : ZMK_KSCAN_EVENT_STATE_RELEASED)};

    k_msgq_put(&zmk_kscan_msgxq, &evx, K_NO_WAIT);
    k_work_submit(&led_processor.work);
}

uint8_t led_lookup_matrix[108] = {  1,  0,  3,  5,  7,  9, 11, 13, 15, 65,  0, 67, 69, 71, 73, 75, 77, 79,
                                    17, 20, 22, 24, 26, 28, 30, 80, 81, 82, 83, 84, 85,  0, 86, 87, 88, 90,
                                    19, 21, 23, 25, 27, 29, 31, 96, 98, 100, 102, 104, 106, 0, 108, 109, 110, 92,
                                    34, 36, 38, 40, 42, 44, 46, 97, 99, 101, 103, 105, 0, 0, 107, 0, 0, 0,
                                    35, 0,  37, 39, 41, 43, 45, 47, 112, 113, 114, 115, 0, 0, 119, 0, 111, 0,
                                    49, 51, 53, 0, 0, 0, 0, 57, 0, 0, 0, 62, 116, 118, 121, 123, 125, 127};

void set_led_rgb(uint8_t led, uint8_t *rgb){
    struct device *dev = NULL;
    if (led < 64){
        dev = device_get_binding("IS31FL3733A");
    }
    else{
        dev = device_get_binding("IS31FL3733B");
        led = led - 64;
    }
    LOG_INF("Setting LED: %d to %d, %d, %d, on Device: %s\n", led, rgb[0], rgb[1], rgb[2], dev->name);
    if (dev == NULL) {
		LOG_ERR("Failed to get device binding\n");
		return;
	}
    led_set_color(dev, led, 3, rgb);
}
void zmk_kscan_process_msgxq(struct k_work *item) {
    struct zmk_kscan_event evx;

    while (k_msgq_get(&zmk_kscan_msgxq, &evx, K_NO_WAIT) == 0) {
        bool pressed = (evx.state == ZMK_KSCAN_EVENT_STATE_PRESSED);
        uint32_t position = zmk_matrix_transform_row_column_to_position(evx.row, evx.column);
        struct position_state_changed *pos_evx;
        LOG_INF("Row: %d, col: %d, position: %d, pressed: %s, turning LED: %d %s\n", evx.row, evx.column, position, 
                (pressed ? "true" : "false"), led_lookup_matrix[position], (pressed ? "ON" : "OFF") );
        if (pressed){
            set_led_rgb(led_lookup_matrix[position], pressed_color);
        }
        else{ 
            set_led_rgb(led_lookup_matrix[position], led_base_color);
        }
        pos_evx = new_position_state_changed();
        pos_evx->state = pressed;
        pos_evx->position = position;
        pos_evx->timestamp = k_uptime_get();
        ZMK_EVENT_RAISE(pos_evx);
    }
}
void set_all_on(const struct device *dev){
        for (uint8_t led = 0; led < 192; led++) {
        led_on(dev, led);
    }
}
void set_all_rgb(const struct device *dev, uint8_t *rgb){
    for (uint8_t led = 0; led < 64; led++) {
       led_set_color(dev, led, 3, rgb);
    }
}
int led_perkey_init(char *name) {    
    LOG_INF("ZMK_PER_KEY_LED INIT\n");
	const struct device *dev_a = device_get_binding("IS31FL3733A");
    	if (!dev_a) {
		LOG_ERR("I2C: Device driver not found.\n");
		return 0;
	}
    const struct device *dev_b = device_get_binding("IS31FL3733B");
    if (!dev_b) {
		LOG_ERR("I2C: Device driver not found.\n");
		return 0;
	}
	if (dev_a == NULL) {
		LOG_ERR("Failed to get device binding\n");
		return 0;
	}
	LOG_INF("device is %p, name is %s\n", dev_a, dev_a->name);
	if (dev_b == NULL) {
		LOG_ERR("Failed to get device binding\n");
		return 0;
	}
	LOG_INF("device is %p, name is %s\n", dev_b, dev_b->name);
    set_all_on(dev_a);
    set_all_on(dev_b);
    set_all_rgb(dev_a, led_base_color);
    set_all_rgb(dev_b, led_base_color);
    const struct device *kscan_dev = device_get_binding(name);
    if (kscan_dev == NULL) {
        LOG_ERR("Failed to get the KSCAN device");
        return -EINVAL;
    }
    k_work_init(&led_processor.work, zmk_kscan_process_msgxq);
    kscan_config(kscan_dev, zmk_kscan_callback_led);
    kscan_enable_callback(kscan_dev);
    return 0;
}
