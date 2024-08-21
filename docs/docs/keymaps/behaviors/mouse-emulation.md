---
title: Mouse Emulation Behaviors
sidebar_label: Mouse Emulation
---

## Summary

Mouse emulation behaviors send mouse events, including mouse button presses, cursor movement and scrolling.

:::warning[Refreshing the HID descriptor]

Enabling or disabling the mouse emulation feature modifies the HID report descriptor and requires it to be [refreshed](../../features/bluetooth.md#refreshing-the-hid-descriptor).
The mouse functionality will not work over BLE until that is done.

:::

## Configuration Option

To use any of the behaviors documented here, the ZMK mouse feature must be enabled explicitly via a config option:

```
CONFIG_ZMK_MOUSE=y
```

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

## Mouse Move

This behavior sends mouse X/Y movement events to the connected host.

### Behavior Binding

- Reference: `&mmv`
- Parameter: A `uint32` with 16-bits each used for vertical and horizontal velocity.

The following defines can be passed for the parameter:

| Define       | Action     |
| :----------- | :--------- |
| `MOVE_UP`    | Move up    |
| `MOVE_DOWN`  | Move down  |
| `MOVE_LEFT`  | Move left  |
| `MOVE_RIGHT` | Move right |

### Examples

The following will send a scroll down event to the host when pressed/held:

```
&mmv MOVE_DOWN
```

The following will send a scroll left event to the host when pressed/held:

```
&mmv MOVE_LEFT
```

## Mouse Scroll

This behavior sends vertical and horizontal scroll events to the connected host.

### Behavior Binding

- Reference: `&msc`
- Parameter: A `uint32` with 16-bits each used for vertical and horizontal velocity.

The following defines can be passed for the parameter:

| Define       | Action     |
| :----------- | :--------- |
| `MOVE_UP`    | Move up    |
| `MOVE_DOWN`  | Move down  |
| `MOVE_LEFT`  | Move left  |
| `MOVE_RIGHT` | Move right |

### Examples

The following will send a scroll down event to the host when pressed/held:

```
&msc MOVE_DOWN
```

The following will send a scroll left event to the host when pressed/held:

```
&msc MOVE_LEFT
```
