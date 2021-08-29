---
title: Key Press Behaviors
sidebar_label: Key Press
---

## Summary

The most basic of behaviors, is the ability to send certain keycode presses and releases in response to activating
a certain key.

The categories of supported codes are:

- [Keyboard & Keypad](../codes/keyboard-keypad.mdx)
- [Editing](../codes/editing.mdx)
- [Media](../codes/media.mdx)
- [Applications](../codes/applications.mdx)
- [Input Assist](../codes/input-assist.mdx)
- [Power](../codes/power.mdx)

Please visit the [codes](../codes/index.mdx) section for a comprehensive list.

For advanced users, user-defined HID usages are also supported but must be encoded, please see [`dt-bindings/zmk/keys.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/keys.h) for further insight.

## Keycode Defines

To make it easier to encode the HID keycode numeric values, most keymaps include
the [`dt-bindings/zmk/keys.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/keys.h) header
provided by ZMK near the top:

```
#include <dt-bindings/zmk/keys.h>
```

Doing so makes a set of defines such as `A`, `N1`, etc. available for use with these behaviors

### Improperly defined keymap - `dtlib.DTError: <board>.dts.pre.tmp:<line number>`

When compiling firmware from a keymap, it may be common to encounter an error in the form of a`dtlib.DTError: <board>.dts.pre.tmp:<line number>`.
For instructions to resolve such an error, click [here](../troubleshooting.md###Improperly-defined-keymap)

## Key Press

The "key press" behavior sends standard keycodes on press/release.

### Behavior Binding

- Reference: `&kp`
- Parameter: The keycode usage ID from the usage page, e.g. `4` or `A`

Example:

```
&kp A
```
