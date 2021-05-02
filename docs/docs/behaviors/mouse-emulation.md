---
title: Mouse Emulation Behaviors
sidebar_label: Mouse Emulation
---

## Summary

Mouse emulation behaviors send mouse movements, button presses or wheel actions.

Please view [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) for a comprehensive list of signals.

## Keycode Defines

To make it easier to encode the HID keycode numeric values, most keymaps include
the [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) header
provided by ZMK near the top:

```
#include <dt-bindings/zmk/mouse.h>
```

Doing so allows using a set of defines such as `MOVE_UP`, `MOVE_DOWN`, `LCLK` and `WHEEL_UP` with these behaviors.

## Mouse Button Press

This behavior can press/release up to 16 mouse buttons.

### Behavior Binding

- Reference: `&mkp`
- Parameter: A `uint16` with each bit referring to a button.

Example:

```
&mkp LCLK
```

## Mouse Movement

This behavior is used to manipulate the cursor.

### Behavior Binding

- Reference: `&mmv`
- Parameter: A `uint32` with the first 16 bits relating to horizontal movement
  and the last 16 - to vertical movement.

Example:

```
&mmv MOVE_UP
```

## Mouse Wheel

This behaviour is used to scroll, both horizontally and vertically.

### Behavior Binding

- Reference: `&mwh`
- Parameter: A `uint16` with the first 8 bits relating to horizontal movement
  and the last 8 - to vertical movement.

Example:

```
&mwh WHEEL_UP
```
