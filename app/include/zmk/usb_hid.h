#ifndef ZMK_USB_HID
#define ZMK_USB_HID

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include <zmk/keys.h>
#include <zmk/hid.h>

int zmk_usb_hid_init();

int zmk_usb_hid_send_report(u8_t *report, size_t len);

#endif
