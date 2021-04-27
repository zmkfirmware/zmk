---
title: Mouse Emulation Behaviors
sidebar_label: Mouse Emulation
---

## Summary

Mouse emulation behaviors send mouse events. Currently, only mouse button presses are supported, but movement
and scroll action support is planned for the future.

Whenever the Mouse Emulation feature is turned on or off, the HID protocol used to communicate events to hosts changes. Unfortunately, those changes are not always detected automatically, and might require re-pairing your keyboard to your devices to work over bluetooth. If mouse behaviors are still not recognized by your device after doing that, you can try [these troubleshooting steps](../features/bluetooth.md#windows-connected-but-not-working).

## Configuration Option

This feature can be enabled or disabled explicitly via a config option:

```
CONFIG_ZMK_MOUSE=y
```

If you use the mouse key press behavior in your keymap, the feature will automatically be enabled for you.

## Mouse Button Defines

To make it easier to encode the HID mouse button numeric values, include
the [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) header
provided by ZMK near the top:

```
#include <dt-bindings/zmk/mouse.h>
```

## Mouse Button Press

This behavior can press/release up to 5 mouse buttons.

### Behavior Binding

- Reference: `&mkp`
- Parameter: A `uint8` with bits 0 through 4 each referring to a button.

The following defines can be passed for the parameter:

| Define        | Action         |
| :------------ | :------------- |
| `MB1`, `LCLK` | Left click     |
| `MB2`, `RCLK` | Right click    |
| `MB3`, `MCLK` | Middle click   |
| `MB4`         | Mouse button 4 |
| `MB5`         | Mouse button 5 |

Mouse buttons 4 and 5 typically map to "back" and "forward" actions in most applications.

### Examples

The following will send a left click press when the binding is triggered:

```
&mkp LCLK
```

This example will send press of the fourth mouse button when the binding is triggered:

```
&mkp MB4
```
