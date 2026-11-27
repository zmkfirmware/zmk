// SPDX-License-Identifier: MIT
#include <pb_decode.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/ztest.h>
#include <zmk/endpoints.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/led_matrix_state_changed.h>
#include "msg_framing.h"
#include "rpc_tx.h"

static struct zmk_endpoint_instance endpoint = {.transport = ZMK_TRANSPORT_USB};
struct zmk_endpoint_instance zmk_endpoint_get_selected(void) { return endpoint; }

static void select_endpoint(enum zmk_transport transport) {
    endpoint.transport = transport;
    raise_zmk_endpoint_changed((struct zmk_endpoint_changed){.endpoint = endpoint});
}

// The UART work item models CDC's independently scheduled readiness callback.
static uart_irq_callback_user_data_t serial_irq;
static void *irq_arg;
static bool tx_enabled, stall;
static uint8_t wire[256];
static size_t wire_len;
static int stall_after = -1;
static K_SEM_DEFINE(attempted, 0, 1);
static void irq_work_fn(struct k_work *work) {
    serial_irq(DEVICE_DT_GET(DT_NODELABEL(studio_test_uart)), irq_arg);
}
static K_WORK_DEFINE(irq_work, irq_work_fn);
static void enable(const struct device *dev) {
    tx_enabled = true;
    k_work_submit(&irq_work);
}
static void disable(const struct device *dev) { tx_enabled = false; }
static void noop(const struct device *dev) {}
static int ready(const struct device *dev) { return tx_enabled; }
static int yes(const struct device *dev) { return 1; }
static int no(const struct device *dev) { return 0; }
static void set_cb(const struct device *dev, uart_irq_callback_user_data_t cb, void *arg) {
    serial_irq = cb;
    irq_arg = arg;
}
static int fill(const struct device *dev, const uint8_t *bytes, int len) {
    k_sem_give(&attempted);
    if (stall) {
        return 0;
    }
    int n = MIN(len, 3);
    if (stall_after >= 0) {
        n = MIN(n, stall_after - (int)wire_len);
        if (!n) {
            stall = true;
            return 0;
        }
    }
    zassert_true(wire_len + n <= sizeof(wire));
    memcpy(wire + wire_len, bytes, n);
    wire_len += n;
    k_work_submit(&irq_work);
    return n;
}
static DEVICE_API(uart, test_uart_api) = {
    .irq_tx_enable = enable,
    .irq_tx_disable = disable,
    .irq_tx_ready = ready,
    .irq_update = yes,
    .irq_rx_ready = no,
    .irq_rx_enable = noop,
    .irq_rx_disable = noop,
    .irq_callback_set = set_cb,
    .fifo_fill = fill,
};
DEVICE_DT_DEFINE(DT_NODELABEL(studio_test_uart), NULL, NULL, NULL, NULL, PRE_KERNEL_1,
                 CONFIG_SERIAL_INIT_PRIORITY, &test_uart_api);

ZMK_RPC_SUBSYSTEM(core)
static zmk_studio_Response get_lock_state(const zmk_studio_Request *req) {
    return ZMK_RPC_RESPONSE(core, get_lock_state, ZMK_STUDIO_CORE_LOCK_STATE_UNLOCKED);
}
ZMK_RPC_SUBSYSTEM_HANDLER(core, get_lock_state, ZMK_STUDIO_RPC_HANDLER_UNSECURED);

static void setup_case(void *fixture) {
    stall_after = -1;
    stall = false;
    select_endpoint(ZMK_TRANSPORT_USB);
    k_msleep(20);
    wire_len = 0;
    k_sem_reset(&attempted);
}

// A valid request with an ID containing a reserved wire byte tests framing as well.
static void request_and_check(void) {
    // request_id=171, core.get_lock_state=true
    const uint8_t request[] = {FRAMING_SOF, 0x08, FRAMING_ESC, 0xab, 0x01,
                               0x1a,        0x02, 0x10,        0x01, FRAMING_EOF};
    zassert_ok(zmk_rpc_rx_put(endpoint.transport, request, sizeof(request)));
    k_msleep(35);
    enum studio_framing_state state = FRAMING_STATE_IDLE;
    uint8_t payload[128];
    size_t count = 0;
    bool found = false;
    for (size_t i = 0; i < wire_len; i++) {
        if (studio_framing_process_byte(&state, wire[i])) {
            zassert_true(count < sizeof(payload));
            payload[count++] = wire[i];
        }
        if (state == FRAMING_STATE_ERR) {
            count = 0;
        } else if (state == FRAMING_STATE_EOF) {
            pb_istream_t stream = pb_istream_from_buffer(payload, count);
            zmk_studio_Response response = zmk_studio_Response_init_zero;
            if (pb_decode(&stream, zmk_studio_Response_fields, &response) &&
                response.which_type == zmk_studio_Response_request_response_tag) {
                zassert_equal(response.type.request_response.request_id, 171);
                zassert_equal(response.type.request_response.which_subsystem,
                              zmk_studio_RequestResponse_core_tag);
                found = true;
            }
            count = 0;
        }
    }
    zassert_true(found, "missing complete response (%u bytes)", (unsigned)wire_len);
}

ZTEST(studio_transport, test_notifications) {
    raise_zmk_led_matrix_state_changed((struct zmk_led_matrix_state_changed){
        .reason_mask =
            ZMK_LED_MATRIX_STATE_CHANGE_STATUS | ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR});
    k_msleep(20);
    zassert_equal(wire_len, 0);
    raise_zmk_led_matrix_state_changed((struct zmk_led_matrix_state_changed){
        .reason_mask = ZMK_LED_MATRIX_STATE_CHANGE_BRIGHTNESS});
    k_msleep(20);
    const uint8_t expected[] = {FRAMING_SOF, 0x12, 4, 0x32, 2, 8, 1, FRAMING_EOF};
    zassert_equal(wire_len, sizeof(expected));
    zassert_mem_equal(expected, wire, sizeof(expected));
}

