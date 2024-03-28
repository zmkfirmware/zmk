/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zmk_input_mouse_ps2

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/ps2.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/*
 * Settings
 */

// Delay mouse init to give the mouse time to send the init sequence.
#define ZMK_MOUSE_PS2_INIT_THREAD_DELAY_MS 1000

// How often the driver try to initialize a mouse before we give up.
#define MOUSE_PS2_INIT_ATTEMPTS 10

// Mouse activity packets are at least three bytes.
// This defines how much time between bytes can pass before
// we give up on the packet and start fresh.
#define MOUSE_PS2_TIMEOUT_ACTIVITY_PACKET K_MSEC(500)

/*
 * PS/2 Defines
 */

// According to the `IBM TrackPoint System Version 4.0 Engineering
// Specification`...
// "The POR shall be timed to occur 600 ms ± 20 % from the time power is
//  applied to the TrackPoint controller."
#define MOUSE_PS2_POWER_ON_RESET_TIME K_MSEC(600)

// Common PS/2 Mouse commands
#define MOUSE_PS2_CMD_GET_SECONDARY_ID "\xe1"
#define MOUSE_PS2_CMD_GET_SECONDARY_ID_RESP_LEN 2

#define MOUSE_PS2_CMD_GET_DEVICE_ID "\xf2"
#define MOUSE_PS2_CMD_GET_DEVICE_ID_RESP_LEN 1

#define MOUSE_PS2_CMD_SET_SAMPLING_RATE "\xf3"
#define MOUSE_PS2_CMD_SET_SAMPLING_RATE_RESP_LEN 0
#define MOUSE_PS2_CMD_SET_SAMPLING_RATE_DEFAULT 100

#define MOUSE_PS2_CMD_ENABLE_REPORTING "\xf4"
#define MOUSE_PS2_CMD_ENABLE_REPORTING_RESP_LEN 0

#define MOUSE_PS2_CMD_DISABLE_REPORTING "\xf5"
#define MOUSE_PS2_CMD_DISABLE_REPORTING_RESP_LEN 0

#define MOUSE_PS2_CMD_RESEND "\xfe"
#define MOUSE_PS2_CMD_RESEND_RESP_LEN 0

#define MOUSE_PS2_CMD_RESET "\xff"
#define MOUSE_PS2_CMD_RESET_RESP_LEN 0

// Trackpoint Commands
// They can be found in the `IBM TrackPoint System Version 4.0 Engineering
// Specification` (YKT3Eext.pdf)...

#define MOUSE_PS2_CMD_TP_GET_CONFIG_BYTE "\xe2\x80\x2c"
#define MOUSE_PS2_CMD_TP_GET_CONFIG_BYTE_RESP_LEN 1

#define MOUSE_PS2_CMD_TP_SET_CONFIG_BYTE "\xe2\x81\x2c"
#define MOUSE_PS2_CMD_TP_SET_CONFIG_BYTE_RESP_LEN 0

#define MOUSE_PS2_ST_TP_SENSITIVITY "tp_sensitivity"
#define MOUSE_PS2_CMD_TP_GET_SENSITIVITY "\xe2\x80\x4a"
#define MOUSE_PS2_CMD_TP_GET_SENSITIVITY_RESP_LEN 1

#define MOUSE_PS2_CMD_TP_SET_SENSITIVITY "\xe2\x81\x4a"
#define MOUSE_PS2_CMD_TP_SET_SENSITIVITY_RESP_LEN 0
#define MOUSE_PS2_CMD_TP_SET_SENSITIVITY_MIN 0
#define MOUSE_PS2_CMD_TP_SET_SENSITIVITY_MAX 255
#define MOUSE_PS2_CMD_TP_SET_SENSITIVITY_DEFAULT 128

#define MOUSE_PS2_ST_TP_NEG_INERTIA "tp_neg_inertia"
#define MOUSE_PS2_CMD_TP_GET_NEG_INERTIA "\xe2\x80\x4d"
#define MOUSE_PS2_CMD_TP_GET_NEG_INERTIA_RESP_LEN 1

#define MOUSE_PS2_CMD_TP_SET_NEG_INERTIA "\xe2\x81\x4d"
#define MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_RESP_LEN 0
#define MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_MIN 0
#define MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_MAX 255
#define MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_DEFAULT 0x06

#define MOUSE_PS2_ST_TP_VALUE6 "tp_value6"
#define MOUSE_PS2_CMD_TP_GET_VALUE6_UPPER_PLATEAU_SPEED "\xe2\x80\x60"
#define MOUSE_PS2_CMD_TP_GET_VALUE6_UPPER_PLATEAU_SPEED_RESP_LEN 1

#define MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED "\xe2\x81\x60"
#define MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_RESP_LEN 0
#define MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_MIN 0
#define MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_MAX 255
#define MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_DEFAULT 0x61

#define MOUSE_PS2_ST_TP_PTS_THRESHOLD "tp_pts_threshold"
#define MOUSE_PS2_CMD_TP_GET_PTS_THRESHOLD "\xe2\x80\x5c"
#define MOUSE_PS2_CMD_TP_GET_PTS_THRESHOLD_RESP_LEN 1

#define MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD "\xe2\x81\x5c"
#define MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_RESP_LEN 0
#define MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_MIN 0
#define MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_MAX 255
#define MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_DEFAULT 0x08

// Trackpoint Config Bits
#define MOUSE_PS2_TP_CONFIG_BIT_PRESS_TO_SELECT 0x00
#define MOUSE_PS2_TP_CONFIG_BIT_RESERVED 0x01
#define MOUSE_PS2_TP_CONFIG_BIT_BUTTON2 0x02
#define MOUSE_PS2_TP_CONFIG_BIT_INVERT_X 0x03
#define MOUSE_PS2_TP_CONFIG_BIT_INVERT_Y 0x04
#define MOUSE_PS2_TP_CONFIG_BIT_INVERT_Z 0x05
#define MOUSE_PS2_TP_CONFIG_BIT_SWAP_XY 0x06
#define MOUSE_PS2_TP_CONFIG_BIT_FORCE_TRANSPARENT 0x07

// Responses
#define MOUSE_PS2_RESP_SELF_TEST_PASS 0xaa
#define MOUSE_PS2_RESP_SELF_TEST_FAIL 0xfc

/*
 * ZMK Defines
 */

#define MOUSE_PS2_BUTTON_L_IDX 0
#define MOUSE_PS2_BUTTON_R_IDX 1
#define MOUSE_PS2_BUTTON_M_IDX 3

#define MOUSE_PS2_THREAD_STACK_SIZE 1024
#define MOUSE_PS2_THREAD_PRIORITY 10

/*
 * Global Variables
 */

#define MOUSE_PS2_SETTINGS_SUBTREE "mouse_ps2"

typedef enum {
    MOUSE_PS2_PACKET_MODE_PS2_DEFAULT,
    MOUSE_PS2_PACKET_MODE_SCROLL,
} zmk_mouse_ps2_packet_mode;

struct zmk_mouse_ps2_config {
    const struct device *ps2_device;
    struct gpio_dt_spec rst_gpio;

    bool scroll_mode;
    bool disable_clicking;
    int sampling_rate;

    bool tp_press_to_select;
    int tp_press_to_select_threshold;
    int tp_sensitivity;
    int tp_neg_inertia;
    int tp_val6_upper_speed;
    bool tp_x_invert;
    bool tp_y_invert;
    bool tp_xy_swap;
};

struct zmk_mouse_ps2_packet {
    int16_t mov_x;
    int16_t mov_y;
    int8_t scroll;
    bool overflow_x;
    bool overflow_y;
    bool button_l;
    bool button_m;
    bool button_r;
};

struct zmk_mouse_ps2_data {
    const struct device *dev;
    struct gpio_dt_spec rst_gpio; /* GPIO used for Power-On-Reset line */

    K_THREAD_STACK_MEMBER(thread_stack, MOUSE_PS2_THREAD_STACK_SIZE);
    struct k_thread thread;

    zmk_mouse_ps2_packet_mode packet_mode;
    uint8_t packet_buffer[4];
    int packet_idx;
    struct zmk_mouse_ps2_packet prev_packet;
    struct k_work_delayable packet_buffer_timeout;

