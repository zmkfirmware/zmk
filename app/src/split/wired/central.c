/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/types.h>
#include <zephyr/init.h>

#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
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
#include <zmk/split/transport/central.h>
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

#define RX_BUFFER_SIZE                                                                             \
    ((sizeof(struct event_envelope) + sizeof(struct msg_postfix)) *                                \
     CONFIG_ZMK_SPLIT_WIRED_EVENT_BUFFER_ITEMS)
#define TX_BUFFER_SIZE                                                                             \
    ((sizeof(struct command_envelope) + sizeof(struct msg_postfix)) *                              \
     CONFIG_ZMK_SPLIT_WIRED_CMD_BUFFER_ITEMS)

#if IS_HALF_DUPLEX_MODE

static K_SEM_DEFINE(tx_sem, 0, 1);

#endif

RING_BUF_DECLARE(rx_buf, RX_BUFFER_SIZE);
RING_BUF_DECLARE(tx_buf, TX_BUFFER_SIZE);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static const struct device *uart = DEVICE_DT_GET(DT_INST_PHANDLE(0, device));

#define HAS_DIR_GPIO (IS_HALF_DUPLEX_MODE && DT_INST_NODE_HAS_PROP(0, dir_gpios))

#if HAS_DIR_GPIO

static const struct gpio_dt_spec dir_gpio = GPIO_DT_SPEC_INST_GET(0, dir_gpios);

#endif

#define HAS_DETECT_GPIO DT_INST_NODE_HAS_PROP(0, detect_gpios)

#if HAS_DETECT_GPIO

static const struct gpio_dt_spec detect_gpio = GPIO_DT_SPEC_INST_GET(0, detect_gpios);

#endif

#else

#error                                                                                             \
    "Need to create a node with compatible of 'zmk,wired-split` with a `device` property set to an enabled UART. See http://zmk.dev/docs/development/hardware-integration/new-shield#wired-split"

#endif

static void publish_events_work(struct k_work *work);

K_WORK_DEFINE(publish_events, publish_events_work);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC)

uint8_t async_rx_buf[RX_BUFFER_SIZE / 2][2];

static struct zmk_split_wired_async_state async_state = {
    .process_tx_work = &publish_events,
    .rx_bufs = {async_rx_buf[0], async_rx_buf[1]},
    .rx_bufs_len = RX_BUFFER_SIZE / 2,
    .rx_size_process_trigger = MSG_EXTRA_SIZE + 1,
    .rx_buf = &rx_buf,
    .tx_buf = &tx_buf,
#if HAS_DIR_GPIO
    .dir_gpio = &dir_gpio,
#endif
};

#endif

#if IS_HALF_DUPLEX_MODE

static int can_tx(void) { return k_sem_take(&tx_sem, K_NO_WAIT); }

#else

static inline int can_tx(void) { return 0; }

#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_POLLING)

static void send_pending_tx_work_cb(struct k_work *work);

static K_WORK_DEFINE(wired_central_tx_work, send_pending_tx_work_cb);

static void read_timer_cb(struct k_timer *_timer) {
    zmk_split_wired_poll_in(&rx_buf, uart, &publish_events, NULL);
    // Check if we found any bytes, read some, or read all the bytes in the RX
}

static K_TIMER_DEFINE(wired_central_read_timer, read_timer_cb, NULL);

#endif

static void begin_tx(void) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)
    uart_irq_tx_enable(uart);
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC)
    zmk_split_wired_async_tx(&async_state);
#else
    k_work_submit(&wired_central_tx_work);
#endif
}

static void begin_rx(void) {
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
    pm_device_runtime_get(uart);
#elif IS_ENABLED(CONFIG_PM_DEVICE)
    pm_device_action_run(uart, PM_DEVICE_ACTION_RESUME);
#endif // IS_ENABLED(CONFIG_PM_DEVICE)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)
    uart_irq_rx_enable(uart);
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC)
    zmk_split_wired_async_rx(&async_state);
#else
    k_timer_start(&wired_central_read_timer, K_TICKS(CONFIG_ZMK_SPLIT_WIRED_POLLING_RX_PERIOD),
                  K_TICKS(CONFIG_ZMK_SPLIT_WIRED_POLLING_RX_PERIOD));
#endif
}

#if HAS_DETECT_GPIO

static void stop_rx(void) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)
    uart_irq_rx_disable(uart);
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC)
    zmk_split_wired_async_rx_cancel(&async_state);
#else
    k_timer_stop(&wired_central_read_timer);
#endif

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
    pm_device_runtime_put(uart);
#elif IS_ENABLED(CONFIG_PM_DEVICE)
    pm_device_action_run(uart, PM_DEVICE_ACTION_SUSPEND);
