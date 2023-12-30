/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>

#include <zmk/split/serial/serial.h>

// TODO TODO TODO
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(slicemk);

#define SERIAL_MSG_PREFIX "UarT"

K_THREAD_STACK_DEFINE(serial_wq_stack, 1024);
static struct k_work_q serial_wq;

static struct serial_device serial_devs[] = {
#ifdef CONFIG_ZMK_SPLIT_SERIAL_UART
    {
        .dev = DEVICE_DT_GET(DT_CHOSEN(zmk_split_uart)),
#ifdef CONFIG_ZMK_SPLIT_SERIAL_UART_POLL
        .poll = true,
#endif
    },
#endif
#ifdef CONFIG_ZMK_SPLIT_SERIAL_CDC_ACM
    {
        .dev = DEVICE_DT_GET(DT_CHOSEN(zmk_split_cdc_acm)),
    },
#endif
};

#define CONFIG_ZMK_SPLIT_SERIAL_COUNT ARRAY_SIZE(serial_devs)

static bool serial_tx_callback(struct serial_device *ud) {
    // Read data from buffer. Stop transmitting if buffer is empty.
    uint8_t data[32];
    int len = ring_buf_peek(&ud->tx_rb, data, sizeof(data));
    if (len == 0) {
        return true;
    }

    // Write data to UART and remove number of bytes written from buffer.
    int ret = uart_fifo_fill(ud->dev, data, len);
    if (ret < 0) {
        LOG_ERR("failed to fill UART FIFO (err %d)", ret);
        return true;
    }
    ring_buf_get(&ud->tx_rb, data, ret);
    return false;
}

static void serial_rx_work_handler(struct k_work *work) {
    struct serial_device *sd = CONTAINER_OF(work, struct serial_device, rx_work);

    // Continue processing data as long as the buffer exceeds the header length
    // (13 bytes).
    uint8_t data[280];
    while (ring_buf_peek(&sd->rx_rb, data, 13) >= 13) {
        // Discard single byte if prefix does not match.
        if (memcmp(data, SERIAL_MSG_PREFIX, strlen(SERIAL_MSG_PREFIX))) {
            uint8_t discard;
            ring_buf_get(&sd->rx_rb, &discard, 1);
            continue;
        }

        // Stop processing if message body is not completely buffered.
        int len = data[12];
        int total = len + 13;
        if (ring_buf_size_get(&sd->rx_rb) < total) {
            return;
        }

        // Check message checksum and handle message.
        uint32_t cmd, crc;
        ring_buf_get(&sd->rx_rb, data, total);
        memcpy(&cmd, &data[4], sizeof(cmd));
        memcpy(&crc, &data[8], sizeof(crc));
        if (crc == crc32_ieee(&data[13], len)) {
            serial_handle_rx(cmd, &data[13], len);
        } else {
            LOG_ERR("received UART message with invalid CRC32 checksum");
        }
    }
}

static void serial_rx_callback(struct serial_device *sd) {
    uint8_t c;
    while (uart_fifo_read(sd->dev, &c, 1) == 1) {
        ring_buf_put(&sd->rx_rb, &c, 1);
    }
    k_work_submit_to_queue(&serial_wq, &sd->rx_work);
}

static void serial_callback(const struct device *dev, void *data) {
    if (uart_irq_update(dev)) {
        struct serial_device *ud = data;

        if (uart_irq_tx_ready(dev)) {
            // If transmission complete, disable IRQ until next transmission.
            bool complete = serial_tx_callback(ud);
            if (complete) {
                uart_irq_tx_disable(dev);
            }
        }

        // TODO TODO TODO lookup index in serial_devs array for slot ID?
        if (uart_irq_rx_ready(dev)) {
            serial_rx_callback(ud);
        }
    }
}

static void serial_write(struct serial_device *sd, uint32_t cmd, uint8_t *data, uint8_t len) {
    // TODO TODO TODO use buf with size SERIAL_BUF_SIZE. do single
    // ring_buf_put() to avoid potential race
    uint8_t header[13] = SERIAL_MSG_PREFIX;
    memcpy(&header[4], &cmd, sizeof(cmd));
    uint32_t crc = crc32_ieee(data, len);
    memcpy(&header[8], &crc, sizeof(crc));
    header[12] = len;
    ring_buf_put(&sd->tx_rb, header, sizeof(header));
    ring_buf_put(&sd->tx_rb, data, len);

#ifdef CONFIG_ZMK_SPLIT_SERIAL_UART_POLL
    if (sd->poll) {
        k_work_submit_to_queue(&serial_wq, &sd->tx_work);
        return;
    }
#endif

    uart_irq_tx_enable(sd->dev);
}

// TODO TODO TODO this should be abstracted a bit differently
#ifdef CONFIG_ZMK_SPLIT_SERIAL_UART
void serial_write_uart(uint32_t cmd, uint8_t *data, uint8_t len) {
    serial_write(&serial_devs[0], cmd, data, len);
}
#endif

#ifdef CONFIG_ZMK_SPLIT_SERIAL_UART_POLL

static void serial_tx_work_handler(struct k_work *work) {
    struct serial_device *sd = CONTAINER_OF(work, struct serial_device, tx_work);
    uint8_t c;
    while (ring_buf_get(&sd->tx_rb, &c, sizeof(c))) {
        uart_poll_out(sd->dev, c);
    }
}

static void serial_rx_timer_handler(struct k_timer *timer) {
    struct serial_device *sd = CONTAINER_OF(timer, struct serial_device, rx_timer);
    uint8_t c;
    while (uart_poll_in(sd->dev, &c) == 0) {
        ring_buf_put(&sd->rx_rb, &c, sizeof(c));
    }
    k_work_submit_to_queue(&serial_wq, &sd->rx_work);
}

#endif

static int serial_init(void) {
    struct k_work_queue_config uart_tx_cfg = {.name = "serial_wq"};
    k_work_queue_start(&serial_wq, serial_wq_stack, K_THREAD_STACK_SIZEOF(serial_wq_stack), 14,
                       &uart_tx_cfg);

    for (int i = 0; i < CONFIG_ZMK_SPLIT_SERIAL_COUNT; i++) {
        struct serial_device *sd = &serial_devs[i];
        if (!device_is_ready(sd->dev)) {
            LOG_ERR("failed to get serial device %s", sd->dev->name);
            return 1;
        }

        // Initialize ring buffer.
        ring_buf_init(&sd->rx_rb, sizeof(sd->rx_buf), sd->rx_buf);
        ring_buf_init(&sd->tx_rb, sizeof(sd->tx_buf), sd->tx_buf);

        k_work_init(&sd->rx_work, serial_rx_work_handler);
#ifdef CONFIG_ZMK_SPLIT_SERIAL_UART_POLL
        if (sd->poll) {
            k_timer_init(&sd->rx_timer, serial_rx_timer_handler, NULL);
            k_timer_start(&sd->rx_timer, K_NO_WAIT, K_TICKS(1));
            k_work_init(&sd->tx_work, serial_tx_work_handler);
            continue;
        }
#endif

        int err = uart_irq_callback_user_data_set(sd->dev, serial_callback, sd);
        if (err) {
            LOG_ERR("failed to set callback for %s (err %d)", sd->dev->name, err);
            return err;
        }
        uart_irq_rx_enable(sd->dev);
    }

    return 0;
}

SYS_INIT(serial_init, APPLICATION, CONFIG_ZMK_SPLIT_INIT_PRIORITY);
