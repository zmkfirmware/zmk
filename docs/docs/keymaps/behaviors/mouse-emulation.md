---
title: Mouse Emulation Behaviors
sidebar_label: Mouse Emulation
---

## Summary

Mouse emulation behaviors send mouse events. Currently, only mouse button presses are supported, but movement
and scroll action support is planned for the future.

:::warning[Refreshing the HID descriptor]

Enabling or disabling the mouse emulation feature modifies the HID report descriptor and requires it to be [refreshed](../../features/bluetooth.md#refreshing-the-hid-descriptor).
The mouse functionality will not work over BLE until that is done.

:::

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


==========================================
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

This option enables several others.

### Dedicated thread processing

`CONFIG_ZMK_MOUSE_WORK_QUEUE_DEDICATED` is enabled by default and separates the processing of mouse signals into a dedicated thread, significantly improving performance.

### Tick rate configuration

`CONFIG_ZMK_MOUSE_TICK_DURATION` sets the tick rate for mouse polling. It is set to 8 ms. by default.

## Keycode Defines

To make it easier to encode the HID keycode numeric values, most keymaps include
the [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) header
provided by ZMK near the top:

```
#include <dt-bindings/zmk/mouse.h>
```

Doing so allows using a set of defines such as `MOVE_UP`, `MOVE_DOWN`, `LCLK` and `SCROLL_UP` with these behaviors.

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

## Mouse Scrolling

This behaviour is used to scroll, both horizontally and vertically.

### Behavior Binding

- Reference: `&mwh`
- Parameter: A `uint16` with the first 8 bits relating to horizontal movement
  and the last 8 - to vertical movement.

Example:

```
&mwh SCROLL_UP
```

## Acceleration

Both mouse movement and scrolling have independently configurable acceleration profiles with three parameters: delay before movement, time to max speed and the acceleration exponent.
The exponent is usually set to 0 for constant speed, 1 for uniform acceleration or 2 for uniform jerk.

These profiles can be configured inside your keymap:

```
&mmv {
    time-to-max-speed-ms = <500>;
};

&mwh {
    acceleration-exponent=<1>;
};

/ {
    keymap {
        ...
    };
};
```
