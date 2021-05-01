---
title: Mouse Wheel Behavior
sidebar_label: Mouse Wheel
---

## Summary

Mouse wheel behavior allows to send keycode signals of mouse wheel scrolling.

## Keycode Defines

To make it easier to encode the HID keycode numeric values, most keymaps include
the [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) header
provided by ZMK near the top:

```
#include <dt-bindings/zmk/mouse.h>
```

Doing so makes a set of defines such as `WHEEL_UP`, `WHEEL_DOWN`, `WHEEL_LEFT` and `WHEEL_RIGHT` available for use with these behaviors.

## Mouse wheel

The "mouse wheel" behavior sends standard keycodes on press/release.

### Behavior Binding

- Reference: `&mwh`
- Parameter: The keycode usage ID.

Example:

```
&mwh WHEEL_UP
```
