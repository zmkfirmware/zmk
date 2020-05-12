#ifndef ZMK_USB_HID
#define ZMK_USB_HID

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include "keys.h"

int zmk_usb_hid_init();

// TODO: Modifiers!

int zmk_usb_hid_press_key(zmk_key key);
int zmk_usb_hid_release_key(zmk_key key);

#endif
