/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_ps2

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ps2.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_PS2_LOG_LEVEL
LOG_MODULE_REGISTER(ps2_gpio);

/*
 * Settings
 */

#define PS2_GPIO_WRITE_MAX_RETRY 5
#define PS2_GPIO_READ_MAX_RETRY 3

#define PS2_GPIO_DATA_QUEUE_SIZE 100

// Custom queue for background PS/2 processing work at low priority
// We purposefully want this to be a fairly low priority, because
// this queue is used while we wait to start a write.
// If the system is very busy with interrupts and other threads, then we
// want to wait until that is over so that our write interrupts don't get
// missed.
#define PS2_GPIO_WORK_QUEUE_PRIORITY 10
#define PS2_GPIO_WORK_QUEUE_STACK_SIZE 1024

// Custom queue for calling the zephyr ps/2 callback.
// We don't want to hand it off to that API in an ISR since that callback
// could be using blocking functions.
// But we also don't want to hand it off at a low priority, since the PS/2
// packets must be dealt with quickly. So we use a fairly high priority.
#define PS2_GPIO_WORK_QUEUE_CB_PRIORITY 2
#define PS2_GPIO_WORK_QUEUE_CB_STACK_SIZE 1024

/*
 * PS/2 Defines
 */

#define PS2_GPIO_POS_START 0
// 1-8 are the data bits
#define PS2_GPIO_POS_PARITY 9
#define PS2_GPIO_POS_STOP 10
#define PS2_GPIO_POS_ACK 11 // Write mode only

#define PS2_GPIO_RESP_ACK 0xfa
#define PS2_GPIO_RESP_RESEND 0xfe
#define PS2_GPIO_RESP_FAILURE 0xfc

/*
 * PS/2 Timings
 */

// PS2 uses a frequency between 10 kHz and 16.7 kHz. So clocks should arrive
// within 60-100us.
#define PS2_GPIO_TIMING_SCL_CYCLE_MIN 60
#define PS2_GPIO_TIMING_SCL_CYCLE_MAX 100

// The minimum time needed to inhibit clock to start a write
// is 100us, but we triple it just in case.
#define PS2_GPIO_TIMING_SCL_INHIBITION_MIN 100
#define PS2_GPIO_TIMING_SCL_INHIBITION (3 * PS2_GPIO_TIMING_SCL_INHIBITION_MIN)

// When we start the inhibition timer for PS2_GPIO_TIMING_SCL_INHIBITION us,
// it doesn't mean it will be called after exactly that time.
// If there are higher priority threads, it will be delayed and might stay
// inhibitied much longer. So we account for that delay and add a maximum
// allowed delay.
#define PS2_GPIO_TIMING_SCL_INHIBITION_TIMER_DELAY_MAX 1000

// After inhibiting and releasing the clock, the device starts sending
// the clock. It's supposed to start immediately, but some devices
// need much longer if you are asking them to interrupt an
// ongoing read.
#define PS2_GPIO_TIMING_SCL_INHIBITION_RESP_MAX 10000

// Writes start with us inhibiting the line and then respond
// with 11 bits (start bit included in inhibition time).
// To be conservative we give it another 2 cycles to complete
#define PS2_GPIO_TIMING_WRITE_MAX_TIME                                                             \
    (PS2_GPIO_TIMING_SCL_INHIBITION + PS2_GPIO_TIMING_SCL_INHIBITION_TIMER_DELAY_MAX +             \
     PS2_GPIO_TIMING_SCL_INHIBITION_RESP_MAX + 11 * PS2_GPIO_TIMING_SCL_CYCLE_MAX +                \
     2 * PS2_GPIO_TIMING_SCL_CYCLE_MAX)

// Reads are 11bit and we give it another 2 cycles to start and stop
#define PS2_GPIO_TIMING_READ_MAX_TIME                                                              \
    (11 * PS2_GPIO_TIMING_SCL_CYCLE_MAX + 2 * PS2_GPIO_TIMING_SCL_CYCLE_MAX)

/*
 * Driver Defines
 */

// Timeout for blocking read using the zephyr PS2 ps2_read() function
// This is not a matter of PS/2 timings, but a preference of how long we let
// the user wait until we give up on reading.
#define PS2_GPIO_TIMEOUT_READ K_SECONDS(2)

// Timeout for write_byte_blocking()
#define PS2_GPIO_TIMEOUT_WRITE_BLOCKING K_USEC(PS2_GPIO_TIMING_WRITE_MAX_TIME)

// Timeout for write_byte_await_response()
// PS/2 spec says that device must respond within 20msec,
// but real life devices take much longer. Especially if
// you interrupt existing transmissions.
#define PS2_GPIO_TIMEOUT_WRITE_AWAIT_RESPONSE K_MSEC(300)

// Max time we allow the device to send the next clock signal during reads
// and writes.
// Even though PS/2 devices send the clock at most every 100us, it doesn't mean
// that the interrupts always get triggered within that time. So we allow a
// little extra time.
#define PS2_GPIO_TIMEOUT_READ_SCL K_USEC(PS2_GPIO_TIMING_SCL_CYCLE_MAX + 50)
#define PS2_GPIO_TIMEOUT_WRITE_SCL K_USEC(PS2_GPIO_TIMING_SCL_CYCLE_MAX + 50)

// But after inhibiting the clock line, sometimes clocks take a little longer
// to start. So we allow a bit more time for the first write clock.
#define PS2_GPIO_TIMEOUT_WRITE_SCL_START K_USEC(PS2_GPIO_TIMING_SCL_INHIBITION_RESP_MAX)

#define PS2_GPIO_WRITE_INHIBIT_SLC_DURATION K_USEC(PS2_GPIO_TIMING_SCL_INHIBITION)

/*
 * Global Variables
 */

typedef enum { PS2_GPIO_MODE_READ, PS2_GPIO_MODE_WRITE } ps2_gpio_mode;

// Used to keep track of blocking write status
typedef enum {
    PS2_GPIO_WRITE_STATUS_INACTIVE,
    PS2_GPIO_WRITE_STATUS_ACTIVE,
    PS2_GPIO_WRITE_STATUS_SUCCESS,
    PS2_GPIO_WRITE_STATUS_FAILURE,
} ps2_gpio_write_status;

struct ps2_gpio_data_queue_item {
    uint8_t byte;
};

struct ps2_gpio_config {
    struct gpio_dt_spec scl_gpio;
    struct gpio_dt_spec sda_gpio;
};