    bool button_l_is_held;
    bool button_m_is_held;
    bool button_r_is_held;

    bool activity_reporting_on;
    bool is_trackpoint;

    uint8_t sampling_rate;
    uint8_t tp_sensitivity;
    uint8_t tp_neg_inertia;
    uint8_t tp_value6;
    uint8_t tp_pts_threshold;
};

static const struct zmk_mouse_ps2_config zmk_mouse_ps2_config = {
    .ps2_device = DEVICE_DT_GET(DT_INST_PHANDLE(0, ps2_device)),

#if DT_INST_NODE_HAS_PROP(0, rst_gpios)
    .rst_gpio = GPIO_DT_SPEC_INST_GET(0, rst_gpios),
#else
    .rst_gpio =
        {
            .port = NULL,
            .pin = 0,
            .dt_flags = 0,
        },
#endif

    .scroll_mode = DT_INST_PROP_OR(0, scroll_mode, false),
    .disable_clicking = DT_INST_PROP_OR(0, disable_clicking, false),
    .sampling_rate = DT_INST_PROP_OR(0, sampling_rate, MOUSE_PS2_CMD_SET_SAMPLING_RATE_DEFAULT),
    .tp_press_to_select = DT_INST_PROP_OR(0, tp_press_to_select, false),
    .tp_press_to_select_threshold = DT_INST_PROP_OR(0, tp_press_to_select_threshold, -1),
    .tp_sensitivity = DT_INST_PROP_OR(0, tp_sensitivity, -1),
    .tp_neg_inertia = DT_INST_PROP_OR(0, tp_neg_inertia, -1),
    .tp_val6_upper_speed = DT_INST_PROP_OR(0, tp_val6_upper_speed, -1),
    .tp_x_invert = DT_INST_PROP_OR(0, tp_x_invert, false),
    .tp_y_invert = DT_INST_PROP_OR(0, tp_y_invert, false),
    .tp_xy_swap = DT_INST_PROP_OR(0, tp_xy_swap, false),
};

static struct zmk_mouse_ps2_data zmk_mouse_ps2_data = {
    .packet_mode = MOUSE_PS2_PACKET_MODE_PS2_DEFAULT,
    .packet_idx = 0,
    .prev_packet =
        {
            .button_l = false,
            .button_r = false,
            .button_m = false,
            .overflow_x = 0,
            .overflow_y = 0,
            .mov_x = 0,
            .mov_y = 0,
            .scroll = 0,
        },

    .button_l_is_held = false,
    .button_m_is_held = false,
    .button_r_is_held = false,

    // Data reporting is disabled on init
    .activity_reporting_on = false,
    .is_trackpoint = false,

    // PS2 devices initialize with this rate
    .sampling_rate = MOUSE_PS2_CMD_SET_SAMPLING_RATE_DEFAULT,
    .tp_sensitivity = MOUSE_PS2_CMD_TP_SET_SENSITIVITY_DEFAULT,
    .tp_neg_inertia = MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_DEFAULT,
    .tp_value6 = MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_DEFAULT,
    .tp_pts_threshold = MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_DEFAULT,
};

static int allowed_sampling_rates[] = {
    10, 20, 40, 60, 80, 100, 200,
};

/*
 * Function Definitions
 */

int zmk_mouse_ps2_settings_save();

/*
 * Helpers
 */

#define MOUSE_PS2_GET_BIT(data, bit_pos) ((data >> bit_pos) & 0x1)
#define MOUSE_PS2_SET_BIT(data, bit_val, bit_pos) (data |= (bit_val) << bit_pos)

/*
 * Mouse Activity Packet Reading
 */

void zmk_mouse_ps2_activity_process_cmd(zmk_mouse_ps2_packet_mode packet_mode, uint8_t packet_state,
                                        uint8_t packet_x, uint8_t packet_y, uint8_t packet_extra);
void zmk_mouse_ps2_activity_abort_cmd();
void zmk_mouse_ps2_activity_move_mouse(int16_t mov_x, int16_t mov_y);
void zmk_mouse_ps2_activity_scroll(int8_t scroll_y);
void zmk_mouse_ps2_activity_click_buttons(bool button_l, bool button_m, bool button_r);
void zmk_mouse_ps2_activity_reset_packet_buffer();
struct zmk_mouse_ps2_packet
zmk_mouse_ps2_activity_parse_packet_buffer(zmk_mouse_ps2_packet_mode packet_mode,
                                           uint8_t packet_state, uint8_t packet_x, uint8_t packet_y,
                                           uint8_t packet_extra);
void zmk_mouse_ps2_activity_toggle_layer();

// Called by the PS/2 driver whenver the mouse sends a byte and
// reporting is enabled through `zmk_mouse_ps2_activity_reporting_enable`.
void zmk_mouse_ps2_activity_callback(const struct device *ps2_device, uint8_t byte) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    k_work_cancel_delayable(&data->packet_buffer_timeout);

    // LOG_DBG("Received mouse movement data: 0x%x", byte);

    data->packet_buffer[data->packet_idx] = byte;

    if (data->packet_idx == 0) {

        // Bit 3 of the first command byte should always be 1
        // If it is not, then we are definitely out of alignment.
        // So we ask the device to resend the entire 3-byte command
        // again.
        int alignment_bit = MOUSE_PS2_GET_BIT(byte, 3);
        if (alignment_bit != 1) {

            zmk_mouse_ps2_activity_abort_cmd("Bit 3 of packet is 0 instead of 1");
            return;
        }
    } else if (data->packet_idx == 1) {
        // Do nothing
    } else if ((data->packet_mode == MOUSE_PS2_PACKET_MODE_PS2_DEFAULT && data->packet_idx == 2) ||
               (data->packet_mode == MOUSE_PS2_PACKET_MODE_SCROLL && data->packet_idx == 3)) {

        zmk_mouse_ps2_activity_process_cmd(data->packet_mode, data->packet_buffer[0],
                                           data->packet_buffer[1], data->packet_buffer[2],
                                           data->packet_buffer[3]);
        zmk_mouse_ps2_activity_reset_packet_buffer();
        return;
    }

    data->packet_idx += 1;

    k_work_schedule(&data->packet_buffer_timeout, MOUSE_PS2_TIMEOUT_ACTIVITY_PACKET);
}

void zmk_mouse_ps2_activity_abort_cmd(char *reason) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;
    const struct zmk_mouse_ps2_config *config = &zmk_mouse_ps2_config;
    const struct device *ps2_device = config->ps2_device;

    LOG_ERR("PS/2 Mouse cmd buffer is out of aligment. Requesting resend: %s", reason);

    data->packet_idx = 0;
    ps2_write(ps2_device, MOUSE_PS2_CMD_RESEND[0]);

    zmk_mouse_ps2_activity_reset_packet_buffer();
}

// Called if the PS/2 driver encounters a transmission error and asks the
// device to resend the packet.
// The device will resend all bytes of the packet. So we need to reset our
// buffer.
void zmk_mouse_ps2_activity_resend_callback(const struct device *ps2_device) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    LOG_WRN("Mouse movement cmd had transmission error on idx=%d", data->packet_idx);

    zmk_mouse_ps2_activity_reset_packet_buffer();
}

// Called if no new byte arrives within
// MOUSE_PS2_TIMEOUT_ACTIVITY_PACKET
void zmk_mouse_ps2_activity_packet_timout(struct k_work *item) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    LOG_DBG("Mouse movement cmd timed out on idx=%d", data->packet_idx);

    // Reset the cmd buffer in case we are out of alignment.
    // This way if the mouse ever gets out of alignment, the user
    // can reset it by just not moving it for a second.
    zmk_mouse_ps2_activity_reset_packet_buffer();
}

void zmk_mouse_ps2_activity_reset_packet_buffer() {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    data->packet_idx = 0;
    memset(data->packet_buffer, 0x0, sizeof(data->packet_buffer));
}

