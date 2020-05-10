#pragma once

#include <devicetree.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>
#include "dt-bindings/zmk/keys.h"

#include "zmk.h"

#define ZMK_KEYMAP_NODE DT_CHOSEN(zmk_keymap)
#define ZMK_KEYMAP_LAYERS_LEN DT_PROP_LEN(ZMK_KEYMAP_NODE, layers)

typedef u64_t zmk_key;

bool zmk_keymap_layer_activate(u8_t layer);
bool zmk_keymap_layer_deactivate(u8_t layer);

zmk_key
zmk_keymap_keycode_from_position(u32_t row, u32_t column);