struct ps2_gpio_data {
    const struct device *dev;
    struct gpio_dt_spec scl_gpio; /* GPIO used for PS2 SCL line */
    struct gpio_dt_spec sda_gpio; /* GPIO used for PS2 SDA line */

    // Interrupt callback
    struct gpio_callback scl_cb_data;

    // PS2 driver interface callback
    struct k_work callback_work;
    uint8_t callback_byte;
    ps2_callback_t callback_isr;

#if IS_ENABLED(CONFIG_PS2_GPIO_ENABLE_PS2_RESEND_CALLBACK)
    ps2_resend_callback_t resend_callback_isr;
#endif /* IS_ENABLED(CONFIG_PS2_GPIO_ENABLE_PS2_RESEND_CALLBACK) */

    bool callback_enabled;

    // Queue for ps2_read()
    struct k_msgq data_queue;
    char data_queue_buffer[PS2_GPIO_DATA_QUEUE_SIZE * sizeof(struct ps2_gpio_data_queue_item)];

    ps2_gpio_mode mode;

    uint8_t cur_read_byte;
    int cur_read_pos;
    int cur_read_try;
    uint32_t last_read_cycle_cnt;
    struct k_work_delayable read_scl_timout;

    ps2_gpio_write_status cur_write_status;
    uint8_t cur_write_byte;
    int cur_write_pos;
    struct k_work_delayable write_inhibition_wait;
    struct k_work_delayable write_scl_timout;
    struct k_sem write_lock;

    bool write_awaits_resp;
    uint8_t write_awaits_resp_byte;
    struct k_sem write_awaits_resp_sem;

    struct k_work resend_cmd_work;
};

static const struct ps2_gpio_config ps2_gpio_config = {
    .scl_gpio = GPIO_DT_SPEC_INST_GET(0, scl_gpios),
    .sda_gpio = GPIO_DT_SPEC_INST_GET(0, sda_gpios),
};

static struct ps2_gpio_data ps2_gpio_data = {
    .callback_byte = 0x0,
    .callback_isr = NULL,

#if IS_ENABLED(CONFIG_PS2_GPIO_ENABLE_PS2_RESEND_CALLBACK)
    .resend_callback_isr = NULL,
#endif /* IS_ENABLED(CONFIG_PS2_GPIO_ENABLE_PS2_RESEND_CALLBACK) */

    .callback_enabled = false,
    .mode = PS2_GPIO_MODE_READ,

    .cur_read_byte = 0x0,
    .cur_read_pos = 0,
    .cur_read_try = 0,

    .cur_write_byte = 0x0,
    .cur_write_pos = 0,
    .cur_write_status = PS2_GPIO_WRITE_STATUS_INACTIVE,

    .write_awaits_resp = false,
    .write_awaits_resp_byte = 0x0,
};

K_THREAD_STACK_DEFINE(ps2_gpio_work_queue_stack_area, PS2_GPIO_WORK_QUEUE_STACK_SIZE);
static struct k_work_q ps2_gpio_work_queue;

K_THREAD_STACK_DEFINE(ps2_gpio_work_queue_cb_stack_area, PS2_GPIO_WORK_QUEUE_CB_STACK_SIZE);
static struct k_work_q ps2_gpio_work_queue_cb;

/*
 * Function Definitions
 */

int ps2_gpio_write_byte(uint8_t byte);

/*
 * Helpers functions
 */

#define PS2_GPIO_GET_BIT(data, bit_pos) ((data >> bit_pos) & 0x1)
#define PS2_GPIO_SET_BIT(data, bit_val, bit_pos) (data |= (bit_val) << bit_pos)

int ps2_gpio_get_scl() {
    const struct ps2_gpio_data *data = &ps2_gpio_data;
    int rc = gpio_pin_get_dt(&data->scl_gpio);

    return rc;
}

int ps2_gpio_get_sda() {
    const struct ps2_gpio_data *data = &ps2_gpio_data;
    int rc = gpio_pin_get_dt(&data->sda_gpio);

    return rc;
}

void ps2_gpio_set_scl(int state) {
    const struct ps2_gpio_data *data = &ps2_gpio_data;

    // LOG_INF("Setting scl to %d", state);
    gpio_pin_set_dt(&data->scl_gpio, state);
}

void ps2_gpio_set_sda(int state) {
    const struct ps2_gpio_data *data = &ps2_gpio_data;

    // LOG_INF("Seting sda to %d", state);
    gpio_pin_set_dt(&data->sda_gpio, state);
}

int ps2_gpio_set_scl_callback_enabled(bool enabled) {
    struct ps2_gpio_data *data = &ps2_gpio_data;
    int err;

    if (enabled) {
        err = gpio_pin_interrupt_configure_dt(&data->scl_gpio, (GPIO_INT_EDGE_FALLING));
        if (err) {
            LOG_ERR("failed to enable interrupt on "
                    "SCL GPIO pin (err %d)",
                    err);
            return err;
        }
    } else {
        err = gpio_pin_interrupt_configure_dt(&data->scl_gpio, (GPIO_INT_DISABLE));
        if (err) {
            LOG_ERR("failed to disable interrupt on "
                    "SCL GPIO pin (err %d)",
                    err);
            return err;
        }
    }

    return err;
}

int ps2_gpio_configure_pin_scl(gpio_flags_t flags, char *descr) {
    struct ps2_gpio_data *data = &ps2_gpio_data;
    int err;

    err = gpio_pin_configure_dt(&data->scl_gpio, flags);
    if (err) {
        LOG_ERR("failed to configure SCL GPIO pin to %s (err %d)", descr, err);
    }

    return err;
}

int ps2_gpio_configure_pin_scl_input() { return ps2_gpio_configure_pin_scl((GPIO_INPUT), "input"); }

int ps2_gpio_configure_pin_scl_output() {
    return ps2_gpio_configure_pin_scl((GPIO_OUTPUT_HIGH), "output");
}

int ps2_gpio_configure_pin_sda(gpio_flags_t flags, char *descr) {
    struct ps2_gpio_data *data = &ps2_gpio_data;
    int err;

    err = gpio_pin_configure_dt(&data->sda_gpio, flags);
    if (err) {
        LOG_ERR("failed to configure SDA GPIO pin to %s (err %d)", descr, err);
    }

    return err;
}

int ps2_gpio_configure_pin_sda_input() { return ps2_gpio_configure_pin_sda((GPIO_INPUT), "input"); }

int ps2_gpio_configure_pin_sda_output() {
    return ps2_gpio_configure_pin_sda((GPIO_OUTPUT_HIGH), "output");
}

