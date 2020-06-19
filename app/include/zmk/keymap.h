#pragma once

#include <devicetree.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>
#include <dt-bindings/zmk/keys.h>

#include <zmk/matrix.h>
// #include <zmk/keys.h>

bool zmk_keymap_layer_activate(u8_t layer);
bool zmk_keymap_layer_deactivate(u8_t layer);

int zmk_keymap_position_state_changed(u32_t row, u32_t column, bool pressed);
