---
title: Key Press Behaviors
sidebar_label: Key Press
---

## Summary

The most basic of behaviors, is the ability to send certain keycode presses and releases in response to activating
a certain key.

For reference on keycode values, see pages 83-89 of the [USB HID Usage Tables](https://www.usb.org/document-library/hid-usage-tables-12).

## Keycode Defines

To make it easier to encode the HID keycode numeric values, most keymaps include
the [`dt-bindings/zmk/keys.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/keys.h) header
provided by ZMK near the top:

```
#include <dt-bindings/zmk/keys.h>
```

Doing so makes a set of defines such as `A`, `NUM_1`, etc. available for use with these behaviors

:::note
There is an [open issue](https://github.com/zmkfirmware/zmk/issues/21) to provide a more comprehensive, and
complete set of defines for the full keypad and consumer usage pages in the future for ZMK.
:::

### Improperly defined keymap - `dtlib.DTError: <board>.dts.pre.tmp:<line number>`

When compiling firmware from a keymap, it may be common to encounter an error in the form of a`dtlib.DTError: <board>.dts.pre.tmp:<line number>`.
For instructions to resolve such an error, click [here](../troubleshooting###Improperly-defined-keymap)

## Keypad Key Press

The "keypad key press" behavior sends standard keypad keycodes on press/release.

### Behavior Binding

- Reference: `&kp`
- Parameter: The keycode usage ID from the keypad usage page, e.g. `4` or `A`

Example:

```
&kp A
```

## Consumer Key Press

The "consumer key press" behavior allows you to send "consumer" usage page keycodes on press/release.
These are mostly used for media and power related keycodes, such as sending "Pause", "Scan Track Next",
"Scan Track Previous", etc.

There are a subset of the full consumer usage IDs found in the `keys.h` include, prefixed with `C_`, e.g. `C_PREV`.

### Behavior Binding

- Reference: `&cp`
- Parameter: The keycode usage ID from the consumer usage page, e.g. `C_PREV` or `C_EJECT`

Example:

```
&cp C_PREV
```
