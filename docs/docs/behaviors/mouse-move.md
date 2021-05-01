---
title: Mouse Move Behavior
sidebar_label: Mouse Move
---

## Summary

Mouse move behavior allows to send keycode signals of mouse movement.

Please visit view [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) for a comprehensive list of signals.

## Keycode Defines

To make it easier to encode the HID keycode numeric values, most keymaps include
the [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) header
provided by ZMK near the top:

```
#include <dt-bindings/zmk/mouse.h>
```

Doing so makes a set of defines such as `MOVE_UP`, `MOVE_DOWN`, `MOVE_LEFT` and `MOVE_RIGHT` available for use with these behaviors.

## Mouse press

The "mouse move" behavior sends standard keycodes on press/release.

### Behavior Binding

- Reference: `&mmv`
- Parameter: The keycode usage ID.

Example:

```
&mmv MOVE_UP
```