#endif // IS_ENABLED(CONFIG_PM_DEVICE)
}

#endif // HAS_DETECT_GPIO

static ssize_t get_payload_data_size(const struct zmk_split_transport_central_command *cmd) {
    switch (cmd->type) {
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_POLL_EVENTS:
        return 0;
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_INVOKE_BEHAVIOR:
        return sizeof(cmd->data.invoke_behavior);
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_SET_PHYSICAL_LAYOUT:
        return sizeof(cmd->data.set_physical_layout);
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_SET_HID_INDICATORS:
        return sizeof(cmd->data.set_hid_indicators);
    default:
        return -ENOTSUP;
    }
}

static int split_central_wired_send_command(uint8_t source,
                                            struct zmk_split_transport_central_command cmd) {
    if (source != 0) {
        return -EINVAL;
    }

    ssize_t data_size = get_payload_data_size(&cmd);
    if (data_size < 0) {
        LOG_WRN("Failed to determine payload data size %d", data_size);
        return data_size;
    }

    // Data + type + source
    size_t payload_size =
        data_size + sizeof(source) + sizeof(enum zmk_split_transport_central_command_type);

    if (ring_buf_space_get(&tx_buf) < MSG_EXTRA_SIZE + payload_size) {
        LOG_WRN("No room to send command to the peripheral %d", source);
        return -ENOSPC;
    }

    struct command_envelope env = {.prefix =
                                       {
                                           .magic_prefix = ZMK_SPLIT_WIRED_ENVELOPE_MAGIC_PREFIX,
                                           .payload_size = payload_size,
                                       },
                                   .payload = {
                                       .source = source,
                                       .cmd = cmd,
                                   }};

    struct msg_postfix postfix = {.crc =
                                      crc32_ieee((void *)&env, sizeof(env.prefix) + payload_size)};

    ring_buf_put(&tx_buf, (uint8_t *)&env, sizeof(env.prefix) + payload_size);
    ring_buf_put(&tx_buf, (uint8_t *)&postfix, sizeof(postfix));

    if (can_tx() >= 0) {
        begin_tx();
    }

    return 0;
}

#if IS_HALF_DUPLEX_MODE

void rx_done_cb(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(rx_done_work, rx_done_cb);

void rx_done_cb(struct k_work *work) {
    k_sem_give(&tx_sem);

    // Poll for the next event data!
    split_central_wired_send_command(0,
                                     (struct zmk_split_transport_central_command){
                                         .type = ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_POLL_EVENTS,
                                     });

    k_work_reschedule(&rx_done_work, K_MSEC(CONFIG_ZMK_SPLIT_WIRED_HALF_DUPLEX_RX_TIMEOUT));
}

#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)

static void serial_cb(const struct device *dev, void *user_data) {

    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (uart_irq_rx_ready(dev)) {
            zmk_split_wired_fifo_read(dev, &rx_buf, &publish_events, NULL);
#if IS_HALF_DUPLEX_MODE
            k_work_reschedule(&rx_done_work,
                              K_TICKS(CONFIG_ZMK_SPLIT_WIRED_HALF_DUPLEX_RX_COMPLETE_TIMEOUT));
#endif
        }

        if (uart_irq_tx_complete(dev)) {

            if (ring_buf_size_get(&tx_buf) == 0) {
                uart_irq_tx_disable(dev);
#if HAS_DIR_GPIO
                gpio_pin_set_dt(&dir_gpio, 0);
#endif
            }
        }

        if (uart_irq_tx_ready(dev)) {
#if HAS_DIR_GPIO
            gpio_pin_set_dt(&dir_gpio, 1);
#endif
            zmk_split_wired_fifo_fill(dev, &tx_buf);
        }
    }
}

#elif IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_POLLING)

static void send_pending_tx_work_cb(struct k_work *work) {
    zmk_split_wired_poll_out(&tx_buf, uart);
}

#endif

#if HAS_DETECT_GPIO

static void notify_transport_status(void);

static struct gpio_callback detect_callback;

static void notify_status_work_cb(struct k_work *_work) { notify_transport_status(); }

static K_WORK_DEFINE(notify_status_work, notify_status_work_cb);

static void detect_pin_irq_callback_handler(const struct device *port, struct gpio_callback *cb,
                                            const gpio_port_pins_t pin) {
    k_work_submit(&notify_status_work);
}

#endif

static int zmk_split_wired_central_init(void) {
    if (!device_is_ready(uart)) {
        return -ENODEV;
    }

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
    pm_device_runtime_put(uart);
#elif IS_ENABLED(CONFIG_PM_DEVICE)
    pm_device_action_run(uart, PM_DEVICE_ACTION_SUSPEND);
#endif // IS_ENABLED(CONFIG_PM_DEVICE)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT)

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

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_WIRED_UART_MODE_*)

