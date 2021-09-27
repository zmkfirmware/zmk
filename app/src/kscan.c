/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/kscan.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix_transform.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

#define ZMK_KSCAN_EVENT_STATE_PRESSED 0
#define ZMK_KSCAN_EVENT_STATE_RELEASED 1

struct zmk_kscan_event {
    uint32_t row;
    uint32_t column;
    uint32_t state;
};

struct zmk_kscan_msg_processor {
    struct k_work work;
} msg_processor;

K_MSGQ_DEFINE(zmk_kscan_msgq, sizeof(struct zmk_kscan_event), CONFIG_ZMK_KSCAN_EVENT_QUEUE_SIZE, 4);

static void zmk_kscan_callback(const struct device *dev, uint32_t row, uint32_t column,
                               bool pressed) {
    struct zmk_kscan_event ev = {
        .row = row,
        .column = column,
        .state = (pressed ? ZMK_KSCAN_EVENT_STATE_PRESSED : ZMK_KSCAN_EVENT_STATE_RELEASED)};

    k_msgq_put(&zmk_kscan_msgq, &ev, K_NO_WAIT);
    k_work_submit(&msg_processor.work);
}

void zmk_kscan_process_msgq(struct k_work *item) {
    struct zmk_kscan_event ev;

    while (k_msgq_get(&zmk_kscan_msgq, &ev, K_NO_WAIT) == 0) {
        bool pressed = (ev.state == ZMK_KSCAN_EVENT_STATE_PRESSED);
        uint32_t position = zmk_matrix_transform_row_column_to_position(ev.row, ev.column);
        LOG_DBG("Row: %d, col: %d, position: %d, pressed: %s", ev.row, ev.column, position,
                (pressed ? "true" : "false"));
        ZMK_EVENT_RAISE(new_zmk_position_state_changed((struct zmk_position_state_changed){
            .state = pressed, .position = position, .timestamp = k_uptime_get()}));
    }
}

int zmk_kscan_init(char *name) {
    const struct device *dev = device_get_binding(name);
    if (dev == NULL) {
        LOG_ERR("Failed to get the KSCAN device");
        return -EINVAL;
    }

    k_work_init(&msg_processor.work, zmk_kscan_process_msgq);

    kscan_config(dev, zmk_kscan_callback);
    kscan_enable_callback(dev);

    return 0;
}
