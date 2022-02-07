/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/split/common.h>
#include <zmk/split/serial/common.h>
#include <init.h>

#include <sys/crc.h>
#include <device.h>
#include <drivers/uart.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/matrix.h>

#if !DT_HAS_CHOSEN(zmk_split_serial)
#error "No zmk-split-serial node is chosen"
#endif

#define UART_NODE1 DT_CHOSEN(zmk_split_serial)
const struct device *serial_dev = DEVICE_DT_GET(UART_NODE1);
static int uart_ready = 0;

K_MEM_SLAB_DEFINE(split_memory_slab, sizeof(split_data_t),
                  CONFIG_ZMK_SPLIT_SERIAL_THREAD_QUEUE_SIZE, 4);

static K_SEM_DEFINE(split_serial_rx_sem, 1, 1);

static K_SEM_DEFINE(split_serial_tx_sem, 1, 1);

rx_complete_t split_serial_rx_complete_fn = NULL;

uint8_t *alloc_split_serial_buffer(k_timeout_t timeout) {
    uint8_t *block_ptr = NULL;
    if (k_mem_slab_alloc(&split_memory_slab, (void **)&block_ptr, timeout) == 0) {
        memset(block_ptr, 0, SPLIT_DATA_LEN);
    } else {
        LOG_WRN("Memory allocation time-out");
    }
    return block_ptr;
}

void free_split_serial_buffer(const uint8_t *data) {
    k_mem_slab_free(&split_memory_slab, (void **)&data);
}

static void enable_rx(const struct device *dev) {
    int ret;
    uint8_t *buf = NULL;
    while (!(buf = alloc_split_serial_buffer(K_MSEC(100)))) {
    };

    while (0 != (ret = uart_rx_enable(serial_dev, buf, sizeof(split_data_t), SYS_FOREVER_MS))) {
        LOG_WRN("UART device:%s RX error:%d", serial_dev->name, ret);
        k_sleep(K_MSEC(100));
    }
    return;
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data) {
    uint8_t *buf = NULL;

    switch (evt->type) {

    case UART_RX_STOPPED:
        LOG_DBG("UART device:%s rx stopped", serial_dev->name);
        break;

    case UART_RX_BUF_REQUEST:
        LOG_DBG("UART device:%s rx extra buf req", serial_dev->name);
        buf = alloc_split_serial_buffer(K_NO_WAIT);
        if (NULL != buf) {
            int ret = uart_rx_buf_rsp(serial_dev, buf, sizeof(split_data_t));
            if (0 != ret) {
                LOG_WRN("UART device:%s rx extra buf req add failed: %d", serial_dev->name, ret);
                free_split_serial_buffer(buf);
            }
        }
        break;

    case UART_RX_RDY:
        LOG_DBG("UART device:%s rx buf ready", serial_dev->name);
        break;

    case UART_RX_BUF_RELEASED:
        LOG_DBG("UART device:%s rx buf released", serial_dev->name);
        if (split_serial_rx_complete_fn) {
            split_serial_rx_complete_fn(evt->data.rx_buf.buf, sizeof(split_data_t));
        }
        free_split_serial_buffer(evt->data.rx_buf.buf);
        break;

    case UART_RX_DISABLED:
        LOG_WRN("UART device:%s rx disabled", serial_dev->name);
        enable_rx(serial_dev);
        break;

    case UART_TX_DONE:
        LOG_DBG("UART device:%s tx done", serial_dev->name);
        free_split_serial_buffer(evt->data.tx.buf);
        k_sem_give(&split_serial_tx_sem);
        break;

    case UART_TX_ABORTED:
        LOG_WRN("UART device:%s tx aborted", serial_dev->name);
        k_sem_give(&split_serial_tx_sem);
        break;

    default:
        LOG_DBG("UART device:%s unhandled event: %u", serial_dev->name, evt->type);
        break;
    };
    return;
}

void split_serial_async_send(uint8_t *data, size_t len) {
    if (!uart_ready) {
        return;
    }

    k_sem_take(&split_serial_tx_sem, K_FOREVER);
    int err = uart_tx(serial_dev, data, len, 0);
    if (0 != err) {
        LOG_WRN("Failed to send data via UART: (%d)", err);
    }
}

void split_serial_async_init(rx_complete_t rx_comp_fn) {
    if (!device_is_ready(serial_dev)) {
        LOG_ERR("UART device:%s not ready", serial_dev->name);
        return;
    }

    int ret = uart_callback_set(serial_dev, uart_callback, NULL);
    if (ret == -ENOTSUP || ret == -ENOSYS) {
        LOG_ERR("UART device:%s ASYNC not supported", serial_dev->name);
        return;
    }

    split_serial_rx_complete_fn = rx_comp_fn;

    uart_ready = 1;
    LOG_INF("UART device:%s ready", serial_dev->name);

    enable_rx(serial_dev);
}