bool ps2_gpio_get_byte_parity(uint8_t byte) {
    int byte_parity = __builtin_parity(byte);

    // gcc parity returns 1 if there is an odd number of bits in byte
    // But the PS2 protocol sets the parity bit to 0 if there is an odd number
    return !byte_parity;
}

uint8_t ps2_gpio_data_queue_get_next(uint8_t *dst_byte, k_timeout_t timeout) {
    struct ps2_gpio_data *data = &ps2_gpio_data;
    struct ps2_gpio_data_queue_item queue_data;
    int ret;

    ret = k_msgq_get(&data->data_queue, &queue_data, timeout);
    if (ret != 0) {
        LOG_WRN("Data queue timed out...");
        return -ETIMEDOUT;
    }

    *dst_byte = queue_data.byte;

    return 0;
}

void ps2_gpio_data_queue_empty() {
    struct ps2_gpio_data *data = &ps2_gpio_data;

    k_msgq_purge(&data->data_queue);
}

void ps2_gpio_data_queue_add(uint8_t byte) {
    struct ps2_gpio_data *data = &ps2_gpio_data;

    int ret;

    struct ps2_gpio_data_queue_item queue_data;
    queue_data.byte = byte;

    LOG_INF("Adding byte to data queue: 0x%x", byte);

    for (int i = 0; i < 2; i++) {
        ret = k_msgq_put(&data->data_queue, &queue_data, K_NO_WAIT);
        if (ret == 0) {
            break;
        } else {
            LOG_WRN("Data queue full. Removing oldest item.");

            uint8_t tmp_byte;
            ps2_gpio_data_queue_get_next(&tmp_byte, K_NO_WAIT);
        }
    }

    if (ret != 0) {
        LOG_ERR("Failed to add byte 0x%x to the data queue.", byte);
    }
}

void ps2_gpio_send_cmd_resend_worker(struct k_work *item) {

#if IS_ENABLED(CONFIG_PS2_GPIO_ENABLE_PS2_RESEND_CALLBACK)

    struct ps2_gpio_data *data = &ps2_gpio_data;

    // Notify the PS/2 device driver that we are requesting a resend.
    // PS/2 devices don't just resend the last byte that was sent, but the
    // entire command packet, which can be multiple bytes.
    if (data->resend_callback_isr != NULL && data->callback_enabled) {

        data->resend_callback_isr(data->dev);
    }

#endif /* IS_ENABLED(CONFIG_PS2_GPIO_ENABLE_PS2_RESEND_CALLBACK) */

    uint8_t cmd = 0xfe;
    // LOG_DBG("Requesting resend of data with command: 0x%x", cmd);
    ps2_gpio_write_byte(cmd);
}

void ps2_gpio_send_cmd_resend() {
    struct ps2_gpio_data *data = &ps2_gpio_data;

    if (k_is_in_isr()) {

        // It's important to submit this on the cb queue and not on the
        // same queue as the inhibition delay.
        // Otherwise the queue will be blocked by the semaphore and the
        // inhibition delay worker will never be called.
        k_work_submit_to_queue(&ps2_gpio_work_queue_cb, &data->resend_cmd_work);
    } else {
        ps2_gpio_send_cmd_resend_worker(NULL);
    }
}

/*
 * Interrupt logging
 *
 * Zephyr logs don't process fast enough and slow down the interrupts enough
 * to make the writes and reads fail.
 *
 * This simple logging process allows us to debug the interrupts in detail.
 */

#define PS2_GPIO_INTERRUPT_LOG_SCL_TIMEOUT K_SECONDS(1)
#define PS2_GPIO_INTERRUPT_LOG_MAX_ITEMS 1000

#if IS_ENABLED(CONFIG_PS2_GPIO_INTERRUPT_LOG_ENABLED)

#define LOG_PS2_INT(...) ps2_gpio_interrupt_log_add(__VA_ARGS__)

struct interrupt_log {
    int64_t uptime_ticks;
    char msg[50];
    int scl;
    int sda;
    ps2_gpio_mode mode;
    int pos;
};

int interrupt_log_offset = 0;
int interrupt_log_idx = 0;
struct interrupt_log interrupt_log[PS2_GPIO_INTERRUPT_LOG_MAX_ITEMS];

struct k_work_delayable interrupt_log_scl_timout;
struct k_work interrupt_log_print_worker;

void ps2_gpio_interrupt_log_add(char *msg, uint8_t *arg);
void ps2_gpio_interrupt_log_print();
void ps2_gpio_interrupt_log_clear();
void ps2_gpio_strncat_hex(char *dst, uint8_t val, size_t dst_size);
char *ps2_gpio_interrupt_log_get_mode_str();
void ps2_gpio_interrupt_log_get_pos_str(int pos, char *pos_str, int pos_str_size);

void ps2_gpio_interrupt_log_add(char *msg, uint8_t *arg) {
    struct ps2_gpio_data *data = &ps2_gpio_data;
    struct interrupt_log l;

    l.uptime_ticks = k_uptime_ticks();

    // va_list arglist;
    // va_start(arglist, format);
    // vsnprintfcb(l.msg, sizeof(l.msg) - 1, format, arglist);
    // va_end(arglist);
    strncpy(l.msg, msg, sizeof(l.msg) - 1);
    if (arg != NULL) {
        ps2_gpio_strncat_hex(l.msg, *arg, sizeof(l.msg));
    }

    l.scl = ps2_gpio_get_scl();
    l.sda = ps2_gpio_get_sda();
    l.mode = data->mode;
    if (data->mode == PS2_GPIO_MODE_READ) {
        l.pos = data->cur_read_pos;
    } else {
        l.pos = data->cur_write_pos;
    }

    if (interrupt_log_idx == (PS2_GPIO_INTERRUPT_LOG_MAX_ITEMS * 0.80)) {
        ps2_gpio_interrupt_log_print();
    } else if (interrupt_log_idx >= PS2_GPIO_INTERRUPT_LOG_MAX_ITEMS) {
        interrupt_log_offset++;
        return;
    }

    interrupt_log[interrupt_log_idx] = l;
    interrupt_log_idx += 1;
}

void ps2_gpio_interrupt_log_print() {
    // ps2_gpio_interrupt_log_print_worker(NULL);
    k_work_submit_to_queue(&ps2_gpio_work_queue_cb, &interrupt_log_print_worker);
}

