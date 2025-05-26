/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "wired.h"

#include <zephyr/sys/crc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_POLLING)

void zmk_split_wired_poll_out(struct ring_buf *tx_buf, const struct device *uart) {
    uint8_t *buf;
    uint32_t claim_len;
    while ((claim_len = ring_buf_get_claim(tx_buf, &buf, MIN(32, tx_buf->size))) > 0) {
        LOG_HEXDUMP_DBG(buf, claim_len, "TX Bytes");
        for (int i = 0; i < claim_len; i++) {
            uart_poll_out(uart, buf[i]);
        }

        ring_buf_get_finish(tx_buf, claim_len);
    }
}

int zmk_split_wired_poll_in(struct ring_buf *rx_buf, const struct device *uart,
                            struct k_work *process_data_work,
                            zmk_split_wired_process_tx_callback_t process_data_cb) {
    uint8_t *buf;
    uint32_t read = 0;
    uint32_t claim_len = ring_buf_put_claim(rx_buf, &buf, ring_buf_space_get(rx_buf));
    if (claim_len < 1) {
        LOG_WRN("No room available for reading in from the serial port");
        return -ENOSPC;
    }

    bool all_read = false;
    while (read < claim_len) {
        if (uart_poll_in(uart, buf + read) < 0) {
            all_read = true;
            break;
        }

        read++;
    }

    ring_buf_put_finish(rx_buf, read);

    if (ring_buf_size_get(rx_buf) > 0) {
        if (process_data_work) {
            k_work_submit(process_data_work);
        } else if (process_data_cb) {
            process_data_cb();
        }
    }

    // TODO: Also indicate if no bytes read at all?
    return (all_read ? 1 : 0);
}

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_POLLING)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)

void zmk_split_wired_fifo_read(const struct device *dev, struct ring_buf *buf,
                               struct k_work *process_work,
                               zmk_split_wired_process_tx_callback_t process_cb) {
    // TODO: Add error checking on platforms that support it
    uint32_t last_read = 0, len = 0;
    do {
        uint8_t *buffer;
        len = ring_buf_put_claim(buf, &buffer, buf->size);
        if (len > 0) {
            last_read = uart_fifo_read(dev, buffer, len);

            ring_buf_put_finish(buf, last_read);
        } else {
            LOG_ERR("Dropping incoming RPC byte, insufficient room in the RX buffer. Bump "
                    "CONFIG_ZMK_STUDIO_RPC_RX_BUF_SIZE.");
            uint8_t dummy;
            last_read = uart_fifo_read(dev, &dummy, 1);
        }
    } while (last_read && last_read == len);

    if (process_work) {
        k_work_submit(process_work);
    } else if (process_cb) {
        process_cb();
    }
}

void zmk_split_wired_fifo_fill(const struct device *dev, struct ring_buf *tx_buf) {
    uint32_t len;
    while ((len = ring_buf_size_get(tx_buf)) > 0) {
        uint8_t *buf;
        uint32_t claim_len = ring_buf_get_claim(tx_buf, &buf, tx_buf->size);

        if (claim_len <= 0) {
            break;
        }

        int sent = uart_fifo_fill(dev, buf, claim_len);

        ring_buf_get_finish(tx_buf, MAX(sent, 0));

        if (sent <= 0) {
            break;
        }
    }

    // if (ring_buf_size_get(tx_buf) == 0) {
    //     uart_irq_tx_disable(dev);
    // }
}

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC)

enum ASYNC_STATE_BITS {
    ASYNC_STATE_BIT_RXBUF0_USED = 0,
    ASYNC_STATE_BIT_RXBUF1_USED,
};

void zmk_split_wired_async_tx(struct zmk_split_wired_async_state *state) {
    uint8_t *buf;
    uint32_t claim_len = ring_buf_get_claim(state->tx_buf, &buf, ring_buf_size_get(state->tx_buf));

    if (claim_len <= 0) {
        return;
    }

    if (state->dir_gpio) {
        gpio_pin_set_dt(state->dir_gpio, 1);
    }

#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    LOG_DBG("Sending %d", claim_len);
#endif
    int err = uart_tx(state->uart, buf, claim_len, SYS_FOREVER_US);
    if (err < 0) {
        LOG_DBG("NO TX %d", err);
    }
}

int zmk_split_wired_async_rx(struct zmk_split_wired_async_state *state) {

    atomic_set_bit(&state->state, ASYNC_STATE_BIT_RXBUF0_USED);
    atomic_clear_bit(&state->state, ASYNC_STATE_BIT_RXBUF1_USED);

    int ret = uart_rx_enable(state->uart, state->rx_bufs[0], state->rx_bufs_len,
                             CONFIG_ZMK_SPLIT_WIRED_ASYNC_RX_TIMEOUT);
    if (ret < 0) {
        LOG_ERR("Failed to enable RX (%d)", ret);
    }

    return ret;
}

int zmk_split_wired_async_rx_cancel(struct zmk_split_wired_async_state *state) {
    return uart_rx_disable(state->uart);
}

static void restart_rx_work_cb(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct zmk_split_wired_async_state *state =
        CONTAINER_OF(dwork, struct zmk_split_wired_async_state, restart_rx_work);

    zmk_split_wired_async_rx(state);
}

