
#include "endpoints.h"
#include "hid.h"
#include "usb_hid.h"
#include "hog.h"

int zmk_endpoints_init()
{
    int err;

    err = zmk_usb_hid_init();
    if (err)
    {
        printk("USB HID Init Failed\n");
        return err;
    }

    err = zmk_hog_init();
    if (err)
    {
        printk("HOG Init Failed\n");
        return err;
    }

    return 0;
}

int zmk_endpoints_send_key_event(struct zmk_key_event key_event)
{
    struct zmk_hid_report *report;
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

    if (zmk_hog_send_report(report) != 0)
    {
        // LOG_DBG("HID Over GATTP Send Failed");
    }

    return 0;
}