void ps2_gpio_interrupt_log_print_worker(struct k_work *item) {

    LOG_INF("===== Interrupt Log =====");
    for (int i = 0; i < interrupt_log_idx; i++) {
        struct interrupt_log *l = &interrupt_log[i];
        char pos_str[50];

        ps2_gpio_interrupt_log_get_pos_str(l->pos, pos_str, sizeof(pos_str));

        LOG_INF("%d - %" PRIu64 ": %s "
                "(mode=%s, pos=%s, scl=%d, sda=%d)",
                interrupt_log_offset + i + 1, l->uptime_ticks, l->msg,
                ps2_gpio_interrupt_log_get_mode_str(), pos_str, l->scl, l->sda);
        k_sleep(K_MSEC(15));
    }
    LOG_INF("======== End Log ========");

    ps2_gpio_interrupt_log_clear();
}

void ps2_gpio_interrupt_log_clear() {
    memset(&interrupt_log, 0x0, sizeof(interrupt_log));
    interrupt_log_offset += interrupt_log_idx;
    interrupt_log_idx = 0;
}

void ps2_gpio_interrupt_log_scl_timeout(struct k_work *item) {
    // Called if there is no interrupt for
    // PS2_GPIO_INTERRUPT_LOG_SCL_TIMEOUT ms
    ps2_gpio_interrupt_log_print();
}

void ps2_gpio_strncat_hex(char *dst, uint8_t val, size_t dst_size) {
    const char hex_chars[] = "0123456789abcdef";
    char val_hex[5];

    val_hex[0] = '0';
    val_hex[1] = 'x';

    val_hex[2] = hex_chars[(val >> (4 * (1 - 0))) & 0xf];
    val_hex[3] = hex_chars[(val >> (4 * (1 - 1))) & 0xf];

    val_hex[4] = '\0';

    strncat(dst, val_hex, dst_size - strlen(dst) - 1);
}

char *ps2_gpio_interrupt_log_get_mode_str() {
    struct ps2_gpio_data *data = &ps2_gpio_data;

    if (data->mode == PS2_GPIO_MODE_READ) {
        return "r";
    } else if (data->mode == PS2_GPIO_MODE_WRITE) {
        return "w";
    } else {
        return "?";
    }
}

void ps2_gpio_interrupt_log_get_pos_str(int pos, char *pos_str, int pos_str_size) {
    char *pos_names[] = {
        "start",  "data_1", "data_2", "data_3", "data_4", "data_5",
        "data_6", "data_7", "data_8", "parity", "stop",   "ack",
    };

    if (pos >= (sizeof(pos_names) / sizeof(pos_names[0]))) {
        snprintf(pos_str, pos_str_size - 1, "%d", pos);
    } else {
        strncpy(pos_str, pos_names[pos], pos_str_size - 1);
    }
}

#else

#define LOG_PS2_INT(...)

#endif /* IS_ENABLED(CONFIG_PS2_GPIO_INTERRUPT_LOG_ENABLED) */

/*
 * Reading PS/2 data
 */

void ps2_gpio_read_interrupt_handler();
void ps2_gpio_read_scl_timeout(struct k_work *item);
void ps2_gpio_read_abort(bool should_resend, char *reason);
void ps2_gpio_read_process_received_byte(uint8_t byte);
void ps2_gpio_read_finish();

// Reading doesn't need to be initiated. It happens automatically whenever
// the device sends data.
// Once a full byte has been received successfully it is processed in
// ps2_gpio_read_process_received_byte, which decides what should happen
// with it.
void ps2_gpio_read_interrupt_handler() {
    struct ps2_gpio_data *data = &ps2_gpio_data;

    uint32_t cur_read_cycle_cnt = k_cycle_get_32();
    uint32_t last_read_cycle_cnt = data->last_read_cycle_cnt;

    data->last_read_cycle_cnt = cur_read_cycle_cnt;

    if (data->cur_read_pos > 0) {
        uint32_t prev_cycle_delta_us =
            k_cyc_to_us_floor32(cur_read_cycle_cnt - last_read_cycle_cnt);

        if (prev_cycle_delta_us > PS2_GPIO_TIMING_SCL_CYCLE_MAX) {
            ps2_gpio_read_abort(true, "missed interrupt");
        }
    }

    k_work_cancel_delayable(&data->read_scl_timout);

    LOG_PS2_INT("Read interrupt", NULL);

    int sda_val = ps2_gpio_get_sda();

    if (data->cur_read_pos == PS2_GPIO_POS_START) {
        // The first bit of every transmission should be 0.
        // If it is not, it means we are out of sync with the device.
        // So we abort the transmission and start from scratch.
        if (sda_val != 0) {
            LOG_PS2_INT("Ignoring read interrupt due to invalid start bit.", NULL);

            // We don't request a resend here, because sometimes after writes
            // devices send some unintended interrupts. If this is a "real
            // transmission" and we are out of sync, we will catch it with the
            // parity and stop bits and then request a resend.
            ps2_gpio_read_abort(false, "invalid start bit");
            return;
        }
    } else if (data->cur_read_pos > PS2_GPIO_POS_START &&
               data->cur_read_pos < PS2_GPIO_POS_PARITY) { // Data Bits

        // Current position, minus start bit
        int bit_pos = data->cur_read_pos - 1;
        PS2_GPIO_SET_BIT(data->cur_read_byte, sda_val, bit_pos);
    } else if (data->cur_read_pos == PS2_GPIO_POS_PARITY) {
        bool read_byte_parity = ps2_gpio_get_byte_parity(data->cur_read_byte);

        if (read_byte_parity != sda_val) {
            LOG_PS2_INT("Requesting re-send due to invalid parity bit.", NULL);

            // If we got to the parity bit and it's incorrect then we
            // are definitly in a transmission and out of sync. So we
            // request a resend.
            ps2_gpio_read_abort(true, "invalid parity bit");
            return;
        }
    } else if (data->cur_read_pos == PS2_GPIO_POS_STOP) {
        if (sda_val != 1) {
            LOG_PS2_INT("Requesting re-send due to invalid stop bit.", NULL);

            // If we got to the stop bit and it's incorrect then we
            // are definitly in a transmission and out of sync. So we
            // request a resend.
            ps2_gpio_read_abort(true, "invalid stop bit");
            return;
        }

        ps2_gpio_read_process_received_byte(data->cur_read_byte);

        return;
    } else {
        LOG_PS2_INT("Invalid read clock triggered", NULL);

        return;
    }

    data->cur_read_pos += 1;
    k_work_schedule(&data->read_scl_timout, PS2_GPIO_TIMEOUT_READ_SCL);
}

