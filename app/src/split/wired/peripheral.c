/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/types.h>
#include <zephyr/init.h>

#include <zephyr/settings/settings.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#include <zmk/stdlib.h>
#include <zmk/behavior.h>
#include <zmk/sensors.h>
#include <zmk/split/transport/peripheral.h>
#include <zmk/split/transport/types.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <zmk/pointing/input_split.h>
#include <zmk/hid_indicators_types.h>
#include <zmk/physical_layouts.h>

#include "wired.h"

#define DT_DRV_COMPAT zmk_wired_split

#define IS_HALF_DUPLEX_MODE                                                                        \
    (DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) && DT_INST_PROP_OR(0, half_duplex, false))

#define TX_BUFFER_SIZE                                                                             \
    ((sizeof(struct event_envelope) + sizeof(struct msg_postfix)) *                                \
     CONFIG_ZMK_SPLIT_WIRED_EVENT_BUFFER_ITEMS)
#define RX_BUFFER_SIZE                                                                             \
    ((sizeof(struct command_envelope) + sizeof(struct msg_postfix)) *                              \
     CONFIG_ZMK_SPLIT_WIRED_CMD_BUFFER_ITEMS)

RING_BUF_DECLARE(chosen_rx_buf, RX_BUFFER_SIZE);
RING_BUF_DECLARE(chosen_tx_buf, TX_BUFFER_SIZE);

static const uint8_t peripheral_id = 0;

K_SEM_DEFINE(tx_sem, 0, 1);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define HAS_DIR_GPIO (DT_INST_NODE_HAS_PROP(0, dir_gpios))
static const struct device *uart = DEVICE_DT_GET(DT_INST_PHANDLE(0, device));

#if HAS_DIR_GPIO

static const struct gpio_dt_spec dir_gpio = GPIO_DT_SPEC_INST_GET(0, dir_gpios);

#endif

#else

#error                                                                                             \
    "Need to create a node with compatible of 'zmk,wired-split` with a `device` property set to an enabled UART. See http://zmk.dev/docs/development/hardware-integration/new-shield#wired-split"

#endif

static void publish_commands_work(struct k_work *work);

K_WORK_DEFINE(publish_commands, publish_commands_work);

static void process_tx_cb(void);
K_MSGQ_DEFINE(cmd_msg_queue, sizeof(struct zmk_split_transport_central_command), 3, 4);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC)

uint8_t async_rx_buf[RX_BUFFER_SIZE / 2][2];

static struct zmk_split_wired_async_state async_state = {
    .rx_bufs = {async_rx_buf[0], async_rx_buf[1]},
    .rx_bufs_len = RX_BUFFER_SIZE / 2,
    .rx_size_process_trigger = sizeof(struct command_envelope),
    .process_tx_callback = process_tx_cb,
    .rx_buf = &chosen_rx_buf,
    .tx_buf = &chosen_tx_buf,
#if HAS_DIR_GPIO
    .dir_gpio = &dir_gpio,
#endif
};

#endif
#if HAS_DIR_GPIO

static void set_dir(uint8_t tx) { gpio_pin_set_dt(&dir_gpio, tx); }

#else

static inline void set_dir(uint8_t tx) {}

#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)

static void serial_cb(const struct device *dev, void *user_data) {
    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (uart_irq_rx_ready(dev)) {
            zmk_split_wired_fifo_read(dev, &chosen_rx_buf, NULL, process_tx_cb);
        }

        if (uart_irq_tx_complete(dev)) {
            if (ring_buf_size_get(&chosen_tx_buf) == 0) {
                uart_irq_tx_disable(dev);
            }

            set_dir(0);
        }

        if (uart_irq_tx_ready(dev)) {
            set_dir(1);
            zmk_split_wired_fifo_fill(dev, &chosen_tx_buf);
        }
    }
}

#elif IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_POLLING)

static void send_pending_tx_work_cb(struct k_work *work) {
    zmk_split_wired_poll_out(&chosen_tx_buf, uart);
}

static K_WORK_DEFINE(send_pending_tx, send_pending_tx_work_cb);

static void wired_peripheral_read_tick_cb(struct k_timer *timer) {
    zmk_split_wired_poll_in(&chosen_rx_buf, uart, NULL, process_tx_cb);
}

static K_TIMER_DEFINE(wired_peripheral_read_timer, wired_peripheral_read_tick_cb, NULL);

#endif

static int zmk_split_wired_peripheral_init(void) {
    if (!device_is_ready(uart)) {
        return -ENODEV;
    }

#if HAS_DIR_GPIO
    gpio_pin_configure_dt(&dir_gpio, GPIO_OUTPUT_INACTIVE);
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)
    /* configure interrupt and callback to receive data */
    int ret = uart_irq_callback_user_data_set(uart, serial_cb, NULL);

    if (ret < 0) {
        if (ret == -ENOTSUP) {
            LOG_ERR("Interrupt-driven UART API support not enabled");
        } else if (ret == -ENOSYS) {
            LOG_ERR("UART device does not support interrupt-driven API");
        } else {
            LOG_ERR("Error setting UART callback: %d\n", ret);
        }
        return ret;
    }

    uart_irq_rx_enable(uart);
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC)
    async_state.uart = uart;
    int ret = zmk_split_wired_async_init(&async_state);
    if (ret < 0) {
        LOG_ERR("Failed to set up async wired split UART (%d)", ret);
        return ret;
    }

