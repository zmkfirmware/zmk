/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "private.h"

static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_split_serial));

static void clear_fifo(const struct device *const dev) {
    uint8_t c;
    while (uart_fifo_read(dev, &c, 1) > 0) {
    }
}

/**
 * Read from the FIFO until it returns a size of 0.
 *
 * This is a workaround because some drivers read max 1 character per call.
 */
static int uart_fifo_read_all(const struct device *dev, uint8_t *rx_data, int size) {
    int ret;
    int num_read = 0;

    while (size > 0) {
        ret = uart_fifo_read(dev, rx_data, size);
        if (ret < 0) {
            LOG_ERR("Failed to read fifo: %d", ret);
            return ret;
        }
        if (ret == 0) {
            break;
        }

        __ASSERT_NO_MSG(num_read <= size);

        rx_data += ret;
        size -= ret;
        num_read += ret;
    }

    return num_read;
}

static void irq_rx_callback(const struct device *const dev) {
    int ret;
    bool had_data = false;

    for (;;) {
        uint8_t *data = NULL;
        const uint32_t max_size =
            ring_buf_put_claim(&zmk_split_serial_rx_ringbuf, &data, UINT32_MAX);
        if (max_size == 0) {
            clear_fifo(uart_dev);
            break;
        }
        const int max_size_int = MIN(max_size, INT_MAX);

        uint32_t num_read;
        ret = uart_fifo_read_all(dev, data, max_size_int);
        if (ret < 0) {
            LOG_ERR("Failed to read fifo: %d", ret);
            num_read = 0;
        } else {
            num_read = ret;
            had_data = true;
        }

        ret = ring_buf_put_finish(&zmk_split_serial_rx_ringbuf, num_read);
        __ASSERT_NO_MSG(ret == 0);

        if (num_read < max_size_int) {
            break;
        }

        /* There's may still be data in the FIFO, if:
         * - The ring buffer didn't return it's full capacity because
         *   it's about to wrap. Another attempt will return the rest.
         * - In between claim and finish, data was read from the ring
         *   buffer so another attempt will return more data.
         * - The ring buffer is still full. Another attempt will stop
         *   the loop.
         */
    }

    if (had_data) {
        k_work_submit(&zmk_split_serial_rx_work);
    }
}

static void irq_callback(const struct device *const dev, void *const user_data) {
    ARG_UNUSED(dev);

    if (!uart_irq_update(dev)) {
        return;
    }

    if (uart_irq_rx_ready(dev)) {
        irq_rx_callback(dev);
    }
}

static int init(const struct device *dev) {
    ARG_UNUSED(dev);

    if (!device_is_ready(uart_dev)) {
        LOG_ERR("split uart device is not ready");
        return -EAGAIN;
    }

    uart_irq_rx_disable(uart_dev);
    uart_irq_tx_disable(uart_dev);
    clear_fifo(uart_dev);

    uart_irq_callback_user_data_set(uart_dev, irq_callback, NULL);
    uart_irq_rx_enable(uart_dev);

    return 0;
}
SYS_INIT(init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