void ps2_gpio_read_scl_timeout(struct k_work *item) {
    // Once we are receiving a transmission we expect the device to
    // to send a new clock/interrupt within 100us.
    // If we don't receive the next interrupt within that timeframe,
    // we abort the read.
    struct k_work_delayable *d_work = k_work_delayable_from_work(item);
    struct ps2_gpio_data *data = CONTAINER_OF(d_work, struct ps2_gpio_data, read_scl_timout);

    LOG_PS2_INT("Read SCL timeout", NULL);

    // We don't request a resend if the timeout happens in the early
    // stage of the transmission.
    //
    // Because, sometimes after writes devices send some unintended
    // interrupts and start the "real response" after one or two cycles.
    //
    // If we are really out of sync the parity and stop bits should catch
    // it and request a re-transmission.
    if (data->cur_read_pos <= 3) {
        ps2_gpio_read_abort(false, "scl timeout");
    } else {
        ps2_gpio_read_abort(true, "scl timeout");
    }
}

void ps2_gpio_read_abort(bool should_resend, char *reason) {
    struct ps2_gpio_data *data = &ps2_gpio_data;

    if (should_resend == true) {
        LOG_ERR("Aborting read with resend request on pos=%d: %s", data->cur_read_pos, reason);
        LOG_PS2_INT("Aborting read with resend request.", NULL);
    } else {
        LOG_PS2_INT("Aborting read without resend request.", NULL);
    }

    ps2_gpio_read_finish();

    k_work_cancel_delayable(&data->read_scl_timout);

    if (should_resend == true) {
        if (data->cur_read_try < PS2_GPIO_READ_MAX_RETRY) {

            data->cur_read_try++;
            ps2_gpio_send_cmd_resend();
        } else {
            LOG_ERR("Failed to read value %d times. Stopping asking the device "
                    "to resend.",
                    data->cur_read_try);

            data->cur_read_try = 0;
        }
    }
}

void ps2_gpio_read_process_received_byte(uint8_t byte) {
    struct ps2_gpio_data *data = &ps2_gpio_data;

    LOG_DBG("Successfully received value: 0x%x", byte);
    LOG_PS2_INT("Successfully received value: ", &byte);

    // Since read was successful we reset the read try
    data->cur_read_try = 0;
    ps2_gpio_read_finish();

    // If write_byte_await_response() is waiting, we notify
    // the blocked write process of whether it was a success or not.
    if (data->write_awaits_resp) {
        data->write_awaits_resp_byte = byte;
        data->write_awaits_resp = false;
        k_sem_give(&data->write_awaits_resp_sem);

        // Don't send ack and err responses to the callback and read
        // data queue.
        // If it's an ack, the write process will return success.
        // If it's an error, the write process will return failure.
        if (byte == PS2_GPIO_RESP_ACK || byte == PS2_GPIO_RESP_RESEND ||
            byte == PS2_GPIO_RESP_FAILURE) {

            return;
        }
    }

    // If no callback is set, we add the data to a fifo queue
    // that can be read later with the read using `ps2_read`
    if (data->callback_isr != NULL && data->callback_enabled) {

        // Call callback from a worker to make sure the callback
        // doesn't block the interrupt.
        // Will call ps2_gpio_read_callback_work_handler
        data->callback_byte = byte;
        k_work_submit_to_queue(&ps2_gpio_work_queue_cb, &data->callback_work);
    } else {
        ps2_gpio_data_queue_add(byte);
    }
}

void ps2_gpio_read_callback_work_handler(struct k_work *work) {
    struct ps2_gpio_data *data = &ps2_gpio_data;

    data->callback_isr(data->dev, data->callback_byte);
    data->callback_byte = 0x0;
}

void ps2_gpio_read_finish() {
    struct ps2_gpio_data *data = &ps2_gpio_data;

    data->cur_read_pos = PS2_GPIO_POS_START;
    data->cur_read_byte = 0x0;

    k_work_cancel_delayable(&data->read_scl_timout);
}

/*
 * Writing PS2 data
 */

int ps2_gpio_write_byte_await_response(uint8_t byte);
int ps2_gpio_write_byte_blocking(uint8_t byte);
int ps2_gpio_write_byte_start(uint8_t byte);
void ps2_gpio_write_inhibition_wait(struct k_work *item);
void ps2_gpio_write_interrupt_handler();
void ps2_gpio_write_finish(bool successful, char *descr);
void ps2_gpio_write_scl_timeout(struct k_work *item);
bool ps2_gpio_get_byte_parity(uint8_t byte);

// Returned when there was an error writing to the PS2 device, such
// as not getting a clock from the device or receiving an invalid
// ack bit.
#define PS2_GPIO_E_WRITE_TRANSMIT 1

// Returned when the semaphore times out. Theoretically this shouldn't be
// happening. But it can happen if the same thread is used for both the
// semaphore wait and the inhibition timeout.
#define PS2_GPIO_E_WRITE_SEM_TIMEOUT 2

// Returned when the write finished seemingly successful, but the
// device didn't send a response in time.
#define PS2_GPIO_E_WRITE_RESPONSE 3

// Returned when the write finished seemingly successful, but the
// device responded with 0xfe (request to resend) and we ran out of
// retry attempts.
#define PS2_GPIO_E_WRITE_RESEND 4

// Returned when the write finished seemingly successful, but the
// device responded with 0xfc (failure / cancel).
#define PS2_GPIO_E_WRITE_FAILURE 5

K_MUTEX_DEFINE(ps2_gpio_write_mutex);

int ps2_gpio_write_byte(uint8_t byte) {
    int err;

    LOG_DBG("\n");
    LOG_DBG("START WRITE: 0x%x", byte);

    k_mutex_lock(&ps2_gpio_write_mutex, K_FOREVER);

    for (int i = 0; i < PS2_GPIO_WRITE_MAX_RETRY; i++) {
        if (i > 0) {
            LOG_WRN("Attempting write re-try #%d of %d...", i + 1, PS2_GPIO_WRITE_MAX_RETRY);
        }

        err = ps2_gpio_write_byte_await_response(byte);

        if (err == 0) {
            if (i > 0) {
                LOG_WRN("Successfully wrote 0x%x on try #%d of %d...", byte, i + 1,
                        PS2_GPIO_WRITE_MAX_RETRY);
            }
            break;
        } else if (err == PS2_GPIO_E_WRITE_FAILURE) {
            // Write failed and the device requested to stop trying
            // to resend.
            break;
        }
    }

    LOG_DBG("END WRITE: 0x%x\n", byte);
    k_mutex_unlock(&ps2_gpio_write_mutex);

    return err;
}

