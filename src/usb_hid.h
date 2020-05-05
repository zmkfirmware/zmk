#ifndef ZMK_USB_HID
#define ZMK_USB_HID

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

int zmk_usb_hid_init();

// TODO: Modifiers!

int zmk_usb_hid_press_key(enum hid_kbd_code code);
int zmk_usb_hid_release_key(enum hid_kbd_code code);

#endif
