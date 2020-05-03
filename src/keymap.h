#ifndef ZMK_KEYMAP_H
#define ZMK_KEYMAP_H

#include <devicetree.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include "zmk.h"

#define ZMK_KEYMAP_NODE DT_CHOSEN(zmk_keymap)
#define ZMK_KEYMAP_LAYERS_LEN DT_PROP_LEN(ZMK_KEYMAP_NODE,layers)


enum hid_kbd_code zmk_keymap_keycode_from_position(u32_t row, u32_t column);

#endif