// Writes the byte and blocks execution until we read the
// response byte.
// Returns failure if the write fails or the response is 0xfe/0xfc (error)
// Returns success if the response is 0xfa (ack) or any value except of
// 0xfe.
// 0xfe, 0xfc and 0xfa are not passed on to the read data queue or callback.
int ps2_gpio_write_byte_await_response(uint8_t byte) {
    struct ps2_gpio_data *data = &ps2_gpio_data;
    int err;

    err = ps2_gpio_write_byte_blocking(byte);
    if (err) {
        return err;
    }

    data->write_awaits_resp = true;

    err = k_sem_take(&data->write_awaits_resp_sem, PS2_GPIO_TIMEOUT_WRITE_AWAIT_RESPONSE);

    uint8_t resp_byte = data->write_awaits_resp_byte;
    data->write_awaits_resp_byte = 0x0;
    data->write_awaits_resp = false;

    if (err) {
        LOG_WRN("Write response didn't arrive in time for byte "
                "0x%x. Considering send a failure.",
                byte);

        return PS2_GPIO_E_WRITE_RESPONSE;
    }

    LOG_DBG("Write for byte 0x%x received response: 0x%x", byte, resp_byte);

    // We fail the write since we got an error response
    if (resp_byte == PS2_GPIO_RESP_RESEND) {

        return PS2_GPIO_E_WRITE_RESEND;
    } else if (resp_byte == PS2_GPIO_RESP_FAILURE) {

        return PS2_GPIO_E_WRITE_FAILURE;
    }

    // Most of the time when a write was successful the device
    // responds with an 0xfa (ack), but for some commands it doesn't.
    // So we consider all non-0xfe and 0xfc responses as successful.
    return 0;
}

int ps2_gpio_write_byte_blocking(uint8_t byte) {
    struct ps2_gpio_data *data = &ps2_gpio_data;
    int err;

    // LOG_DBG("ps2_gpio_write_byte_blocking called with byte=0x%x", byte);

    err = ps2_gpio_write_byte_start(byte);
    if (err) {
        LOG_ERR("Could not initiate writing of byte.");
        return PS2_GPIO_E_WRITE_TRANSMIT;
    }

    // The async `write_byte_start` function takes the only available semaphor.
    // This causes the `k_sem_take` call below to block until
    // `ps2_gpio_write_finish` gives it back.
    err = k_sem_take(&data->write_lock, PS2_GPIO_TIMEOUT_WRITE_BLOCKING);
    if (err) {

        // This usually means the controller is busy with other interrupts,
        // timed out processing the interrupts and even the scl timeout
        // delayable wasn't called due to the delay.
        //
        // So we abort the write and try again.
        LOG_ERR("Blocking write failed due to semaphore timeout for byte "
                "0x%x: %d",
                byte, err);

        ps2_gpio_write_finish(false, "semaphore timeout");
        return PS2_GPIO_E_WRITE_SEM_TIMEOUT;
    }

    if (data->cur_write_status == PS2_GPIO_WRITE_STATUS_SUCCESS) {
        // LOG_DBG("Blocking write finished successfully for byte 0x%x", byte);
        err = 0;
    } else {
        LOG_ERR("Blocking write finished with failure for byte 0x%x status: %d", byte,
                data->cur_write_status);
        err = -data->cur_write_status;
    }

    data->cur_write_status = PS2_GPIO_WRITE_STATUS_INACTIVE;

    return err;
}

int ps2_gpio_write_byte_start(uint8_t byte) {
    struct ps2_gpio_data *data = &ps2_gpio_data;
    int err;

    LOG_DBG("ps2_gpio_write_byte_start called with byte=0x%x", byte);

    if (data->mode == PS2_GPIO_MODE_WRITE) {
        LOG_ERR("Preventing write off byte 0x%x: "
                "Another write in progress for 0x%x",
                byte, data->cur_write_byte);

        return -EBUSY;
    }

    // Take semaphore so that when `ps2_gpio_write_byte_blocking` attempts
    // taking it, the process gets blocked.
    // It is released in `ps2_gpio_write_finish`.
    err = k_sem_take(&data->write_lock, K_NO_WAIT);
    if (err != 0 && err != -EBUSY) {
        LOG_ERR("ps2_gpio_write_byte_start could not take semaphore: %d", err);

        return err;
    }

    // Change mode and set write_pos so that the read interrupt handler
    // doesn't trigger when we bring the clock line low.
    data->mode = PS2_GPIO_MODE_WRITE;
    data->cur_write_pos = PS2_GPIO_POS_START;
    data->cur_write_byte = byte;

    // Initiating a send aborts any in-progress reads, so we
    // reset the current read byte
    data->cur_write_status = PS2_GPIO_WRITE_STATUS_ACTIVE;
    if (data->cur_read_pos != PS2_GPIO_POS_START || data->cur_read_byte != 0x0) {
        LOG_WRN("Aborting in-progress read due to write of byte 0x%x", byte);
        ps2_gpio_read_abort(false, "starting write");
    }

    // Configure data and clock lines for output
    ps2_gpio_set_scl_callback_enabled(false);
    ps2_gpio_configure_pin_scl_output();
    ps2_gpio_configure_pin_sda_output();

    LOG_PS2_INT("Starting write of byte ", &byte);

    // Inhibit the line by setting clock low and data high
    ps2_gpio_set_scl(0);
    ps2_gpio_set_sda(1);

    LOG_PS2_INT("Inhibited clock line", NULL);

    // Keep the line inhibited for at least 100 microseconds
    k_work_schedule_for_queue(&ps2_gpio_work_queue, &data->write_inhibition_wait,
                              PS2_GPIO_WRITE_INHIBIT_SLC_DURATION);

    // The code continues in ps2_gpio_write_inhibition_wait
    return 0;
}

void ps2_gpio_write_inhibition_wait(struct k_work *item) {
    LOG_PS2_INT("Inhibition timer finished", NULL);

    struct k_work_delayable *d_work = k_work_delayable_from_work(item);
    struct ps2_gpio_data *data = CONTAINER_OF(d_work, struct ps2_gpio_data, write_inhibition_wait);

    // Enable the scl interrupt again
    ps2_gpio_set_scl_callback_enabled(true);

    // Set data to value of start bit
    ps2_gpio_set_sda(0);

    LOG_PS2_INT("Set sda to start bit", NULL);

    // The start bit was sent by setting sda to low
    // So the next scl interrupt will be for the first
    // data bit.
    data->cur_write_pos += 1;

    // Release the clock line and configure it as input
    // This let's the device take control of the clock again
    ps2_gpio_set_scl(1);
    ps2_gpio_configure_pin_scl_input();

    LOG_PS2_INT("Released clock", NULL);

    k_work_schedule_for_queue(&ps2_gpio_work_queue, &data->write_scl_timout,
                              PS2_GPIO_TIMEOUT_WRITE_SCL_START);

    // From here on the device takes over the control of the clock again
    // Every time it is ready for the next bit to be trasmitted, it will...
    //  - Pull the clock line low
    //  - Which will trigger our `ps2_gpio_write_interrupt_handler`
    //  - Which will send the correct bit
    //  - After all bits are sent `ps2_gpio_write_finish` is called
}

