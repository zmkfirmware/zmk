---
title: Mouse Emulation Behaviors
sidebar_label: Mouse Emulation
---

## Summary

Mouse emulation behaviors send mouse movements, button presses or scroll actions.

Please view [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) for a comprehensive list of signals.

## Configuration options

This feature should be enabled via a config option:

```
CONFIG_ZMK_MOUSE=y
```

## Keycode Defines

To make it easier to encode the HID keycode numeric values, most keymaps include
the [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) header
provided by ZMK near the top:

```
#include <dt-bindings/zmk/mouse.h>
```

Doing so allows using a set of defines such as `LCLK` and `RCLK` with these behaviors.

## Mouse Button Press

This behavior can press/release up to 16 mouse buttons.

### Behavior Binding

- Reference: `&mkp`
- Parameter: A `uint16` with each bit referring to a button.

Example:

```
&mkp LCLK
```
