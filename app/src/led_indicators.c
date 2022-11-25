/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <device.h>
#include <init.h>

#include <logging/log.h>
#include <drivers/led.h>

#include <zmk/event_manager.h>
#include <zmk/led_indicators.h>
#include <zmk/events/led_indicator_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#ifndef DT_NODE_CHILD_IDX

#define CHILD_IDX(idx, child_id, node_id) (DT_SAME_NODE(child_id, node_id) ? idx : 0)
#define ADD_COMMA(node_id) node_id,
#define DT_NODE_CHILD_IDX(node_id)                                                                 \
    (FOR_EACH_IDX_FIXED_ARG(CHILD_IDX, (+), node_id,                                               \
                            DT_FOREACH_CHILD(DT_PARENT(node_id), ADD_COMMA) DT_ROOT))

#endif

struct zmk_led_cfg {
    const struct device *dev;
    int index;
    int indicator;
};

#define LED_CFG_IMPL(node_id, indicator_bit)                                                       \
    (struct zmk_led_cfg) {                                                                         \
        .dev = DEVICE_DT_GET(DT_PARENT(node_id)), .index = DT_NODE_CHILD_IDX(node_id),             \
        .indicator = indicator_bit,                                                                \
    }

#define LED_CFG(chosen, indicator_bit)                                                             \
    IF_ENABLED(DT_HAS_CHOSEN(chosen), (LED_CFG_IMPL(DT_CHOSEN(chosen), indicator_bit)))

static const struct zmk_led_cfg led_indicators[] = {LIST_DROP_EMPTY(
    LED_CFG(zmk_led_numlock, ZMK_LED_NUMLOCK_BIT), LED_CFG(zmk_led_capslock, ZMK_LED_CAPSLOCK_BIT),
    LED_CFG(zmk_led_scrolllock, ZMK_LED_SCROLLLOCK_BIT),
    LED_CFG(zmk_led_compose, ZMK_LED_COMPOSE_BIT), LED_CFG(zmk_led_kana, ZMK_LED_KANA_BIT))};

static int leds_init(const struct device *_arg) {
    for (int i = 0; i < ARRAY_SIZE(led_indicators); i++) {
        if (!device_is_ready(led_indicators[i].dev)) {
            LOG_ERR("LED device \"%s\" is not ready", led_indicators[i].dev->name);
            return -ENODEV;
        }
    }

    return 0;
}

static int leds_listener(const zmk_event_t *eh) {
    const struct zmk_led_changed *ev = as_zmk_led_changed(eh);
    zmk_leds_flags_t leds = ev->leds;

    for (int i = 0; i < ARRAY_SIZE(led_indicators); i++) {
        uint8_t value = (leds & led_indicators[i].indicator) ? CONFIG_ZMK_LED_INDICATORS_BRT : 0;
        int rc = led_set_brightness(led_indicators[i].dev, led_indicators[i].index, value);
        if (rc != 0) {
            LOG_ERR("Failed to set LED indicator %d: %d", i, rc);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(leds_listener, leds_listener);
ZMK_SUBSCRIPTION(leds_listener, zmk_led_changed);

SYS_INIT(leds_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