void ps2_gpio_write_interrupt_handler() {
    // After initiating writing, the device takes over
    // the clock and asks us for a new bit of data on
    // each falling edge.
    struct ps2_gpio_data *data = &ps2_gpio_data;

    if (data->cur_write_pos == PS2_GPIO_POS_START) {
        // This should not be happening, because the PS2_GPIO_POS_START bit
        // is sent in ps2_gpio_write_byte_start during inhibition
        LOG_PS2_INT("Write interrupt", NULL);
        return;
    }

    k_work_cancel_delayable(&data->write_scl_timout);

    if (data->cur_write_pos > PS2_GPIO_POS_START && data->cur_write_pos < PS2_GPIO_POS_PARITY) {
        // Set it to the data bit corresponding to the current
        // write position (subtract start bit postion)
        ps2_gpio_set_sda(PS2_GPIO_GET_BIT(data->cur_write_byte, (data->cur_write_pos - 1)));
    } else if (data->cur_write_pos == PS2_GPIO_POS_PARITY) {
        ps2_gpio_set_sda(ps2_gpio_get_byte_parity(data->cur_write_byte));
    } else if (data->cur_write_pos == PS2_GPIO_POS_STOP) {
        // Send the stop bit (always 1)
        ps2_gpio_set_sda(1);

        // Give control over data pin back to device after sending stop bit
        // so that we can receive the ack bit from the device
        ps2_gpio_configure_pin_sda_input();
    } else if (data->cur_write_pos == PS2_GPIO_POS_ACK) {
        int ack_val = ps2_gpio_get_sda();

        LOG_PS2_INT("Write interrupt", NULL);

        if (ack_val == 0) {
            LOG_PS2_INT("Write was successful on ack: ", NULL);
            ps2_gpio_write_finish(true, "valid ack bit");
        } else {
            LOG_PS2_INT("Write failed on ack", NULL);
            ps2_gpio_write_finish(false, "invalid ack bit");
        }

        return;
    } else {
        LOG_PS2_INT("Invalid write clock triggered", NULL);

        return;
    }

    LOG_PS2_INT("Write interrupt", NULL);

    data->cur_write_pos += 1;
    k_work_schedule_for_queue(&ps2_gpio_work_queue, &data->write_scl_timout,
                              PS2_GPIO_TIMEOUT_WRITE_SCL);
}

void ps2_gpio_write_scl_timeout(struct k_work *item) {
    // Once we start a transmission we expect the device to
    // to send a new clock/interrupt within 100us.
    // If we don't receive the next interrupt within that timeframe,
    // we abort the writ).

    LOG_PS2_INT("Write SCL timeout", NULL);
    ps2_gpio_write_finish(false, "scl timeout");
}

void ps2_gpio_write_finish(bool successful, char *descr) {
    struct ps2_gpio_data *data = &ps2_gpio_data;

    k_work_cancel_delayable(&data->write_scl_timout);

    if (successful) {
        LOG_DBG("Successfully wrote value 0x%x", data->cur_write_byte);
        LOG_PS2_INT("Successfully wrote value ", &data->cur_write_byte);
        data->cur_write_status = PS2_GPIO_WRITE_STATUS_SUCCESS;
    } else { // Failure
        LOG_ERR("Failed to write value 0x%x at pos=%d: %s", data->cur_write_byte,
                data->cur_write_pos, descr);
        LOG_PS2_INT("Failed to write value ", &data->cur_write_byte);

        data->cur_write_status = PS2_GPIO_WRITE_STATUS_FAILURE;

        // Make sure the scl callback is enabled
        // It's possible that all threads are busy,
        // write_inhibition_wait doesn't get called in time
        // and the semaphore times out.
        // In that case we want to make sure the interrupt
        // callback is enabled again.
        ps2_gpio_set_scl_callback_enabled(true);
    }

    data->mode = PS2_GPIO_MODE_READ;
    data->cur_read_pos = PS2_GPIO_POS_START;
    data->cur_write_pos = PS2_GPIO_POS_START;
    data->cur_write_byte = 0x0;

    // Give back control over data and clock line if we still hold on to it
    ps2_gpio_configure_pin_sda_input();
    ps2_gpio_configure_pin_scl_input();

    // Give the semaphore to allow write_byte_blocking to continue
    k_sem_give(&data->write_lock);
}

/*
 * Interrupt Handler
 */

void ps2_gpio_scl_interrupt_handler(const struct device *dev, struct gpio_callback *cb,
                                    uint32_t pins) {
    struct ps2_gpio_data *data = &ps2_gpio_data;

#if IS_ENABLED(CONFIG_PS2_GPIO_INTERRUPT_LOG_ENABLED)
    k_work_cancel_delayable(&interrupt_log_scl_timout);
#endif /* IS_ENABLED(CONFIG_PS2_GPIO_INTERRUPT_LOG_ENABLED) */

    if (data->mode == PS2_GPIO_MODE_READ) {
        ps2_gpio_read_interrupt_handler();
    } else {
        ps2_gpio_write_interrupt_handler();
    }

#if IS_ENABLED(CONFIG_PS2_GPIO_INTERRUPT_LOG_ENABLED)
    k_work_schedule_for_queue(&ps2_gpio_work_queue_cb, &interrupt_log_scl_timout,
                              PS2_GPIO_INTERRUPT_LOG_SCL_TIMEOUT);
#endif /* IS_ENABLED(CONFIG_PS2_GPIO_INTERRUPT_LOG_ENABLED) */
}

/*
 * Zephyr PS/2 driver interface
 */
static int ps2_gpio_enable_callback(const struct device *dev);

#if IS_ENABLED(CONFIG_PS2_GPIO_ENABLE_PS2_RESEND_CALLBACK)

