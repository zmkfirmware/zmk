---
title: Key Press Behaviors
sidebar_label: Key Press
---

## Summary

The most basic of behaviors, is the ability to send certain keycode presses and releases in response to activating
a certain key.

The categories of supported codes are:

- [Keyboard](../list-of-codes.mdx#keyboard)
- [Modifiers](../list-of-codes.mdx#modifiers)
- [Keypad](../list-of-codes.mdx#keypad)
- [Editing](../list-of-codes.mdx#editing)
- [Media](../list-of-codes.mdx#media)
- [Applications](../list-of-codes.mdx#applications)
- [Input Assist](../list-of-codes.mdx#input-assist)
- [Power & Lock](../list-of-codes.mdx#power--lock)

For advanced users, user-defined HID usages are also supported but must be encoded, please see [`dt-bindings/zmk/keys.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/keys.h) for further insight.

## Keycode Defines

To make it easier to encode the HID keycode numeric values, most keymaps include
the [`dt-bindings/zmk/keys.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/keys.h) header
provided by ZMK near the top:

```dts
#include <dt-bindings/zmk/keys.h>
```

Doing so makes a set of defines such as `A`, `N1`, etc. available for use with these behaviors

## Key Press

The "key press" behavior sends standard keycodes on press/release.

### Behavior Binding

- Reference: `&kp`
- Parameter: The keycode usage ID from the usage page, e.g. `N4` or `A`

Example:

```dts
&kp A
```
