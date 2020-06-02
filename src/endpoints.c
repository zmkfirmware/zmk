
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

#ifdef CONFIG_ZMK_USB
    err = zmk_usb_hid_init();
    if (err)
    {
        LOG_ERR("USB HID Init Failed\n");
        return err;
    }
#endif /* CONFIG_ZMK_USB */

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

int zmk_endpoints_send_report(enum zmk_hid_report_changes report_type)
{
    int err;
    struct zmk_hid_keypad_report *keypad_report;
    struct zmk_hid_consumer_report *consumer_report;
    switch (report_type)
    {
    case Keypad:
        keypad_report = zmk_hid_get_keypad_report();
#ifdef CONFIG_ZMK_USB
        if (zmk_usb_hid_send_report((u8_t *)keypad_report, sizeof(struct zmk_hid_keypad_report)) != 0)
        {
            LOG_DBG("USB Send Failed");
        }
#endif /* CONFIG_ZMK_USB */

#ifdef CONFIG_ZMK_BLE
        err = zmk_hog_send_keypad_report(&keypad_report->body);
        if (err)
        {
            LOG_ERR("FAILED TO SEND OVER HOG: %d", err);
        }
#endif /* CONFIG_ZMK_BLE */

        break;
    case Consumer:
        consumer_report = zmk_hid_get_consumer_report();
#ifdef CONFIG_ZMK_USB
        if (zmk_usb_hid_send_report((u8_t *)consumer_report, sizeof(struct zmk_hid_consumer_report)) != 0)
        {
            LOG_DBG("USB Send Failed");
        }
#endif /* CONFIG_ZMK_USB */

#ifdef CONFIG_ZMK_BLE
        err = zmk_hog_send_consumer_report(&consumer_report->body);
        if (err)
        {
            LOG_ERR("FAILED TO SEND OVER HOG: %d", err);
        }
#endif /* CONFIG_ZMK_BLE */

        break;
    default:
        LOG_ERR("Unknown report change type %d", report_type);
        return -EINVAL;
    }

    return 0;
}

int zmk_endpoints_send_key_event(struct zmk_key_event key_event)
{
    enum zmk_hid_report_changes changes;

    LOG_DBG("key %d, state %d\n", key_event.key, key_event.pressed);

    if (key_event.pressed)
    {
        changes = zmk_hid_press_key(key_event.key);
    }
    else
    {
        changes = zmk_hid_release_key(key_event.key);
    }

    return zmk_endpoints_send_report(changes);
}