static int ps2_gpio_configure(const struct device *dev, ps2_callback_t callback_isr,
                              ps2_resend_callback_t resend_callback_isr) {
    struct ps2_gpio_data *data = dev->data;

    if (!callback_isr && !resend_callback_isr) {
        return -EINVAL;
    }

    if (callback_isr) {
        data->callback_isr = callback_isr;
        ps2_gpio_enable_callback(dev);
    }

    if (resend_callback_isr) {
        data->resend_callback_isr = resend_callback_isr;
    }

    return 0;
}

#else

static int ps2_gpio_configure(const struct device *dev, ps2_callback_t callback_isr) {
    struct ps2_gpio_data *data = dev->data;

    if (!callback_isr) {
        return -EINVAL;
    }

    data->callback_isr = callback_isr;
    ps2_gpio_enable_callback(dev);

    return 0;
}

#endif /* IS_ENABLED(CONFIG_PS2_GPIO_ENABLE_PS2_RESEND_CALLBACK) */

int ps2_gpio_read(const struct device *dev, uint8_t *value) {
    // TODO: Add a way to not return old queue items
    // Maybe only bytes that were received within past 10 seconds.
    uint8_t queue_byte;
    int err = ps2_gpio_data_queue_get_next(&queue_byte, PS2_GPIO_TIMEOUT_READ);
    if (err) { // Timeout due to no data to read in data queue
        // LOG_DBG("ps2_gpio_read: Fifo timed out...");

        return -ETIMEDOUT;
    }

    // LOG_DBG("ps2_gpio_read: Returning 0x%x", queue_byte);
    *value = queue_byte;

    return 0;
}

static int ps2_gpio_write(const struct device *dev, uint8_t value) {
    return ps2_gpio_write_byte(value);
}

static int ps2_gpio_disable_callback(const struct device *dev) {
    struct ps2_gpio_data *data = dev->data;

    // Make sure there are no stale items in the data queue
    // from before the callback was disabled.
    ps2_gpio_data_queue_empty();

    data->callback_enabled = false;

    // LOG_DBG("Disabled PS2 callback.");

    return 0;
}

static int ps2_gpio_enable_callback(const struct device *dev) {
    struct ps2_gpio_data *data = dev->data;
    data->callback_enabled = true;

    // LOG_DBG("Enabled PS2 callback.");

    ps2_gpio_data_queue_empty();

    return 0;
}

static const struct ps2_driver_api ps2_gpio_driver_api = {
    .config = ps2_gpio_configure,
    .read = ps2_gpio_read,
    .write = ps2_gpio_write,
    .disable_callback = ps2_gpio_disable_callback,
    .enable_callback = ps2_gpio_enable_callback,
};

/*
 * PS/2 GPIO Driver Init
 */

static int ps2_gpio_init_gpio(void) {
    struct ps2_gpio_data *data = &ps2_gpio_data;
    struct ps2_gpio_config *config = (struct ps2_gpio_config *)&ps2_gpio_config;
    int err;

    // Make pin info accessible through the data struct
    data->scl_gpio = config->scl_gpio;
    data->sda_gpio = config->sda_gpio;

    // Overwrite any user-provided flags from the devicetree
    data->scl_gpio.dt_flags = 0;
    data->scl_gpio.dt_flags = 0;

    // Setup interrupt callback for clock line
    gpio_init_callback(&data->scl_cb_data, ps2_gpio_scl_interrupt_handler, BIT(data->scl_gpio.pin));

    err = gpio_add_callback(config->scl_gpio.port, &data->scl_cb_data);
    if (err) {
        LOG_ERR("failed to enable interrupt callback on "
                "SCL GPIO pin (err %d)",
                err);
    }

    ps2_gpio_set_scl_callback_enabled(true);
    ps2_gpio_configure_pin_scl_input();
    ps2_gpio_configure_pin_sda_input();

    // Check if this stuff is needed
    // TODO: Figure out why this is requiered.
    ps2_gpio_set_sda(1);
    ps2_gpio_set_scl(1);

    return err;
}

static int ps2_gpio_init(const struct device *dev) {

    struct ps2_gpio_data *data = dev->data;

    // Set the ps2 device so we can retrieve it later for
    // the ps2 callback
    data->dev = dev;

    ps2_gpio_init_gpio();

    // Init data queue for synchronous read operations
    k_msgq_init(&data->data_queue, data->data_queue_buffer, sizeof(struct ps2_gpio_data_queue_item),
                PS2_GPIO_DATA_QUEUE_SIZE);

    // Init semaphore for blocking writes
    k_sem_init(&data->write_lock, 0, 1);

    // Init semaphore that waits for read after write
    k_sem_init(&data->write_awaits_resp_sem, 0, 1);

    // Custom queue for background PS/2 processing work at high priority
    k_work_queue_start(&ps2_gpio_work_queue, ps2_gpio_work_queue_stack_area,
                       K_THREAD_STACK_SIZEOF(ps2_gpio_work_queue_stack_area),
                       PS2_GPIO_WORK_QUEUE_PRIORITY, NULL);

    // Custom queue for calling the zephyr ps/2 callback at lower priority
    k_work_queue_start(&ps2_gpio_work_queue_cb, ps2_gpio_work_queue_cb_stack_area,
                       K_THREAD_STACK_SIZEOF(ps2_gpio_work_queue_cb_stack_area),
                       PS2_GPIO_WORK_QUEUE_CB_PRIORITY, NULL);

    // Timeouts for clock pulses during read and write
    k_work_init_delayable(&data->read_scl_timout, ps2_gpio_read_scl_timeout);
    k_work_init_delayable(&data->write_scl_timout, ps2_gpio_write_scl_timeout);
    k_work_init_delayable(&data->write_inhibition_wait, ps2_gpio_write_inhibition_wait);

#if IS_ENABLED(CONFIG_PS2_GPIO_INTERRUPT_LOG_ENABLED)
    k_work_init_delayable(&interrupt_log_scl_timout, ps2_gpio_interrupt_log_scl_timeout);
    k_work_init(&interrupt_log_print_worker, ps2_gpio_interrupt_log_print_worker);
#endif /* IS_ENABLED(CONFIG_PS2_GPIO_INTERRUPT_LOG_ENABLED) */

    k_work_init(&data->callback_work, ps2_gpio_read_callback_work_handler);
    k_work_init(&data->resend_cmd_work, ps2_gpio_send_cmd_resend_worker);

    return 0;
}

DEVICE_DT_INST_DEFINE(0, &ps2_gpio_init, NULL, &ps2_gpio_data, &ps2_gpio_config, POST_KERNEL,
                      CONFIG_PS2_INIT_PRIORITY, &ps2_gpio_driver_api);
