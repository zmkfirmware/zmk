---
title: Mouse Press Behavior
sidebar_label: Mouse Press
---

## Summary

Mouse press behavior allows to send keycode presses and releases of mouse buttons.

Please visit view [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) for a comprehensive list.

## Keycode Defines

To make it easier to encode the HID keycode numeric values, most keymaps include
the [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) header
provided by ZMK near the top:

```
#include <dt-bindings/zmk/mouse.h>
```

Doing so makes a set of defines such as `LCLK`, `RCLK`, etc. available for use with these behaviors

## Mouse press

The "mouse press" behavior sends standard keycodes on press/release.

### Behavior Binding

- Reference: `&mp`
- Parameter: The keycode usage ID.

Example:

```
&mp LCLK
```
