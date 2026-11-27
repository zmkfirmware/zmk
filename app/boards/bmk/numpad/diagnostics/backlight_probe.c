/* SPDX-License-Identifier: MIT */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_gpio.h>

#include <drivers/ext_power.h>
#include <zmk/activity.h>
#include <zmk/backlight.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/position_state_changed.h>

LOG_MODULE_REGISTER(numpad_lcd_diag, LOG_LEVEL_INF);

#define LIGHT_NODE DT_CHILD(DT_CHOSEN(zmk_backlight), screen_backlight)
#define POWER_NODE DT_INST(0, zmk_ext_power_generic)

/* Raw register observations below are deliberately specific to this PCB. */
BUILD_ASSERT(DT_SAME_NODE(DT_GPIO_CTLR(LIGHT_NODE, gpios), DT_NODELABEL(gpio0)) &&
                 DT_GPIO_PIN(LIGHT_NODE, gpios) == 8,
             "LCD diagnostic expects backlight on P0.08");
BUILD_ASSERT(DT_SAME_NODE(DT_GPIO_CTLR(POWER_NODE, control_gpios), DT_NODELABEL(gpio0)) &&
                 DT_GPIO_PIN(POWER_NODE, control_gpios) == 12,
             "LCD diagnostic expects LCD power on P0.12");

static const struct device *const power_dev = DEVICE_DT_GET(POWER_NODE);
static const struct device *const light_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_backlight));

static void snapshot(const char *reason) {
    /* OUT is a software latch, NOT a voltage measurement. Reading PIN_CNF and
     * OUT does not connect the input buffer or change pin ownership/direction.
     */
    uint32_t out = NRF_P0->OUT;
    uint32_t light_cnf = NRF_P0->PIN_CNF[8];
    uint32_t power_cnf = NRF_P0->PIN_CNF[12];
    int power_state = device_is_ready(power_dev) ? ext_power_get(power_dev) : -ENODEV;

    LOG_INF("%s t=%u activity=%d bl_ready=%d bl_on=%d brt=%u power=%d "
            "P0.08_OUT=%u CNF08=0x%08x P0.12_OUT=%u CNF12=0x%08x",
            reason, k_uptime_get_32(), (int)zmk_activity_get_state(), device_is_ready(light_dev),
            zmk_backlight_is_on(), zmk_backlight_get_brt(), power_state, (out >> 8) & 1U, light_cnf,
            (out >> 12) & 1U, power_cnf);
}

static void sample_work_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sample_work, sample_work_cb);

static void sample_work_cb(struct k_work *work) {
    ARG_UNUSED(work);
    snapshot("sample");
    k_work_reschedule(&sample_work, K_SECONDS(1));
}

static int diagnostic_event(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *key = as_zmk_position_state_changed(eh);
    const struct zmk_activity_state_changed *activity = as_zmk_activity_state_changed(eh);
    if (key) {
        LOG_INF("key t=%u pos=%u down=%d", k_uptime_get_32(), key->position, key->state);
    } else if (activity) {
        LOG_INF("activity_event t=%u state=%d", k_uptime_get_32(), activity->state);
    }
    /* Let all event subscribers finish before observing the resulting state. */
    k_work_reschedule(&sample_work, K_MSEC(1));
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(numpad_lcd_diag, diagnostic_event);
ZMK_SUBSCRIPTION(numpad_lcd_diag, zmk_position_state_changed);
ZMK_SUBSCRIPTION(numpad_lcd_diag, zmk_activity_state_changed);

static int diagnostic_init(void) {
    LOG_INF("read-only probe: settings=%d auto_idle=%d auto_usb=%d blank_idle=%d; "
            "OUT/CNF are registers, not EN voltage",
            IS_ENABLED(CONFIG_SETTINGS), IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE),
            IS_ENABLED(CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB),
            IS_ENABLED(CONFIG_ZMK_DISPLAY_BLANK_ON_IDLE));
    /* APPLICATION init runs before main() loads settings. This sample can be
     * compared to subsequent samples to identify saved-state restoration.
     */
    snapshot("before_settings");
    k_work_reschedule(&sample_work, K_MSEC(1));
    return 0;
}

BUILD_ASSERT(CONFIG_APPLICATION_INIT_PRIORITY < 99,
             "LCD probe must initialize after ZMK backlight");
SYS_INIT(diagnostic_init, APPLICATION, 99);