#if IS_HALF_DUPLEX_MODE

#if HAS_DIR_GPIO
    gpio_pin_configure_dt(&dir_gpio, GPIO_OUTPUT_INACTIVE);
#endif

#endif // IS_HALF_DUPLEX_MODE

#if HAS_DETECT_GPIO

    gpio_pin_configure_dt(&detect_gpio, GPIO_INPUT);

    gpio_init_callback(&detect_callback, detect_pin_irq_callback_handler, BIT(detect_gpio.pin));
    int err = gpio_add_callback(detect_gpio.port, &detect_callback);
    if (err) {
        LOG_ERR("Error adding the callback to the detect pin: %i", err);
        return err;
    }

    err = gpio_pin_interrupt_configure_dt(&detect_gpio, GPIO_INT_EDGE_BOTH);
    if (err < 0) {
        LOG_WRN("Failed to so configure interrupt for detection pin (%d)", err);
        return err;
    }

#endif // HAS_DETECT_GPIO

    return 0;
}

SYS_INIT(zmk_split_wired_central_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static int split_central_wired_get_available_source_ids(uint8_t *sources) {
    sources[0] = 0;

    return 1;
}

static int split_central_wired_set_enabled(bool enabled) {
    if (enabled) {
        begin_rx();
#if IS_HALF_DUPLEX_MODE
        k_work_schedule(&rx_done_work, K_MSEC(CONFIG_ZMK_SPLIT_WIRED_HALF_DUPLEX_RX_TIMEOUT));
#endif
        return 0;
#if HAS_DETECT_GPIO
    } else {
#if IS_HALF_DUPLEX_MODE
        k_work_cancel_delayable(&rx_done_work);
#endif
        stop_rx();
        return 0;
#endif
    }

    return -ENOTSUP;
}

#if HAS_DETECT_GPIO

static zmk_split_transport_central_status_changed_cb_t transport_status_cb;

static int
split_central_wired_set_status_callback(zmk_split_transport_central_status_changed_cb_t cb) {
    transport_status_cb = cb;
    return 0;
}

static struct zmk_split_transport_status split_central_wired_get_status() {
    int detected = gpio_pin_get_dt(&detect_gpio);
    if (detected > 0) {
        return (struct zmk_split_transport_status){
            .available = true,
            .enabled = true, // Track this
            .connections = ZMK_SPLIT_TRANSPORT_CONNECTIONS_STATUS_ALL_CONNECTED,

        };
    } else {
        return (struct zmk_split_transport_status){
            .available = false,
            .enabled = true, // Track this
            .connections = ZMK_SPLIT_TRANSPORT_CONNECTIONS_STATUS_DISCONNECTED,

        };
    }
}

#endif // HAS_DETECT_GPIO

static const struct zmk_split_transport_central_api central_api = {
    .send_command = split_central_wired_send_command,
    .get_available_source_ids = split_central_wired_get_available_source_ids,
    .set_enabled = split_central_wired_set_enabled,
#if HAS_DETECT_GPIO
    .set_status_callback = split_central_wired_set_status_callback,
    .get_status = split_central_wired_get_status,
#endif // HAS_DETECT_GPIO
};

ZMK_SPLIT_TRANSPORT_CENTRAL_REGISTER(wired_central, &central_api, CONFIG_ZMK_SPLIT_WIRED_PRIORITY);

#if HAS_DETECT_GPIO

static void notify_transport_status(void) {
    if (transport_status_cb) {
        transport_status_cb(&wired_central, split_central_wired_get_status());
    }
}

#endif

static void publish_events_work(struct k_work *work) {

#if IS_HALF_DUPLEX_MODE
    k_work_reschedule(&rx_done_work,
                      K_MSEC(CONFIG_ZMK_SPLIT_WIRED_HALF_DUPLEX_RX_COMPLETE_TIMEOUT));
#endif // IS_HALF_DUPLEX_MODE

    while (ring_buf_size_get(&rx_buf) > MSG_EXTRA_SIZE) {
        struct event_envelope env;
        int item_err =
            zmk_split_wired_get_item(&rx_buf, (uint8_t *)&env, sizeof(struct event_envelope));
        switch (item_err) {
        case 0:
            zmk_split_transport_central_peripheral_event_handler(&wired_central, env.payload.source,
                                                                 env.payload.event);
            break;
        case -EAGAIN:
            return;
        default:
            LOG_WRN("Issue fetching an item from the RX buffer: %d", item_err);
            return;
        }
    }
}
