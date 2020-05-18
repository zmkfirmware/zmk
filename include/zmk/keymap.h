#pragma once

#include <devicetree.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>
#include <dt-bindings/zmk/keys.h>

#include <zmk/matrix.h>
#include <zmk/keys.h>

#define ZMK_KEYMAP_NODE DT_CHOSEN(zmk_keymap)
#define ZMK_KEYMAP_LAYERS_LEN DT_PROP_LEN(ZMK_KEYMAP_NODE, layers)

#define _ZMK_LAYER_ENUM_ITEM(label) DT_CAT(label, _layer)

#define _ZMK_KEYMAP_GENERATE_LAYER_CONST(node_id) \
    _ZMK_LAYER_ENUM_ITEM(DT_NODELABEL(node_id)),

enum zmk_keymap_layer
{
    DT_FOREACH_CHILD(DT_INST(0, zmk_layers), _ZMK_KEYMAP_GENERATE_LAYER_CONST)
};

bool zmk_keymap_layer_activate(u8_t layer);
bool zmk_keymap_layer_deactivate(u8_t layer);

zmk_key
zmk_keymap_keycode_from_position(u32_t row, u32_t column);