void zmk_mouse_ps2_activity_process_cmd(zmk_mouse_ps2_packet_mode packet_mode, uint8_t packet_state,
                                        uint8_t packet_x, uint8_t packet_y, uint8_t packet_extra) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;
    struct zmk_mouse_ps2_packet packet;
    packet = zmk_mouse_ps2_activity_parse_packet_buffer(packet_mode, packet_state, packet_x,
                                                        packet_y, packet_extra);

    int x_delta = abs(data->prev_packet.mov_x - packet.mov_x);
    int y_delta = abs(data->prev_packet.mov_y - packet.mov_y);

    LOG_DBG("Got mouse activity cmd "
            "(mov_x=%d, mov_y=%d, o_x=%d, o_y=%d, scroll=%d, "
            "b_l=%d, b_m=%d, b_r=%d) and ("
            "x_delta=%d, y_delta=%d)",
            packet.mov_x, packet.mov_y, packet.overflow_x, packet.overflow_y, packet.scroll,
            packet.button_l, packet.button_m, packet.button_r, x_delta, y_delta);

#if IS_ENABLED(CONFIG_ZMK_INPUT_MOUSE_PS2_ENABLE_ERROR_MITIGATION)
    if (packet.overflow_x == 1 && packet.overflow_y == 1) {
        LOG_WRN("Detected overflow in both x and y. "
                "Probably mistransmission. Aborting...");

        zmk_mouse_ps2_activity_abort_cmd("Overflow in both x and y");
        return;
    }

    // If the mouse exceeds the allowed threshold of movement, it's probably
    // a mistransmission or misalignment.
    // But we only do this check if there was prior movement that wasn't
    // reset in `zmk_mouse_ps2_activity_packet_timout`.
    if ((packet.mov_x != 0 && packet.mov_y != 0) && (x_delta > 150 || y_delta > 150)) {
        LOG_WRN("Detected malformed packet with "
                "(mov_x=%d, mov_y=%d, o_x=%d, o_y=%d, scroll=%d, "
                "b_l=%d, b_m=%d, b_r=%d) and ("
                "x_delta=%d, y_delta=%d)",
                packet.mov_x, packet.mov_y, packet.overflow_x, packet.overflow_y, packet.scroll,
                packet.button_l, packet.button_m, packet.button_r, x_delta, y_delta);
        zmk_mouse_ps2_activity_abort_cmd("Exceeds movement threshold.");
        return;
    }
#endif

    zmk_mouse_ps2_activity_move_mouse(packet.mov_x, packet.mov_y);
    zmk_mouse_ps2_activity_click_buttons(packet.button_l, packet.button_m, packet.button_r);

    data->prev_packet = packet;
}

struct zmk_mouse_ps2_packet
zmk_mouse_ps2_activity_parse_packet_buffer(zmk_mouse_ps2_packet_mode packet_mode,
                                           uint8_t packet_state, uint8_t packet_x, uint8_t packet_y,
                                           uint8_t packet_extra) {
    struct zmk_mouse_ps2_packet packet;

    packet.button_l = MOUSE_PS2_GET_BIT(packet_state, 0);
    packet.button_r = MOUSE_PS2_GET_BIT(packet_state, 1);
    packet.button_m = MOUSE_PS2_GET_BIT(packet_state, 2);
    packet.overflow_x = MOUSE_PS2_GET_BIT(packet_state, 6);
    packet.overflow_y = MOUSE_PS2_GET_BIT(packet_state, 7);
    packet.scroll = 0;

    // The coordinates are delivered as a signed 9bit integers.
    // But a PS/2 packet is only 8 bits, so the most significant
    // bit with the sign is stored inside the state packet.
    //
    // Since we are converting the uint8_t into a int16_t
    // we must pad the unused most significant bits with
    // the sign bit.
    //
    // Example:
    //                              ↓ x sign bit
    //  - State: 0x18 (          0001 1000)
    //                             ↑ y sign bit
    //  - X:     0xfd (          1111 1101) / decimal 253
    //  - New X:      (1111 1111 1111 1101) / decimal -3
    //
    //  - Y:     0x02 (          0000 0010) / decimal 2
    //  - New Y:      (0000 0000 0000 0010) / decimal 2
    //
    // The code below creates a signed int and is from...
    // https://wiki.osdev.org/PS/2_Mouse
    packet.mov_x = packet_x - ((packet_state << 4) & 0x100);
    packet.mov_y = packet_y - ((packet_state << 3) & 0x100);

    // If packet mode scroll or scroll+5 buttons is used,
    // then the first 4 bit of the extra byte are used for the
    // scroll wheel. It is a signed number with the rango of
    // -8 to +7.
    if (packet_mode == MOUSE_PS2_PACKET_MODE_SCROLL) {
        MOUSE_PS2_SET_BIT(packet.scroll, MOUSE_PS2_GET_BIT(packet_extra, 0), 0);
        MOUSE_PS2_SET_BIT(packet.scroll, MOUSE_PS2_GET_BIT(packet_extra, 1), 1);
        MOUSE_PS2_SET_BIT(packet.scroll, MOUSE_PS2_GET_BIT(packet_extra, 2), 2);
        packet.scroll = packet_extra - ((packet.scroll << 3) & 0x100);
    }

    return packet;
}

/*
 * Mouse Moving and Clicking
 */

static bool zmk_mouse_ps2_is_non_zero_1d_movement(int16_t speed) { return speed != 0; }

void zmk_mouse_ps2_activity_move_mouse(int16_t mov_x, int16_t mov_y) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;
    int ret = 0;

    bool have_x = zmk_mouse_ps2_is_non_zero_1d_movement(mov_x);
    bool have_y = zmk_mouse_ps2_is_non_zero_1d_movement(mov_y);

    if (have_x) {
        ret = input_report_rel(data->dev, INPUT_REL_X, mov_x, !have_y, K_NO_WAIT);
    }
    if (have_y) {
        ret = input_report_rel(data->dev, INPUT_REL_Y, mov_y, true, K_NO_WAIT);
    }
}

void zmk_mouse_ps2_activity_click_buttons(bool button_l, bool button_m, bool button_r) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;
    const struct zmk_mouse_ps2_config *config = &zmk_mouse_ps2_config;

    // TODO: Integrate this with the proper button mask instead
    // of hardcoding the mouse button indeces.
    // Check hid.c and zmk_hid_mouse_buttons_press() for more info.

    int buttons_pressed = 0;
    int buttons_released = 0;

    // First we check which mouse button press states have changed
    bool button_l_pressed = false;
    bool button_l_released = false;
    if (button_l == true && data->button_l_is_held == false) {
        LOG_INF("Pressed button_l");

        button_l_pressed = true;
        buttons_pressed++;
    } else if (button_l == false && data->button_l_is_held == true) {
        LOG_INF("Releasing button_l");

        button_l_released = true;
        buttons_released++;
    }

    bool button_m_released = false;
    bool button_m_pressed = false;
    if (button_m == true && data->button_m_is_held == false) {
        LOG_INF("Pressing button_m");

        button_m_pressed = true;
        buttons_pressed++;
    } else if (button_m == false && data->button_m_is_held == true) {
        LOG_INF("Releasing button_m");

        button_m_released = true;
        buttons_released++;
    }

    bool button_r_released = false;
    bool button_r_pressed = false;
    if (button_r == true && data->button_r_is_held == false) {
        LOG_INF("Pressing button_r");

        button_r_pressed = true;
        buttons_pressed++;
    } else if (button_r == false && data->button_r_is_held == true) {
        LOG_INF("Releasing button_r");

        button_r_released = true;
        buttons_released++;
    }

    // Then we check if this is likely a transmission error
    if (buttons_pressed > 1 || buttons_released > 1) {
        LOG_WRN("Ignoring button presses: Received %d button presses "
                "and %d button releases in one packet. "
                "Probably tranmission error.",
                buttons_pressed, buttons_released);

        zmk_mouse_ps2_activity_abort_cmd("Multiple button presses");
        return;
    }

    if (config->disable_clicking != true) {
        // If it wasn't, we actually send the events.
        if (buttons_pressed > 0 || buttons_released > 0) {

            int buttons_need_reporting = buttons_pressed + buttons_released;

            // Left button
            if (button_l_pressed) {

                input_report_key(data->dev, INPUT_BTN_0, 1,
                                 buttons_need_reporting == 1 ? true : false, K_FOREVER);
                data->button_l_is_held = true;
            } else if (button_l_released) {

                input_report_key(data->dev, INPUT_BTN_0, 0,
                                 buttons_need_reporting == 1 ? true : false, K_FOREVER);
                data->button_l_is_held = false;
            }

            buttons_need_reporting--;

            // Right button
            if (button_r_pressed) {

                input_report_key(data->dev, INPUT_BTN_1, 1,
                                 buttons_need_reporting == 1 ? true : false, K_FOREVER);
                data->button_r_is_held = true;
            } else if (button_r_released) {

                input_report_key(data->dev, INPUT_BTN_1, 0,
                                 buttons_need_reporting == 1 ? true : false, K_FOREVER);
                data->button_r_is_held = false;
            }

            buttons_need_reporting--;

            // Middle Button
            if (button_m_pressed) {

                input_report_key(data->dev, INPUT_BTN_2, 1,
                                 buttons_need_reporting == 1 ? true : false, K_FOREVER);
                data->button_m_is_held = true;
            } else if (button_m_released) {

                input_report_key(data->dev, INPUT_BTN_2, 0,
                                 buttons_need_reporting == 1 ? true : false, K_FOREVER);
                data->button_m_is_held = false;
            }
        }
    }
}