static void async_uart_cb(const struct device *dev, struct uart_event *ev, void *user_data) {
    struct zmk_split_wired_async_state *state = (struct zmk_split_wired_async_state *)user_data;

    switch (ev->type) {
    case UART_TX_ABORTED:
        // This can only really occur for a TX timeout for a HW flow control UART setup. What to do
        // here in practice?
        LOG_WRN("TX Aborted");
        break;
    case UART_TX_DONE:
        LOG_DBG("TX Done %d", ev->data.tx.len);
        ring_buf_get_finish(state->tx_buf, ev->data.tx.len);
        if (ring_buf_size_get(state->tx_buf) > 0) {
            zmk_split_wired_async_tx(state);
        } else {
            if (state->dir_gpio) {
                gpio_pin_set_dt(state->dir_gpio, 0);
            }
        }
        break;
    case UART_RX_RDY: {
        size_t received =
            ring_buf_put(state->rx_buf, &ev->data.rx.buf[ev->data.rx.offset], ev->data.rx.len);
        if (received < ev->data.rx.len) {
            LOG_ERR("RX overrun!");
            break;
        }

        // LOG_DBG("RX %d and now buffer is %d", received, ring_buf_size_get(state->rx_buf));
        if (state->process_tx_callback) {
            state->process_tx_callback();
        } else if (state->process_tx_work) {
            k_work_submit(state->process_tx_work);
        }

        break;
    }
    case UART_RX_BUF_RELEASED:
        if (ev->data.rx_buf.buf == state->rx_bufs[0]) {
            atomic_clear_bit(&state->state, ASYNC_STATE_BIT_RXBUF0_USED);
        } else if (ev->data.rx_buf.buf == state->rx_bufs[1]) {
            atomic_clear_bit(&state->state, ASYNC_STATE_BIT_RXBUF1_USED);
        }

        break;
    case UART_RX_BUF_REQUEST:
        if (!atomic_test_and_set_bit(&state->state, ASYNC_STATE_BIT_RXBUF0_USED)) {
            uart_rx_buf_rsp(state->uart, state->rx_bufs[0], state->rx_bufs_len);
            break;
        }

        if (!atomic_test_and_set_bit(&state->state, ASYNC_STATE_BIT_RXBUF1_USED)) {
            uart_rx_buf_rsp(state->uart, state->rx_bufs[1], state->rx_bufs_len);
            break;
        }

        LOG_WRN("No RX buffers available!");
        break;
    case UART_RX_STOPPED:
        // LOG_WRN("UART RX Stopped %d with state %ld", ev->data.rx_stop.reason, state->state);
        break;
    case UART_RX_DISABLED: {
        k_work_schedule(&state->restart_rx_work, K_MSEC(1));

        break;
    }
    default:
        break;
    }
}

int zmk_split_wired_async_init(struct zmk_split_wired_async_state *state) {
    __ASSERT(state != NULL, "State is null");

    k_work_init_delayable(&state->restart_rx_work, restart_rx_work_cb);

    int ret = uart_callback_set(state->uart, async_uart_cb, state);
    if (ret < 0) {
        LOG_ERR("Failed to set up async callback on UART (%d)", ret);
        return ret;
    }

    // atomic_set_bit(&state->state, ASYNC_STATE_BIT_RXBUF0_USED);

    // ret = uart_rx_enable(state->uart, state->rx_bufs[0], state->rx_bufs_len,
    //                      CONFIG_ZMK_SPLIT_WIRED_ASYNC_RX_TIMEOUT);
    // if (ret < 0) {
    //     LOG_ERR("Failed to enable RX (%d)", ret);
    //     return ret;
    // }

    return 0;
}

#endif

int zmk_split_wired_get_item(struct ring_buf *rx_buf, uint8_t *env, size_t env_size) {
    while (ring_buf_size_get(rx_buf) > sizeof(struct msg_prefix) + sizeof(struct msg_postfix)) {
        struct msg_prefix prefix;

        __ASSERT_EVAL(
            (void)ring_buf_peek(rx_buf, (uint8_t *)&prefix, sizeof(prefix)),
            uint32_t peek_read = ring_buf_peek(rx_buf, (uint8_t *)&prefix, sizeof(prefix)),
            peek_read == sizeof(prefix), "Somehow read less than we expect from the RX buffer");

        if (memcmp(&prefix.magic_prefix, &ZMK_SPLIT_WIRED_ENVELOPE_MAGIC_PREFIX,
                   sizeof(prefix.magic_prefix)) != 0) {
            uint8_t discarded_byte;
            ring_buf_get(rx_buf, &discarded_byte, 1);

            LOG_WRN("Prefix mismatch, discarding byte %0x", discarded_byte);

            continue;
        }

        size_t payload_to_read = sizeof(prefix) + prefix.payload_size;

        if (payload_to_read > env_size) {
            LOG_WRN("Invalid message with payload %d bigger than expected max %d", payload_to_read,
                    env_size);
            return -EINVAL;
        }

        if (ring_buf_size_get(rx_buf) < payload_to_read + sizeof(struct msg_postfix)) {
            return -EAGAIN;
        }

        // Now that prefix matches, read it out so we can read the rest of the payload.
        __ASSERT_EVAL((void)ring_buf_get(rx_buf, env, payload_to_read),
                      uint32_t read = ring_buf_get(rx_buf, env, payload_to_read),
                      read == payload_to_read,
                      "Somehow read less than we expect from the RX buffer");

        struct msg_postfix postfix;
        __ASSERT_EVAL((void)ring_buf_get(rx_buf, (uint8_t *)&postfix, sizeof(postfix)),
                      uint32_t read = ring_buf_get(rx_buf, (uint8_t *)&postfix, sizeof(postfix)),
                      read == sizeof(postfix),
                      "Somehow read less of the postfix than we expect from the RX buffer");

        uint32_t crc = crc32_ieee(env, payload_to_read);
        if (crc != postfix.crc) {
            LOG_WRN("Data corruption in received peripheral event, ignoring %d vs %d", crc,
                    postfix.crc);
            return -EINVAL;
        }

        return 0;
    }

    return -EAGAIN;
}