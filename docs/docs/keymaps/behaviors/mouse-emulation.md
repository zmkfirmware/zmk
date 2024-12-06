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

## Mouse Emulation Defines

To make it easier to encode the HID mouse button and move/scroll speed numeric values, include
the [`dt-bindings/zmk/mouse.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/mouse.h) header
provided by ZMK near the top:

```
#include <dt-bindings/zmk/mouse.h>
```

Should you wish to override the default movement or scrolling max velocities, you can define the defaults before including the header, e.g.:

```c
#define ZMK_MOUSE_DEFAULT_MOVE_VAL 1500  // default: 600
#define ZMK_MOUSE_DEFAULT_SCRL_VAL 20    // default: 10

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

### Input Processors

If you want to apply any [input processors](../input-processors/index.md#input-processors-overview) to `&mkp` you can do so by referencing `&mkp_input_listener`, e.g.:

```dts
&mkp_input_listener {
    input-processors = <&zip_temp_layer 2 2000>;
}
```

## Mouse Move

This behavior sends mouse X/Y movement events to the connected host.

### Behavior Binding

- Reference: `&mmv`
- Parameter: A `uint32` with 16-bits each used for vertical and horizontal max velocity.

The following predefined values can be passed for the parameter:

| Define       | Action     |
| :----------- | :--------- |
| `MOVE_UP`    | Move up    |
| `MOVE_DOWN`  | Move down  |
| `MOVE_LEFT`  | Move left  |
| `MOVE_RIGHT` | Move right |

Additionally, if you want to pass a different max speed than the default for the `MOVE_*` defines, custom X and Y velocity values can be passed with `MOVE_X` and `MOVE_Y`, e.g. `MOVE_X(100)` or `MOVE_Y(-100)`. Positive values indicate movement directions right or down.

### Examples

The following will send a down mouse movement event to the host when pressed/held:

```
&mmv MOVE_DOWN
```

The following will send a left mouse movement event to the host when pressed/held:

```
&mmv MOVE_LEFT
```

### Input Processors

If you want to apply any [input processors](../input-processors/index.md#input-processors-overview) to `&mmv` you can do so by referencing `&mmv_input_listener`, e.g.:

```dts
&mmv_input_listener {
    input-processors = <&zip_temp_layer 2 2000>;
}
```

## Mouse Scroll

This behavior sends vertical and horizontal scroll events to the connected host.

### Behavior Binding

- Reference: `&msc`
- Parameter: A `uint32` with 16-bits each used for vertical and horizontal velocity.

The following defines can be passed for the parameter:

| Define       | Action       |
| :----------- | :----------- |
| `SCRL_UP`    | Scroll up    |
| `SCRL_DOWN`  | Scroll down  |
| `SCRL_LEFT`  | Scroll left  |
| `SCRL_RIGHT` | Scroll right |

Additionally, if you want to pass a different max speed than the default for the `SCRL_*` defines, custom X and Y velocity values can be passed with `MOVE_X` and `MOVE_Y`, e.g. `MOVE_X(5)` or `MOVE_Y(-5)`. Positive values indicate scroll directions right or up.

### Examples

The following will send a scroll down event to the host when pressed/held:

```
&msc SCRL_DOWN
```

The following will send a scroll left event to the host when pressed/held:

```
&msc SCRL_LEFT
```

:::note

If you enabled [smooth scrolling](../../config/pointing.md#kconfig) then you will want to use the same `MOVE_UP`, `MOVE_DOWN`, etc values instead of the smaller `SCRL_*` parameters.

:::

### Input Processors

If you want to apply any [input processors](../input-processors/index.md#input-processors-overview) to `&msc` you can do so by referencing `&msc_input_listener`, e.g.:

```dts
&msc_input_listener {
    input-processors = <&zip_temp_layer 2 2000>;
}
```

### Advanced Configuration

Both `&mmv` and `&msc` are instances of the same `"zmk,behavior-input-two-axis"` behavior. As such, the following settings can be applied to either behavior, e.g.:

```dts
&mmv {
    trigger-period-ms = <12>;
    delay-ms = <15>;
    time-to-max-speed-ms = <600>;
    acceleration-exponent = <1>;
};
```

| Property                | Description                                                                                                         |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `trigger-period-ms`     | How many milliseconds between generated input events based on the current speed/direction.                          |
| `delay-ms`              | How many milliseconds to delay any processing or event generation when first pressed.                               |
| `time-to-max-speed-ms`  | How many milliseconds it takes to accelerate to the curren max speed.                                               |
| `acceleration-exponent` | The acceleration exponent to apply: `0` - uniform speed, `1` - uniform acceleration, `2` - exponential acceleration |