/*
 * PS/2 Command Sending Wrapper
 */
int zmk_mouse_ps2_activity_reporting_enable();
int zmk_mouse_ps2_activity_reporting_disable();

struct zmk_mouse_ps2_send_cmd_resp {
    int err;
    char err_msg[80];
    uint8_t resp_buffer[8];
    int resp_len;
};

struct zmk_mouse_ps2_send_cmd_resp zmk_mouse_ps2_send_cmd(char *cmd, int cmd_len, uint8_t *arg,
                                                          int resp_len, bool pause_reporting) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;
    const struct zmk_mouse_ps2_config *config = &zmk_mouse_ps2_config;
    const struct device *ps2_device = config->ps2_device;
    int err = 0;
    bool prev_activity_reporting_on = data->activity_reporting_on;

    struct zmk_mouse_ps2_send_cmd_resp resp = {
        .err = 0,
        .err_msg = "",
        .resp_len = 0,
    };
    memset(resp.resp_buffer, 0x0, sizeof(resp.resp_buffer));

    // Don't send the string termination NULL byte
    int cmd_bytes = cmd_len - 1;
    if (cmd_bytes < 1) {
        err = 1;
        snprintf(resp.err_msg, sizeof(resp.err_msg),
                 "Cannot send cmd with less than 1 byte length");

        return resp;
    }

    if (resp_len > sizeof(resp.resp_buffer)) {
        err = 2;
        snprintf(resp.err_msg, sizeof(resp.err_msg),
                 "Response can't be longer than the resp_buffer (%d)", sizeof(resp.err_msg));

        return resp;
    }

    if (pause_reporting == true && data->activity_reporting_on == true) {
        LOG_DBG("Disabling mouse activity reporting...");

        err = zmk_mouse_ps2_activity_reporting_disable();
        if (err) {
            resp.err = err;
            snprintf(resp.err_msg, sizeof(resp.err_msg), "Could not disable data reporting (%d)",
                     err);
        }
    }

    if (resp.err == 0) {
        LOG_DBG("Sending cmd...");

        for (int i = 0; i < cmd_bytes; i++) {
            err = ps2_write(ps2_device, cmd[i]);
            if (err) {
                resp.err = err;
                snprintf(resp.err_msg, sizeof(resp.err_msg), "Could not send cmd byte %d/%d (%d)",
                         i + 1, cmd_bytes, err);
                break;
            }
        }
    }

    if (resp.err == 0 && arg != NULL) {
        LOG_DBG("Sending arg...");
        err = ps2_write(ps2_device, *arg);
        if (err) {
            resp.err = err;
            snprintf(resp.err_msg, sizeof(resp.err_msg), "Could not send arg (%d)", err);
        }
    }

    if (resp.err == 0 && resp_len > 0) {
        LOG_DBG("Reading response...");
        for (int i = 0; i < resp_len; i++) {
            err = ps2_read(ps2_device, &resp.resp_buffer[i]);
            if (err) {
                resp.err = err;
                snprintf(resp.err_msg, sizeof(resp.err_msg),
                         "Could not read response cmd byte %d/%d (%d)", i + 1, resp_len, err);
                break;
            }
        }
    }

    if (pause_reporting == true && prev_activity_reporting_on == true) {
        LOG_DBG("Enabling mouse activity reporting...");

        err = zmk_mouse_ps2_activity_reporting_enable();
        if (err) {
            // Don' overwrite existing error
            if (resp.err == 0) {
                resp.err = err;
                snprintf(resp.err_msg, sizeof(resp.err_msg),
                         "Could not re-enable data reporting (%d)", err);
            }
        }
    }

    return resp;
}

int zmk_mouse_ps2_activity_reporting_enable() {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;
    const struct zmk_mouse_ps2_config *config = &zmk_mouse_ps2_config;
    const struct device *ps2_device = config->ps2_device;

    if (data->activity_reporting_on == true) {
        return 0;
    }

    uint8_t cmd = MOUSE_PS2_CMD_ENABLE_REPORTING[0];
    int err = ps2_write(ps2_device, cmd);
    if (err) {
        LOG_ERR("Could not enable data reporting: %d", err);
        return err;
    }

    err = ps2_enable_callback(ps2_device);
    if (err) {
        LOG_ERR("Could not enable ps2 callback: %d", err);
        return err;
    }

    data->activity_reporting_on = true;

    return 0;
}

int zmk_mouse_ps2_activity_reporting_disable() {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;
    const struct zmk_mouse_ps2_config *config = &zmk_mouse_ps2_config;
    const struct device *ps2_device = config->ps2_device;

    if (data->activity_reporting_on == false) {
        return 0;
    }

    uint8_t cmd = MOUSE_PS2_CMD_DISABLE_REPORTING[0];
    int err = ps2_write(ps2_device, cmd);
    if (err) {
        LOG_ERR("Could not disable data reporting: %d", err);
        return err;
    }

    err = ps2_disable_callback(ps2_device);
    if (err) {
        LOG_ERR("Could not disable ps2 callback: %d", err);
        return err;
    }

    data->activity_reporting_on = false;

    return 0;
}

/*
 * PS/2 Command Helpers
 */

int zmk_mouse_ps2_array_get_elem_index(int elem, int *array, size_t array_size) {
    int elem_index = -1;
    for (int i = 0; i < array_size; i++) {
        if (array[i] == elem) {
            elem_index = i;
            break;
        }
    }

    return elem_index;
}

int zmk_mouse_ps2_array_get_next_elem(int elem, int *array, size_t array_size) {
    int elem_index = zmk_mouse_ps2_array_get_elem_index(elem, array, array_size);
    if (elem_index == -1) {
        return -1;
    }

    int next_index = elem_index + 1;
    if (next_index >= array_size) {
        return -1;
    }

    return array[next_index];
}

int zmk_mouse_ps2_array_get_prev_elem(int elem, int *array, size_t array_size) {
    int elem_index = zmk_mouse_ps2_array_get_elem_index(elem, array, array_size);
    if (elem_index == -1) {
        return -1;
    }

    int prev_index = elem_index - 1;
    if (prev_index < 0 || prev_index >= array_size) {
        return -1;
    }

    return array[prev_index];
}

/*
 * PS/2 Commands
 */

int zmk_mouse_ps2_reset(const struct device *ps2_device) {
    struct zmk_mouse_ps2_send_cmd_resp resp =
        zmk_mouse_ps2_send_cmd(MOUSE_PS2_CMD_RESET, sizeof(MOUSE_PS2_CMD_RESET), NULL,
                               MOUSE_PS2_CMD_RESET_RESP_LEN, false);
    if (resp.err) {
        LOG_ERR("Could not send reset cmd");
    }

    return resp.err;
}

