#ifndef ZMK_USB_HID
#define ZMK_USB_HID

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include "keys.h"
#include "hid.h"

int zmk_usb_hid_init();

int zmk_usb_hid_send_report(const struct zmk_hid_report *report);

#endif
