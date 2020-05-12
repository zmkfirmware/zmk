
#include "endpoints.h"
#include "usb_hid.h"

int zmk_endpoints_send_key_event(struct zmk_key_event key_event)
{
    if (key_event.pressed)
    {
        zmk_usb_hid_press_key(key_event.key);
    }
    else
    {
        zmk_usb_hid_release_key(key_event.key);
    }

    return 0;
}