int zmk_mouse_ps2_get_secondary_id(uint8_t *resp_byte_1, uint8_t *resp_byte_2) {
    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_GET_SECONDARY_ID, sizeof(MOUSE_PS2_CMD_GET_SECONDARY_ID), NULL,
        MOUSE_PS2_CMD_GET_SECONDARY_ID_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not get secondary id");
        return resp.err;
    }

    *resp_byte_1 = resp.resp_buffer[0];
    *resp_byte_2 = resp.resp_buffer[1];

    return 0;
}

int zmk_mouse_ps2_set_sampling_rate(uint8_t sampling_rate) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    int rate_idx = zmk_mouse_ps2_array_get_elem_index(sampling_rate, allowed_sampling_rates,
                                                      sizeof(allowed_sampling_rates));
    if (rate_idx == -1) {
        LOG_ERR("Requested to set illegal sampling rate: %d", sampling_rate);
        return -1;
    }

    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_SET_SAMPLING_RATE, sizeof(MOUSE_PS2_CMD_SET_SAMPLING_RATE), &sampling_rate,
        MOUSE_PS2_CMD_SET_SAMPLING_RATE_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not set sample rate to %d", sampling_rate);
        return resp.err;
    }

    data->sampling_rate = sampling_rate;

    LOG_INF("Successfully set sampling rate to %d", sampling_rate);

    return resp.err;
}

int zmk_mouse_ps2_get_device_id(uint8_t *device_id) {
    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_GET_DEVICE_ID, sizeof(MOUSE_PS2_CMD_GET_DEVICE_ID), NULL, 1, true);
    if (resp.err) {
        LOG_ERR("Could not get device id");
        return resp.err;
    }

    *device_id = resp.resp_buffer[0];

    return 0;
}

int zmk_mouse_ps2_set_packet_mode(zmk_mouse_ps2_packet_mode mode) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    if (mode == MOUSE_PS2_PACKET_MODE_PS2_DEFAULT) {
        // Do nothing. Mouse devices enable this by
        // default.
        return 0;
    }

    bool prev_activity_reporting_on = data->activity_reporting_on;
    zmk_mouse_ps2_activity_reporting_disable();

    // Setting a mouse mode is a bit like using a cheat code
    // in a video game.
    // You have to send a specific sequence of sampling rates.
    if (mode == MOUSE_PS2_PACKET_MODE_SCROLL) {

        zmk_mouse_ps2_set_sampling_rate(200);
        zmk_mouse_ps2_set_sampling_rate(100);
        zmk_mouse_ps2_set_sampling_rate(80);
    }

    // Scroll mouse + 5 buttons mode can be enabled with the
    // following sequence, but since I don't have a mouse to
    // test it, I am commenting it out for now.
    // else if(mode == MOUSE_PS2_PACKET_MODE_SCROLL_5_BUTTONS) {

    //     zmk_mouse_ps2_set_sampling_rate(200);
    //     zmk_mouse_ps2_set_sampling_rate(200);
    //     zmk_mouse_ps2_set_sampling_rate(80);
    // }

    uint8_t device_id;
    int err = zmk_mouse_ps2_get_device_id(&device_id);
    if (err) {
        LOG_ERR("Could not enable packet mode %d. Failed to get device id with "
                "error %d",
                mode, err);
    } else {
        if (device_id == 0x00) {
            LOG_ERR("Could not enable packet mode %d. The device does not "
                    "support it",
                    mode);

            data->packet_mode = MOUSE_PS2_PACKET_MODE_PS2_DEFAULT;
            err = 1;
        } else if (device_id == 0x03 || device_id == 0x04) {
            LOG_INF("Successfully activated packet mode %d. Mouse returned "
                    "device id: %d",
                    mode, device_id);

            data->packet_mode = MOUSE_PS2_PACKET_MODE_SCROLL;
            err = 0;
        }
        // else if(device_id == 0x04) {
        //     LOG_INF(
        //         "Successfully activated packet mode %d. Mouse returned device "
        //         "id: %d", mode, device_id
        //     );

        //     data->packet_mode = MOUSE_PS2_PACKET_MODE_SCROLL_5_BUTTONS;
        //     err = 0;
        // }
        else {
            LOG_ERR("Could not enable packet mode %d. Received an invalid "
                    "device id: %d",
                    mode, device_id);

            data->packet_mode = MOUSE_PS2_PACKET_MODE_PS2_DEFAULT;
            err = 1;
        }
    }

    // Restore sampling rate to prev value
    zmk_mouse_ps2_set_sampling_rate(data->sampling_rate);

    if (prev_activity_reporting_on == true) {
        zmk_mouse_ps2_activity_reporting_enable();
    }

    return err;
}

/*
 * Trackpoint Commands
 */

bool zmk_mouse_ps2_is_device_trackpoint() {
    bool ret = false;

    uint8_t second_id_1, second_id_2;
    int err = zmk_mouse_ps2_get_secondary_id(&second_id_1, &second_id_2);
    if (err) {
        // Not all devices implement this command.
        ret = false;
    } else {
        if (second_id_1 == 0x1) {
            ret = true;
        }
    }

    LOG_DBG("Connected device is a trackpoint: %d", ret);

    return ret;
}

int zmk_mouse_ps2_tp_get_config_byte(uint8_t *config_byte) {
    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_TP_GET_CONFIG_BYTE, sizeof(MOUSE_PS2_CMD_TP_GET_CONFIG_BYTE), NULL,
        MOUSE_PS2_CMD_TP_GET_CONFIG_BYTE_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not read trackpoint config byte");
        return resp.err;
    }

    *config_byte = resp.resp_buffer[0];

    return 0;
}

int zmk_mouse_ps2_tp_set_config_option(int config_bit, bool enabled, char *descr) {
    uint8_t config_byte;
    int err = zmk_mouse_ps2_tp_get_config_byte(&config_byte);
    if (err) {
        return err;
    }

    bool is_enabled = MOUSE_PS2_GET_BIT(config_byte, config_bit);

    if (is_enabled == enabled) {
        LOG_DBG("Trackpoint %s was already %s... not doing anything.", descr,
                is_enabled ? "enabled" : "disabled");
        return 0;
    }

    LOG_DBG("Setting trackpoint %s: %s", descr, enabled ? "enabled" : "disabled");

    MOUSE_PS2_SET_BIT(config_byte, enabled, config_bit);

    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_TP_SET_CONFIG_BYTE, sizeof(MOUSE_PS2_CMD_TP_SET_CONFIG_BYTE), &config_byte,
        MOUSE_PS2_CMD_TP_SET_CONFIG_BYTE_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not set trackpoint %s to %s", descr, enabled ? "enabled" : "disabled");
        return resp.err;
    }

    LOG_INF("Successfully set config option %s to %s", descr, enabled ? "enabled" : "disabled");

    return 0;
}

int zmk_mouse_ps2_tp_press_to_select_set(bool enabled) {
    int err = zmk_mouse_ps2_tp_set_config_option(MOUSE_PS2_TP_CONFIG_BIT_PRESS_TO_SELECT, enabled,
                                                 "Press To Select");

    return err;
}

int zmk_mouse_ps2_tp_invert_x_set(bool enabled) {
    int err =
        zmk_mouse_ps2_tp_set_config_option(MOUSE_PS2_TP_CONFIG_BIT_INVERT_X, enabled, "Invert X");

    return err;
}

int zmk_mouse_ps2_tp_invert_y_set(bool enabled) {
    int err =
        zmk_mouse_ps2_tp_set_config_option(MOUSE_PS2_TP_CONFIG_BIT_INVERT_Y, enabled, "Invert Y");

    return err;
}

int zmk_mouse_ps2_tp_swap_xy_set(bool enabled) {
    int err =
        zmk_mouse_ps2_tp_set_config_option(MOUSE_PS2_TP_CONFIG_BIT_SWAP_XY, enabled, "Swap XY");

    return err;
}

int zmk_mouse_ps2_tp_sensitivity_get(uint8_t *sensitivity) {
    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_TP_GET_SENSITIVITY, sizeof(MOUSE_PS2_CMD_TP_GET_SENSITIVITY), NULL,
        MOUSE_PS2_CMD_TP_GET_SENSITIVITY_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not get trackpoint sensitivity");
        return resp.err;
    }

    // Convert uint8_t to float
    // 0x80 (128) represents 1.0
    uint8_t sensitivity_int = resp.resp_buffer[0];
    *sensitivity = sensitivity_int;

    LOG_DBG("Trackpoint sensitivity is %d", sensitivity_int);

    return 0;
}

