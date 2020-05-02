#ifndef ZMK_KEYMAP_H
#define ZMK_KEYMAP_H

#include <devicetree.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

// TODO: Pull these in fro a kscan_gpio.h file from Zephyr!
//
#define MATRIX_NODE_ID DT_PATH(kscan)
#define MATRIX_ROWS DT_PROP_LEN(MATRIX_NODE_ID,row_gpios)
#define MATRIX_COLS DT_PROP_LEN(MATRIX_NODE_ID,col_gpios)

enum hid_kbd_code keymap[MATRIX_ROWS][MATRIX_COLS] = {
	{ HID_KEY_A, HID_KEY_B },
	{ HID_KEY_C, HID_KEY_D }
};

#define zmk_keymap_keycode_from_position(row, column) keymap[row][column]

#endif