#elif IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_POLLING)
    k_timer_start(&wired_peripheral_read_timer, K_TICKS(CONFIG_ZMK_SPLIT_WIRED_POLLING_RX_PERIOD),
                  K_TICKS(CONFIG_ZMK_SPLIT_WIRED_POLLING_RX_PERIOD));
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)

    return 0;
}

SYS_INIT(zmk_split_wired_peripheral_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static void begin_tx(void) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)
    uart_irq_tx_enable(uart);
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC)
    zmk_split_wired_async_tx(&async_state);
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_POLLING)
    k_work_submit(&send_pending_tx);
#endif
}

static ssize_t get_payload_data_size(const struct zmk_split_transport_peripheral_event *evt) {
    switch (evt->type) {
    case ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_INPUT_EVENT:
        return sizeof(evt->data.input_event);
    case ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_KEY_POSITION_EVENT:
        return sizeof(evt->data.key_position_event);
    case ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_SENSOR_EVENT:
        return sizeof(evt->data.sensor_event);
    case ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_BATTERY_EVENT:
        return sizeof(evt->data.battery_event);
    default:
        return -ENOTSUP;
    }
}

static int
split_peripheral_wired_report_event(const struct zmk_split_transport_peripheral_event *event) {
    ssize_t data_size = get_payload_data_size(event);
    if (data_size < 0) {
        LOG_WRN("Failed to determine payload data size %d", data_size);
        return data_size;
    }

    // Data + type + source
    size_t payload_size =
        data_size + sizeof(peripheral_id) + sizeof(enum zmk_split_transport_peripheral_event_type);

    if (ring_buf_space_get(&chosen_tx_buf) < MSG_EXTRA_SIZE + payload_size) {
        LOG_WRN("No room to send peripheral to the central (have %d but only space for %d)",
                MSG_EXTRA_SIZE + payload_size, ring_buf_space_get(&chosen_tx_buf));
        return -ENOSPC;
    }

    struct event_envelope env = {.prefix =
                                     {
                                         .magic_prefix = ZMK_SPLIT_WIRED_ENVELOPE_MAGIC_PREFIX,
                                         .payload_size = payload_size,
                                     },
                                 .payload = {
                                     .source = peripheral_id,
                                     .event = *event,
                                 }};

    struct msg_postfix postfix = {.crc =
                                      crc32_ieee((void *)&env, sizeof(env.prefix) + payload_size)};

    LOG_HEXDUMP_DBG(&env, sizeof(env.prefix) + payload_size, "Payload");

    size_t put = ring_buf_put(&chosen_tx_buf, (uint8_t *)&env, sizeof(env.prefix) + payload_size);
    if (put != sizeof(env.prefix) + payload_size) {
        LOG_WRN("Failed to put the whole message (%d vs %d)", put,
                sizeof(env.prefix) + payload_size);
    }
    put = ring_buf_put(&chosen_tx_buf, (uint8_t *)&postfix, sizeof(postfix));
    if (put != sizeof(postfix)) {
        LOG_WRN("Failed to put the whole message (%d vs %d)", put, sizeof(postfix));
    }

#if !IS_HALF_DUPLEX_MODE
    begin_tx();
#endif

    return 0;
}

static const struct zmk_split_transport_peripheral_api peripheral_api = {
    .report_event = split_peripheral_wired_report_event,
};

ZMK_SPLIT_TRANSPORT_PERIPHERAL_REGISTER(wired_peripheral, &peripheral_api);

static void process_tx_cb(void) {
    while (ring_buf_size_get(&chosen_rx_buf) > MSG_EXTRA_SIZE) {
        struct command_envelope env;
        int item_err = zmk_split_wired_get_item(&chosen_rx_buf, (uint8_t *)&env,
                                                sizeof(struct command_envelope));
        switch (item_err) {
        case 0:
            if (env.payload.cmd.type == ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_POLL_EVENTS) {
                begin_tx();
            } else {
                int ret = k_msgq_put(&cmd_msg_queue, &env.payload.cmd, K_NO_WAIT);
                if (ret < 0) {
                    LOG_WRN("Failed to queue command for processing (%d)", ret);
                    return;
                }

                k_work_submit(&publish_commands);
            }
            break;
        case -EAGAIN:
            return;
        default:
            LOG_WRN("Issue fetching an item from the RX buffer: %d", item_err);
            return;
        }
    }
}
static void publish_commands_work(struct k_work *work) {
    struct zmk_split_transport_central_command cmd;

    while (k_msgq_get(&cmd_msg_queue, &cmd, K_NO_WAIT) >= 0) {
        zmk_split_transport_peripheral_command_handler(&wired_peripheral, cmd);
    }
}