int zmk_mouse_ps2_tp_sensitivity_set(int sensitivity) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    if (sensitivity < MOUSE_PS2_CMD_TP_SET_SENSITIVITY_MIN ||
        sensitivity > MOUSE_PS2_CMD_TP_SET_SENSITIVITY_MAX) {
        LOG_ERR("Invalid sensitivity value %d. Min: %d; Max: %d", sensitivity,
                MOUSE_PS2_CMD_TP_SET_SENSITIVITY_MIN, MOUSE_PS2_CMD_TP_SET_SENSITIVITY_MAX);
        return 1;
    }

    uint8_t arg = sensitivity;

    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_TP_SET_SENSITIVITY, sizeof(MOUSE_PS2_CMD_TP_SET_SENSITIVITY), &arg,
        MOUSE_PS2_CMD_TP_SET_SENSITIVITY_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not set sensitivity to %d", sensitivity);
        return resp.err;
    }

    data->tp_sensitivity = sensitivity;

    LOG_INF("Successfully set TP sensitivity to %d", sensitivity);

    return 0;
}

int zmk_mouse_ps2_tp_sensitivity_change(int amount) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    int new_val = data->tp_sensitivity + amount;

    LOG_INF("Setting trackpoint sensitivity to %d", new_val);
    int err = zmk_mouse_ps2_tp_sensitivity_set(new_val);
    if (err == 0) {

        zmk_mouse_ps2_settings_save();
    }

    return err;
}

int zmk_mouse_ps2_tp_negative_inertia_get(uint8_t *neg_inertia) {
    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_TP_GET_NEG_INERTIA, sizeof(MOUSE_PS2_CMD_TP_GET_NEG_INERTIA), NULL,
        MOUSE_PS2_CMD_TP_GET_NEG_INERTIA_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not get trackpoint negative inertia");
        return resp.err;
    }

    uint8_t neg_inertia_int = resp.resp_buffer[0];
    *neg_inertia = neg_inertia_int;

    LOG_DBG("Trackpoint negative inertia is %d", neg_inertia_int);

    return 0;
}

int zmk_mouse_ps2_tp_neg_inertia_set(int neg_inertia) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    if (neg_inertia < MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_MIN ||
        neg_inertia > MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_MAX) {
        LOG_ERR("Invalid negative inertia value %d. Min: %d; Max: %d", neg_inertia,
                MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_MIN, MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_MAX);
        return 1;
    }

    uint8_t arg = neg_inertia;

    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_TP_SET_NEG_INERTIA, sizeof(MOUSE_PS2_CMD_TP_SET_NEG_INERTIA), &arg,
        MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not set negative inertia to %d", neg_inertia);
        return resp.err;
    }

    data->tp_neg_inertia = neg_inertia;

    LOG_INF("Successfully set TP negative inertia to %d", neg_inertia);

    return 0;
}

int zmk_mouse_ps2_tp_neg_inertia_change(int amount) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    int new_val = data->tp_neg_inertia + amount;

    LOG_INF("Setting negative inertia to %d", new_val);
    int err = zmk_mouse_ps2_tp_neg_inertia_set(new_val);
    if (err == 0) {

        zmk_mouse_ps2_settings_save();
    }

    return err;
}

int zmk_mouse_ps2_tp_value6_upper_plateau_speed_get(uint8_t *value6) {
    struct zmk_mouse_ps2_send_cmd_resp resp =
        zmk_mouse_ps2_send_cmd(MOUSE_PS2_CMD_TP_GET_VALUE6_UPPER_PLATEAU_SPEED,
                               sizeof(MOUSE_PS2_CMD_TP_GET_VALUE6_UPPER_PLATEAU_SPEED), NULL,
                               MOUSE_PS2_CMD_TP_GET_VALUE6_UPPER_PLATEAU_SPEED_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not get trackpoint value6 upper plateau speed");
        return resp.err;
    }

    uint8_t value6_int = resp.resp_buffer[0];
    *value6 = value6_int;

    LOG_DBG("Trackpoint value6 upper plateau speed is %d", value6_int);

    return 0;
}

int zmk_mouse_ps2_tp_value6_upper_plateau_speed_set(int value6) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    if (value6 < MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_MIN ||
        value6 > MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_MAX) {
        LOG_ERR("Invalid value6 upper plateau speed value %d. Min: %d; Max: %d", value6,
                MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_MIN,
                MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_MAX);
        return 1;
    }

    uint8_t arg = value6;

    struct zmk_mouse_ps2_send_cmd_resp resp =
        zmk_mouse_ps2_send_cmd(MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED,
                               sizeof(MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED), &arg,
                               MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not set value6 upper plateau speed to %d", value6);
        return resp.err;
    }

    data->tp_value6 = value6;

    LOG_INF("Successfully set TP value6 upper plateau speed to %d", value6);

    return 0;
}

int zmk_mouse_ps2_tp_value6_upper_plateau_speed_change(int amount) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    int new_val = data->tp_value6 + amount;

    LOG_INF("Setting value6 upper plateau speed to %d", new_val);
    int err = zmk_mouse_ps2_tp_value6_upper_plateau_speed_set(new_val);
    if (err == 0) {

        zmk_mouse_ps2_settings_save();
    }

    return err;
}

int zmk_mouse_ps2_tp_pts_threshold_get(uint8_t *pts_threshold) {
    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_TP_GET_PTS_THRESHOLD, sizeof(MOUSE_PS2_CMD_TP_GET_PTS_THRESHOLD), NULL,
        MOUSE_PS2_CMD_TP_GET_PTS_THRESHOLD_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not get trackpoint press-to-select threshold");
        return resp.err;
    }

    uint8_t pts_threshold_int = resp.resp_buffer[0];
    *pts_threshold = pts_threshold_int;

    LOG_DBG("Trackpoint press-to-select threshold is %d", pts_threshold_int);

    return 0;
}

int zmk_mouse_ps2_tp_pts_threshold_set(int pts_threshold) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    if (pts_threshold < MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_MIN ||
        pts_threshold > MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_MAX) {
        LOG_ERR("Invalid press-to-select threshold value %d. Min: %d; Max: %d", pts_threshold,
                MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_MIN, MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_MAX);
        return 1;
    }

    uint8_t arg = pts_threshold;

    struct zmk_mouse_ps2_send_cmd_resp resp = zmk_mouse_ps2_send_cmd(
        MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD, sizeof(MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD), &arg,
        MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_RESP_LEN, true);
    if (resp.err) {
        LOG_ERR("Could not set press-to-select threshold to %d", pts_threshold);
        return resp.err;
    }

    data->tp_pts_threshold = pts_threshold;

    LOG_INF("Successfully set TP press-to-select threshold to %d", pts_threshold);

    return 0;
}

int zmk_mouse_ps2_tp_pts_threshold_change(int amount) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    int new_val = data->tp_pts_threshold + amount;

    LOG_INF("Setting press-to-select threshold to %d", new_val);
    int err = zmk_mouse_ps2_tp_pts_threshold_set(new_val);
    if (err == 0) {

        zmk_mouse_ps2_settings_save();
    }

    return err;
}

/*
 * State Saving
 */

#if IS_ENABLED(CONFIG_SETTINGS)

struct k_work_delayable zmk_mouse_ps2_save_work;

int zmk_mouse_ps2_settings_save_setting(char *setting_name, const void *value, size_t val_len) {
    char setting_path[40];
    snprintf(setting_path, sizeof(setting_path), "%s/%s", MOUSE_PS2_SETTINGS_SUBTREE, setting_name);

    LOG_DBG("Saving setting to `%s`", setting_path);
    int err = settings_save_one(setting_path, value, val_len);
    if (err) {
        LOG_ERR("Could not save setting to `%s`: %d", setting_path, err);
    }

    return err;
}