static K_SEM_DEFINE(input_done, 0, 1);
static K_SEM_DEFINE(display_done, 0, 1);
static void input_work_fn(struct k_work *work) {
    raise_zmk_led_matrix_state_changed(
        (struct zmk_led_matrix_state_changed){.reason_mask = ZMK_LED_MATRIX_STATE_CHANGE_PROFILE});
    k_sem_give(&input_done);
}
static void display_work_fn(struct k_work *work) { k_sem_give(&display_done); }
static K_WORK_DEFINE(input_work, input_work_fn);
static K_WORK_DEFINE(display_work, display_work_fn);

ZTEST(studio_transport, test_stalled_uart_keeps_system_queue_running) {
    stall = true;
    k_work_submit(&input_work);
    zassert_ok(k_sem_take(&input_done, K_MSEC(100)));
    zassert_ok(k_sem_take(&attempted, K_MSEC(100)));
    k_work_submit(&display_work);
    zassert_ok(k_sem_take(&display_done, K_MSEC(100)));
    k_msleep(1100);
    uint8_t byte;
    uint32_t ticket;
    zassert_equal(studio_tx_peek(ZMK_TRANSPORT_USB, &byte, 1, &ticket), 0);
    stall = false;
    request_and_check();
}

static bool reject_encode(pb_ostream_t *s, const pb_field_t *field, void *const *arg) {
    return false;
}
ZTEST(studio_transport, test_encoder_error_releases_writer) {
    zmk_lighting_Catalog catalog = {.profiles.funcs.encode = reject_encode};
    zmk_studio_Response response = ZMK_RPC_RESPONSE(lighting, catalog, catalog);
    zassert_equal(studio_tx_send(&response, studio_tx_session()), -EINVAL);
    request_and_check();
}

ZTEST(studio_transport, test_disconnect_discards_inflight_frame) {
    stall = true;
    k_work_submit(&input_work);
    zassert_ok(k_sem_take(&attempted, K_MSEC(100)));
    uint8_t byte;
    uint32_t old;
    zassert_equal(studio_tx_peek(ZMK_TRANSPORT_USB, &byte, 1, &old), 1);
    const uint8_t partial[] = {FRAMING_SOF, 8};
    zassert_ok(zmk_rpc_rx_put(ZMK_TRANSPORT_USB, partial, sizeof(partial)));
    k_msleep(10);
    select_endpoint(ZMK_TRANSPORT_NONE);
    select_endpoint(ZMK_TRANSPORT_USB);
    studio_tx_complete(old, 1);
    stall = false;
    request_and_check();
}

// Only the radio and peer confirmation are simulated; the production GATT worker runs.
static char peer;
static bool hold_ack;
static struct bt_gatt_indicate_params *inflight;
struct bt_conn *zmk_ble_active_profile_conn(void) { return (struct bt_conn *)&peer; }
void __wrap_bt_conn_unref(struct bt_conn *conn) {}
uint16_t __wrap_bt_gatt_get_mtu(struct bt_conn *conn) { return 23; }
bool __wrap_bt_gatt_is_subscribed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                  uint16_t ccc) {
    return true;
}
static void confirm(void) {
    struct bt_gatt_indicate_params *params = inflight;
    inflight = NULL;
    params->func((struct bt_conn *)&peer, params, 0);
    params->destroy(params);
}
int __wrap_bt_gatt_indicate(struct bt_conn *conn, struct bt_gatt_indicate_params *params) {
    zassert_is_null(inflight);
    zassert_true(wire_len + params->len <= sizeof(wire));
    memcpy(wire + wire_len, params->data, params->len);
    wire_len += params->len;
    inflight = params;
    if (!hold_ack) {
        confirm();
    }
    return 0;
}

ZTEST(studio_transport, test_late_ble_ack_after_reconnect) {
    select_endpoint(ZMK_TRANSPORT_BLE);
    hold_ack = true;
    k_work_submit(&input_work);
    k_msleep(20);
    zassert_not_null(inflight);
    select_endpoint(ZMK_TRANSPORT_NONE);
    select_endpoint(ZMK_TRANSPORT_BLE);
    k_work_submit(&input_work);
    k_msleep(20);
    hold_ack = false;
    confirm(); // completion for the old link cannot acknowledge the queued new frame
    k_msleep(20);
    request_and_check();
}

ZTEST(studio_transport, test_timeout_after_escape_recovers_framing) {
    // Capture a normal multi-chunk response and interrupt a second copy at its escape prefix.
    request_and_check();
    uint8_t *escape = memchr(wire, FRAMING_ESC, wire_len);
    zassert_not_null(escape);
    stall_after = escape - wire + 1;
    wire_len = 0;
    const uint8_t request[] = {FRAMING_SOF, 8, FRAMING_ESC, 0xab, 1, 0x1a, 2, 0x10, 1, FRAMING_EOF};
    zassert_ok(zmk_rpc_rx_put(ZMK_TRANSPORT_USB, request, sizeof(request)));
    k_msleep(1100);
    zassert_equal(wire_len, stall_after);
    zassert_equal(wire[wire_len - 1], FRAMING_ESC);
    stall_after = -1;
    stall = false;
    request_and_check();
}

ZTEST_SUITE(studio_transport, NULL, NULL, setup_case, NULL, NULL);
