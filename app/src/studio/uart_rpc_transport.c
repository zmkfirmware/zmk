/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
#include "rpc_tx.h"

LOG_MODULE_DECLARE(zmk_studio, CONFIG_ZMK_STUDIO_LOG_LEVEL);

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zmk_studio_rpc_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static void uart_tx_start(void) {
#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
    uart_irq_tx_enable(uart_dev);
#endif
}

#if !IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)

// Polling UART drivers may block in uart_poll_out. Keep this off the frame writer.
static void uart_poll_tx(void) {
    uint32_t ticket;
    uint8_t buf[32];
    size_t len;
    if ((len = studio_tx_peek(ZMK_TRANSPORT_USB, buf, sizeof(buf), &ticket))) {
        for (size_t i = 0; i < len; i++) {
            uart_poll_out(uart_dev, buf[i]);
        }
        studio_tx_complete(ticket, len);
    }
}

static void uart_rx_main(void) {
    for (;;) {
        uart_poll_tx();
        uint8_t byte;
        if (uart_poll_in(uart_dev, &byte) < 0) {
            k_sleep(K_MSEC(1));
        } else {
            zmk_rpc_rx_put(ZMK_TRANSPORT_USB, &byte, 1);
        }
    }
}

K_THREAD_DEFINE(uart_transport_read_thread, CONFIG_ZMK_STUDIO_TRANSPORT_UART_RX_STACK_SIZE,
                uart_rx_main, NULL, NULL, NULL, CONFIG_ZMK_STUDIO_TRANSPORT_UART_RX_PRIORITY, 0, 0);

#endif

static int start_rx() {
#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
    uart_irq_rx_enable(uart_dev);
#else
    k_thread_resume(uart_transport_read_thread);
#endif
    return 0;
}

static int stop_rx(void) {
#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
    uart_irq_rx_disable(uart_dev);
#else
    k_thread_suspend(uart_transport_read_thread);
#endif
    return 0;
}

ZMK_RPC_TRANSPORT(uart, ZMK_TRANSPORT_USB, start_rx, stop_rx, uart_tx_start);

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
static void serial_cb(const struct device *dev, void *user_data) {
    if (!uart_irq_update(uart_dev)) {
        return;
    }

    if (uart_irq_rx_ready(uart_dev)) {
        uint8_t buf[MIN(16, CONFIG_ZMK_STUDIO_RPC_RX_BUF_SIZE)];
        int len;
        do {
            len = uart_fifo_read(uart_dev, buf, sizeof(buf));
            if (len > 0 && zmk_rpc_rx_put(ZMK_TRANSPORT_USB, buf, len) == -ENOSPC) {
                LOG_WRN("Studio RX buffer full");
            }
        } while (len == sizeof(buf));
    }

    if (uart_irq_tx_ready(uart_dev)) {
        uint8_t buf[32];
        uint32_t ticket;
        size_t len = studio_tx_peek(ZMK_TRANSPORT_USB, buf, sizeof(buf), &ticket);
        if (!len) {
            uart_irq_tx_disable(uart_dev);
            // A writer may have enabled TX just before we disabled it.
            if (studio_tx_peek(ZMK_TRANSPORT_USB, buf, sizeof(buf), &ticket)) {
                uart_irq_tx_enable(uart_dev);
            }
            return;
        }
        int sent = uart_fifo_fill(uart_dev, buf, len);
        studio_tx_complete(ticket, sent);
        // A full hardware FIFO will signal again when space becomes available.
    }
}

#endif

static int uart_rpc_interface_init(void) {
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not found!");
        return -ENODEV;
    }

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
    /* configure interrupt and callback to receive data */
    int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

    if (ret < 0) {
        if (ret == -ENOTSUP) {
            printk("Interrupt-driven UART API support not enabled\n");
        } else if (ret == -ENOSYS) {
            printk("UART device does not support interrupt-driven API\n");
        } else {
            printk("Error setting UART callback: %d\n", ret);
        }
        return ret;
    }
#endif // IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)

    return 0;
}

SYS_INIT(uart_rpc_interface_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