int zmk_mouse_ps2_settings_reset_setting(char *setting_name) {
    char setting_path[40];
    snprintf(setting_path, sizeof(setting_path), "%s/%s", MOUSE_PS2_SETTINGS_SUBTREE, setting_name);

    LOG_DBG("Reseting setting `%s`", setting_path);
    int err = settings_delete(setting_path);
    if (err) {
        LOG_ERR("Could not reset setting `%s`", setting_path);
    }

    return err;
}

static void zmk_mouse_ps2_settings_save_work(struct k_work *work) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    LOG_DBG("");

    zmk_mouse_ps2_settings_save_setting(MOUSE_PS2_ST_TP_SENSITIVITY, &data->tp_sensitivity,
                                        sizeof(data->tp_sensitivity));
    zmk_mouse_ps2_settings_save_setting(MOUSE_PS2_ST_TP_NEG_INERTIA, &data->tp_neg_inertia,
                                        sizeof(data->tp_neg_inertia));
    zmk_mouse_ps2_settings_save_setting(MOUSE_PS2_ST_TP_VALUE6, &data->tp_value6,
                                        sizeof(data->tp_value6));
    zmk_mouse_ps2_settings_save_setting(MOUSE_PS2_ST_TP_PTS_THRESHOLD, &data->tp_pts_threshold,
                                        sizeof(data->tp_pts_threshold));
}
#endif

int zmk_mouse_ps2_settings_save() {
    LOG_DBG("");

#if IS_ENABLED(CONFIG_SETTINGS)
    int ret =
        k_work_reschedule(&zmk_mouse_ps2_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
    return MIN(ret, 0);
#else
    return 0;
#endif
}

int zmk_mouse_ps2_settings_reset() {

    LOG_INF("Deleting runtime settings...");
    zmk_mouse_ps2_settings_reset_setting(MOUSE_PS2_ST_TP_SENSITIVITY);
    zmk_mouse_ps2_settings_reset_setting(MOUSE_PS2_ST_TP_NEG_INERTIA);
    zmk_mouse_ps2_settings_reset_setting(MOUSE_PS2_ST_TP_VALUE6);
    zmk_mouse_ps2_settings_reset_setting(MOUSE_PS2_ST_TP_PTS_THRESHOLD);

    LOG_INF("Restoring default settings to TP..");
    zmk_mouse_ps2_tp_sensitivity_set(MOUSE_PS2_CMD_TP_SET_SENSITIVITY_DEFAULT);

    zmk_mouse_ps2_tp_neg_inertia_set(MOUSE_PS2_CMD_TP_SET_NEG_INERTIA_DEFAULT);

    zmk_mouse_ps2_tp_value6_upper_plateau_speed_set(
        MOUSE_PS2_CMD_TP_SET_VALUE6_UPPER_PLATEAU_SPEED_DEFAULT);

    zmk_mouse_ps2_tp_pts_threshold_set(MOUSE_PS2_CMD_TP_SET_PTS_THRESHOLD_DEFAULT);

    return 0;
}

int zmk_mouse_ps2_settings_log() {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;

    char settings_str[250];

    snprintf(settings_str, sizeof(settings_str), " \n\
&mouse_ps2_conf = { \n\
    tp-sensitivity = <%d>; \n\
    tp-neg-inertia = <%d>; \n\
    tp-val6-upper-speed = <%d>; \n\
    tp-tp-press-to-select-threshold = <%d>; \n\
}",
             data->tp_sensitivity, data->tp_neg_inertia, data->tp_value6, data->tp_pts_threshold);

    LOG_INF("Current settings... %s", settings_str);

    return 0;
}

// This function is called when settings are loaded from flash by
// `settings_load_subtree`.
// It's called once for each PS/2 mouse setting that has been stored.
static int zmk_mouse_ps2_settings_restore(const char *name, size_t len, settings_read_cb read_cb,
                                          void *cb_arg) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;
    const struct zmk_mouse_ps2_config *config = &zmk_mouse_ps2_config;

    uint8_t setting_val;

    if (len != sizeof(setting_val)) {
        LOG_ERR("Could not restore settings %s: Len mismatch", name);

        return -EINVAL;
    }

    int rc = read_cb(cb_arg, &setting_val, sizeof(setting_val));
    if (rc <= 0) {
        LOG_ERR("Could not restore setting %s: %d", name, rc);
        return -EINVAL;
    }

    if (data->is_trackpoint == false) {
        LOG_INF("Mouse device is not a trackpoint. Not restoring setting %s.", name);

        return 0;
    }

    LOG_INF("Restoring setting %s with value: %d", name, setting_val);

    if (strcmp(name, MOUSE_PS2_ST_TP_SENSITIVITY) == 0) {

        if (config->tp_sensitivity != -1) {
            LOG_WRN("Not restoring runtime settings for %s with value %d, because deviceconfig "
                    "defines the setting with value %d",
                    name, setting_val, config->tp_sensitivity);

            return 0;
        }

        return zmk_mouse_ps2_tp_sensitivity_set(setting_val);
    } else if (strcmp(name, MOUSE_PS2_ST_TP_NEG_INERTIA) == 0) {
        if (config->tp_neg_inertia != -1) {
            LOG_WRN("Not restoring runtime settings for %s with value %d, because deviceconfig "
                    "defines the setting with value %d",
                    name, setting_val, config->tp_neg_inertia);

            return 0;
        }

        return zmk_mouse_ps2_tp_neg_inertia_set(setting_val);
    } else if (strcmp(name, MOUSE_PS2_ST_TP_VALUE6) == 0) {
        if (config->tp_val6_upper_speed != -1) {
            LOG_WRN("Not restoring runtime settings for %s with value %d, because deviceconfig "
                    "defines the setting with value %d",
                    name, setting_val, config->tp_val6_upper_speed);

            return 0;
        }

        return zmk_mouse_ps2_tp_value6_upper_plateau_speed_set(setting_val);
    } else if (strcmp(name, MOUSE_PS2_ST_TP_PTS_THRESHOLD) == 0) {
        if (config->tp_press_to_select_threshold != -1) {
            LOG_WRN("Not restoring runtime settings for %s with value %d, because deviceconfig "
                    "defines the setting with value %d",
                    name, setting_val, config->tp_press_to_select_threshold);

            return 0;
        }

        return zmk_mouse_ps2_tp_pts_threshold_set(setting_val);
    }

    return -EINVAL;
}

struct settings_handler zmk_mouse_ps2_settings_conf = {
    .name = MOUSE_PS2_SETTINGS_SUBTREE,
    .h_set = zmk_mouse_ps2_settings_restore,
};

int zmk_mouse_ps2_settings_init() {
#if IS_ENABLED(CONFIG_SETTINGS)
    LOG_DBG("");

    settings_subsys_init();

    int err = settings_register(&zmk_mouse_ps2_settings_conf);
    if (err) {
        LOG_ERR("Failed to register the PS/2 mouse settings handler (err %d)", err);
        return err;
    }

    k_work_init_delayable(&zmk_mouse_ps2_save_work, zmk_mouse_ps2_settings_save_work);

    // This will load the settings and then call
    // `zmk_mouse_ps2_settings_restore`, which will set the settings
    settings_load_subtree(MOUSE_PS2_SETTINGS_SUBTREE);
#endif

    return 0;
}

/*
 * Init
 */

static void zmk_mouse_ps2_init_thread(int dev_ptr, int unused);
int zmk_mouse_ps2_init_power_on_reset();
int zmk_mouse_ps2_init_wait_for_mouse(const struct device *dev);

static int zmk_mouse_ps2_init(const struct device *dev) {
    LOG_DBG("Inside zmk_mouse_ps2_init");

    LOG_DBG("Creating mouse_ps2 init thread.");
    k_thread_create(&zmk_mouse_ps2_data.thread, zmk_mouse_ps2_data.thread_stack,
                    MOUSE_PS2_THREAD_STACK_SIZE, (k_thread_entry_t)zmk_mouse_ps2_init_thread,
                    (struct device *)dev, 0, NULL, K_PRIO_COOP(MOUSE_PS2_THREAD_PRIORITY), 0,
                    K_MSEC(ZMK_MOUSE_PS2_INIT_THREAD_DELAY_MS));

    return 0;
}

