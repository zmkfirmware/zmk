/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/split/common.h>
#include <sys/util.h>
#include <sys/crc.h>
#include <init.h>

#include <device.h>
#include <drivers/uart.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix.h>

#if !DT_HAS_CHOSEN(zmk_split_serial)
#error "No zmk-split-serial node is chosen"
#endif

#define UART_NODE1 DT_CHOSEN(zmk_split_serial)
const struct device *serial_dev = DEVICE_DT_GET(UART_NODE1);
static int uart_ready = 0;

static uint8_t position_state[SPLIT_DATA_LEN];

K_THREAD_STACK_DEFINE(service_q_stack, CONFIG_ZMK_SPLIT_SERIAL_THREAD_STACK_SIZE);

struct k_work_q service_work_q;

K_MSGQ_DEFINE(position_state_msgq, sizeof(char[SPLIT_DATA_LEN]),
              CONFIG_ZMK_SPLIT_SERIAL_THREAD_QUEUE_SIZE, 4);

void send_data_via_uart(const struct device *dev, char *data, size_t len) {
    if (!uart_ready) {
        return;
    }

    for (int i = 0; i < len; i++) {
        uart_poll_out(serial_dev, data[i]);
    }
}

void send_position_state_callback(struct k_work *work) {
    split_data_t split_data = {.type = SPLIT_TYPE_KEYPOSITION};

    while (k_msgq_get(&position_state_msgq, &split_data.data, K_NO_WAIT) == 0) {
        split_data.crc = crc16_ansi(split_data.data, sizeof(split_data.data));
        send_data_via_uart(serial_dev, (void *)&split_data, sizeof(split_data));
    }
};

K_WORK_DEFINE(service_position_notify_work, send_position_state_callback);

int send_position_state() {
    int err = k_msgq_put(&position_state_msgq, position_state, K_MSEC(100));
    if (err) {
        switch (err) {
        case -EAGAIN: {
            LOG_WRN("Position state message queue full, popping first message and queueing again");
            uint8_t discarded_state[SPLIT_DATA_LEN];
            k_msgq_get(&position_state_msgq, &discarded_state, K_NO_WAIT);
            return send_position_state();
        }
        default:
            LOG_WRN("Failed to queue position state to send (%d)", err);
            return err;
        }
    }

    k_work_submit_to_queue(&service_work_q, &service_position_notify_work);
    return 0;
}

int zmk_split_position_pressed(uint8_t position) {
    WRITE_BIT(position_state[position / 8], position % 8, true);
    return send_position_state();
}

int zmk_split_position_released(uint8_t position) {
    WRITE_BIT(position_state[position / 8], position % 8, false);
    return send_position_state();
}

int service_init(const struct device *_arg) {

    if (!device_is_ready(serial_dev)) {
        LOG_WRN("UART device:%s not ready", serial_dev->name);
        return 1;
    }

    uart_ready = 1;
    LOG_INF("UART device:%s ready", serial_dev->name);
    k_work_q_start(&service_work_q, service_q_stack, K_THREAD_STACK_SIZEOF(service_q_stack),
                   CONFIG_ZMK_SPLIT_SERIAL_THREAD_PRIORITY);

    return 0;
}

SYS_INIT(service_init, APPLICATION, CONFIG_ZMK_USB_INIT_PRIORITY);
