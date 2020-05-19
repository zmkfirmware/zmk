
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/usb_hid.h>
#include <zmk/hog.h>

int zmk_endpoints_init()
{
    int err;

    err = zmk_usb_hid_init();
    if (err)
    {
        printk("USB HID Init Failed\n");
        return err;
    }

#ifdef CONFIG_ZMK_BLE
    err = zmk_hog_init();
    if (err)
    {
        printk("HOG Init Failed\n");
        return err;
    }

#endif /* CONFIG_ZMK_BLE */

    return 0;
}

int zmk_endpoints_send_key_event(struct zmk_key_event key_event)
{
    struct zmk_hid_report *report;
    int err;


    if (key_event.pressed)
    {
        zmk_hid_press_key(key_event.key);
    }
    else
    {
        zmk_hid_release_key(key_event.key);
    }

    report = zmk_hid_get_report();

    // if (zmk_usb_hid_send_report(report) != 0)
    // {
    //     // LOG_DBG("USB Send Failed");
    // }

#ifdef CONFIG_ZMK_BLE
    err = zmk_hog_send_report(report);
    if (err)
    {
        printk("FAILED TO SEND OVER HOG: %d\n", err);
        // LOG_DBG("HID Over GATTP Send Failed");
    }
#endif /* CONFIG_ZMK_BLE */

    return 0;
}