static void zmk_mouse_ps2_init_thread(int dev_ptr, int unused) {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;
    int err;

    data->dev = INT_TO_POINTER(dev_ptr);

    const struct zmk_mouse_ps2_config *config = data->dev->config;

    zmk_mouse_ps2_init_power_on_reset();

    LOG_INF("Waiting for mouse to connect...");
    err = zmk_mouse_ps2_init_wait_for_mouse(data->dev);
    if (err) {
        LOG_ERR("Could not init a mouse in %d attempts. Giving up. "
                "Power cycle the mouse and reset zmk to try again.",
                MOUSE_PS2_INIT_ATTEMPTS);
        return;
    }

    if (config->sampling_rate != MOUSE_PS2_CMD_SET_SAMPLING_RATE_DEFAULT) {

        LOG_INF("Setting sample rate to %d...", config->sampling_rate);
        zmk_mouse_ps2_set_sampling_rate(config->sampling_rate);
        if (err) {
            LOG_ERR("Could not set sampling rate to %d: %d", config->sampling_rate, err);
            return;
        }
    }

    if (zmk_mouse_ps2_is_device_trackpoint() == true) {
        LOG_INF("Device is a trackpoint");
        data->is_trackpoint = true;

        if (config->tp_press_to_select) {
            LOG_INF("Enabling TP press to select...");
            zmk_mouse_ps2_tp_press_to_select_set(true);
        }

        if (config->tp_press_to_select_threshold != -1) {
            LOG_INF("Setting TP press to select thereshold to %d...",
                    config->tp_press_to_select_threshold);
            zmk_mouse_ps2_tp_pts_threshold_set(config->tp_press_to_select_threshold);
        }

        if (config->tp_sensitivity != -1) {
            LOG_INF("Setting TP sensitivity to %d...", config->tp_sensitivity);
            zmk_mouse_ps2_tp_sensitivity_set(config->tp_sensitivity);
        }

        if (config->tp_neg_inertia != -1) {
            LOG_INF("Setting TP inertia to %d...", config->tp_neg_inertia);
            zmk_mouse_ps2_tp_neg_inertia_set(config->tp_neg_inertia);
        }

        if (config->tp_val6_upper_speed != -1) {
            LOG_INF("Setting TP value 6 upper speed plateau to %d...", config->tp_val6_upper_speed);
            zmk_mouse_ps2_tp_value6_upper_plateau_speed_set(config->tp_val6_upper_speed);
        }
        if (config->tp_x_invert) {
            LOG_INF("Inverting trackpoint x axis.");
            zmk_mouse_ps2_tp_invert_x_set(true);
        }

        if (config->tp_y_invert) {
            LOG_INF("Inverting trackpoint y axis.");
            zmk_mouse_ps2_tp_invert_y_set(true);
        }

        if (config->tp_xy_swap) {
            LOG_INF("Swapping trackpoint x and y axis.");
            zmk_mouse_ps2_tp_swap_xy_set(true);
        }
    }

    if (config->scroll_mode) {
        LOG_INF("Enabling scroll mode.");
        zmk_mouse_ps2_set_packet_mode(MOUSE_PS2_PACKET_MODE_SCROLL);
    }

    zmk_mouse_ps2_settings_init();

    // Configure read callback
    LOG_DBG("Configuring ps2 callback...");
    err = ps2_config(config->ps2_device, &zmk_mouse_ps2_activity_callback,
                     &zmk_mouse_ps2_activity_resend_callback);
    if (err) {
        LOG_ERR("Could not configure ps2 interface: %d", err);
        return;
    }

    LOG_INF("Enabling data reporting and ps2 callback...");
    err = zmk_mouse_ps2_activity_reporting_enable();
    if (err) {
        LOG_ERR("Could not activate ps2 callback: %d", err);
    } else {
        LOG_DBG("Successfully activated ps2 callback");
    }

    k_work_init_delayable(&data->packet_buffer_timeout, zmk_mouse_ps2_activity_packet_timout);

    return;
}

// Power-On-Reset for trackpoints (and possibly other devices).
// From the `IBM TrackPoint System Version 4.0 Engineering
// Specification`...
// "The TrackPoint logic shall execute a Power On Reset (POR) when power is
//  applied to the device. The POR shall be timed to occur 600 ms ± 20 % from
//  the time power is applied to the TrackPoint controller. Activity on the
//  clock and data lines is ignored prior to the completion of the diagnostic
//  sequence. (See RESET mode of operation.)"
int zmk_mouse_ps2_init_power_on_reset() {
    struct zmk_mouse_ps2_data *data = &zmk_mouse_ps2_data;
    const struct zmk_mouse_ps2_config *config = &zmk_mouse_ps2_config;

    // Check if the optional rst-gpios setting was set
    if (config->rst_gpio.port == NULL) {
        return 0;
    }

    LOG_INF("Performing Power-On-Reset...");

    if (data->rst_gpio.port == NULL) {
        data->rst_gpio = config->rst_gpio;

        // Overwrite any user-provided flags from the devicetree
        data->rst_gpio.dt_flags = 0;
    }

    //  Set reset pin low...
    int err = gpio_pin_configure_dt(&data->rst_gpio, (GPIO_OUTPUT_HIGH));
    if (err) {
        LOG_ERR("Failed Power-On-Reset: Failed to configure RST GPIO pin to "
                "output low (err %d)",
                err);
        return err;
    }

    // Wait 600ms
    k_sleep(MOUSE_PS2_POWER_ON_RESET_TIME);

    // Set pin high
    err = gpio_pin_set_dt(&data->rst_gpio, 0);
    if (err) {
        LOG_ERR("Failed Power-On-Reset: Failed to set RST GPIO pin to "
                "low (err %d)",
                err);
        return err;
    }

    LOG_DBG("Finished Power-On-Reset successfully...");

    return 0;
}

int zmk_mouse_ps2_init_wait_for_mouse(const struct device *dev) {
    const struct zmk_mouse_ps2_config *config = dev->config;
    int err;

    uint8_t read_val;

    for (int i = 0; i < MOUSE_PS2_INIT_ATTEMPTS; i++) {

        LOG_INF("Trying to initialize mouse device (attempt %d / %d)", i + 1,
                MOUSE_PS2_INIT_ATTEMPTS);

        // PS/2 Devices do a self-test and send the result when they power up.

        err = ps2_read(config->ps2_device, &read_val);
        if (err == 0) {
            if (read_val != MOUSE_PS2_RESP_SELF_TEST_PASS) {
                LOG_WRN("Got invalid PS/2 self-test result: 0x%x", read_val);

                LOG_INF("Trying to reset PS2 device...");
                zmk_mouse_ps2_reset(config->ps2_device);

                continue;
            }

            LOG_INF("PS/2 Device passed self-test: 0x%x", read_val);

            // Read device id
            LOG_INF("Reading PS/2 device id...");
            err = ps2_read(config->ps2_device, &read_val);
            if (err) {
                LOG_WRN("Could not read PS/2 device id: %d", err);
            } else {
                if (read_val == 0) {
                    LOG_INF("Connected PS/2 device is a mouse...");
                    return 0;
                } else {
                    LOG_WRN("PS/2 device is not a mouse: 0x%x", read_val);
                    return 1;
                }
            }
        } else {
            LOG_WRN("Could not read PS/2 device self-test result: %d. ", err);
        }

        // But when a zmk device is reset, it doesn't cut the power to external
        // devices. So the device acts as if it was never disconnected.
        // So we try sending the reset command.
        if (i % 2 == 0) {
            LOG_INF("Trying to reset PS2 device...");
            zmk_mouse_ps2_reset(config->ps2_device);
            continue;
        }

        k_sleep(K_SECONDS(5));
    }

    return 1;
}

// Depends on the UART and PS2 init priorities, which are 55 and 45 by default
#define ZMK_MOUSE_PS2_INIT_PRIORITY 90

DEVICE_DT_INST_DEFINE(0, &zmk_mouse_ps2_init, NULL, &zmk_mouse_ps2_data, &zmk_mouse_ps2_config,
                      POST_KERNEL, ZMK_MOUSE_PS2_INIT_PRIORITY, NULL);
