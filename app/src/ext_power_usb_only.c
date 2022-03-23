#include <settings/settings.h>
#include <drivers/ext_power.h>
#include <zmk/activity.h>
#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_EXT_POWER_USB_ONLY)

static bool initialized = false;

void zmk_ext_power_usb_only_toggle() {

    // Do not touch external power until properly initialized.
    // This prevents cutting external power until displays and other
    // components are properly initialized
    if (initialized == false) {
        LOG_DBG("zmk_ext_power_usb_only not initialized yet. Ignoring.");
        return;
    }

    const struct device *ext_power = device_get_binding("EXT_POWER");
    const int ext_power_enabled = ext_power_get(ext_power);

    if (zmk_usb_is_powered() == true && ext_power_enabled == false) {
        LOG_DBG("USB power was connected. Enabling external power.");
        ext_power_enable(ext_power);
    } else if (zmk_usb_is_powered() == false && ext_power_enabled == true) {
        LOG_DBG("USB power was removed. Disabling external power.");
        ext_power_disable(ext_power);
    }
}

static int zmk_ext_power_usb_only_event_listener(const zmk_event_t *eh) {
    if (as_zmk_usb_conn_state_changed(eh)) {
        LOG_DBG("USB conn state changed: %d", zmk_usb_is_powered());

        zmk_ext_power_usb_only_toggle();
    }

    return 0;
}

ZMK_LISTENER(ext_power, zmk_ext_power_usb_only_event_listener);
ZMK_SUBSCRIPTION(ext_power, zmk_usb_conn_state_changed);

static int zmk_ext_power_usb_only_init(const struct device *_arg) {

    LOG_DBG("Running zmk_ext_power_usb_only_init");

    initialized = true;
    zmk_ext_power_usb_only_toggle();

    return 0;
}

// Initialized ext power usb only after everything else has been
// initialized.
//
// This avoids interrupting the iniitalization of displays for example.
#define ZMK_EXT_POWER_USB_ONLY_INIT_PRIORITY CONFIG_APPLICATION_INIT_PRIORITY+100
SYS_INIT(zmk_ext_power_usb_only_init, APPLICATION, ZMK_EXT_POWER_USB_ONLY_INIT_PRIORITY);

#endif
