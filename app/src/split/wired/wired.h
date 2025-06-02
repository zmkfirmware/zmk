/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/sys/ring_buffer.h>
#include <zephyr/device.h>

#include <zmk/split/transport/types.h>

#define ZMK_SPLIT_WIRED_ENVELOPE_MAGIC_PREFIX "ZmKw"

struct msg_prefix {
    uint8_t magic_prefix[sizeof(ZMK_SPLIT_WIRED_ENVELOPE_MAGIC_PREFIX) - 1];
    uint8_t payload_size;
} __packed;

struct command_payload {
    uint8_t source;
    struct zmk_split_transport_central_command cmd;
} __packed;

struct command_envelope {
    struct msg_prefix prefix;
    struct command_payload payload;
} __packed;

struct event_payload {
    uint8_t source;
    struct zmk_split_transport_peripheral_event event;
} __packed;

struct event_envelope {
    struct msg_prefix prefix;
    struct event_payload payload;
} __packed;

struct msg_postfix {
    uint32_t crc;
} __packed;

#define MSG_EXTRA_SIZE (sizeof(struct msg_prefix) + sizeof(struct msg_postfix))

typedef void (*zmk_split_wired_process_tx_callback_t)(void);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_POLLING)

void zmk_split_wired_poll_out(struct ring_buf *tx_buf, const struct device *uart);

int zmk_split_wired_poll_in(struct ring_buf *rx_buf, const struct device *uart,
                            struct k_work *process_data_work,
                            zmk_split_wired_process_tx_callback_t process_data_cb);

#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)

void zmk_split_wired_fifo_read(const struct device *dev, struct ring_buf *buf,
                               struct k_work *process_work,
                               zmk_split_wired_process_tx_callback_t process_cb);
void zmk_split_wired_fifo_fill(const struct device *dev, struct ring_buf *tx_buf);

#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC)

struct zmk_split_wired_async_state {
    atomic_t state;

    uint8_t *rx_bufs[2];
    size_t rx_bufs_len;
    size_t rx_size_process_trigger;

    struct ring_buf *tx_buf;
    struct ring_buf *rx_buf;

    zmk_split_wired_process_tx_callback_t process_tx_callback;

    const struct device *uart;

    struct k_work_delayable restart_rx_work;
    struct k_work *process_tx_work;
    const struct gpio_dt_spec *dir_gpio;
};

int zmk_split_wired_async_init(struct zmk_split_wired_async_state *state);
void zmk_split_wired_async_tx(struct zmk_split_wired_async_state *state);
int zmk_split_wired_async_rx(struct zmk_split_wired_async_state *state);
int zmk_split_wired_async_rx_cancel(struct zmk_split_wired_async_state *state);

#endif

int zmk_split_wired_get_item(struct ring_buf *rx_buf, uint8_t *env, size_t env_size);