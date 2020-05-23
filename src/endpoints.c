
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/usb_hid.h>
#include <zmk/hog.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

int zmk_endpoints_init()
{
    int err;

    LOG_DBG("");

    err = zmk_usb_hid_init();
    if (err)
    {
        LOG_ERR("USB HID Init Failed\n");
        return err;
    }

#ifdef CONFIG_ZMK_BLE
    err = zmk_hog_init();
    if (err)
    {
        LOG_ERR("HOG Init Failed\n");
        return err;
    }

#endif /* CONFIG_ZMK_BLE */

    return 0;
}

int zmk_endpoints_send_report()
{
    int err;
    struct zmk_hid_report *report = zmk_hid_get_report();

    // if (zmk_usb_hid_send_report(report) != 0)
    // {
    //     // LOG_DBG("USB Send Failed");
    // }

#ifdef CONFIG_ZMK_BLE
    err = zmk_hog_send_report(report);
    if (err)
    {
        LOG_ERR("FAILED TO SEND OVER HOG: %d", err);
    }
#endif /* CONFIG_ZMK_BLE */

    return 0;
}

int zmk_endpoints_send_key_event(struct zmk_key_event key_event)
{
    struct zmk_hid_report *report;
    int err;

    LOG_DBG("key %d, state %d\n", key_event.key, key_event.pressed);

    if (key_event.pressed)
    {
        zmk_hid_press_key(key_event.key);
    }
    else
    {
        zmk_hid_release_key(key_event.key);
    }

    return zmk_endpoints_send_report();
}
