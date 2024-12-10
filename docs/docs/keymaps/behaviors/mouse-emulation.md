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

```ini
CONFIG_ZMK_POINTING=y
```

## Mouse Emulation Defines

To make it easier to encode the HID mouse button and move/scroll speed numeric values, include
the [`dt-bindings/zmk/pointing.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/pointing.h) header
provided by ZMK near the top:

```dts
#include <dt-bindings/zmk/pointing.h>
```

Should you wish to override the default movement or scrolling max velocities, you can define the defaults before including the header, e.g.:

```c
#define ZMK_POINTING_DEFAULT_MOVE_VAL 1500  // default: 600
#define ZMK_POINTING_DEFAULT_SCRL_VAL 20    // default: 10

#include <dt-bindings/zmk/pointing.h>
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

```dts
&mkp LCLK
```

This example will send press of the fourth mouse button when the binding is triggered:

```dts
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

Additionally, if you want to pass a different max speed than the default for the `MOVE_*` defines, custom X and Y velocity values can be passed with `MOVE_X` and `MOVE_Y`, e.g. `MOVE_X(100)` or `MOVE_Y(-100)`. Positive values indicate movement directions right or down. Note that the default value of the max speed depends on [the value of `ZMK_POINTING_DEFAULT_MOVE_VAL`](#mouse-emulation-defines).

### Examples

The following will send a down mouse movement event to the host when pressed/held:

```dts
&mmv MOVE_DOWN
```

The following will send a left mouse movement event to the host when pressed/held:

```dts
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

Additionally, if you want to pass a different max speed than the default for the `SCRL_*` defines, custom X and Y velocity values can be passed with `MOVE_X` and `MOVE_Y`, e.g. `MOVE_X(5)` or `MOVE_Y(-5)`. Positive values indicate scroll directions right or up. Note that the default value of the max speed depends on [the value of `ZMK_POINTING_DEFAULT_SCRL_VAL`](#mouse-emulation-defines).

### Examples

The following will send a scroll down event to the host when pressed/held:

```dts
&msc SCRL_DOWN
```

The following will send a scroll left event to the host when pressed/held:

```dts
&msc SCRL_LEFT
```

:::note

If you enabled [smooth scrolling](../../config/pointing.md#kconfig) then you will want to use the same `MOVE_UP`, `MOVE_DOWN`, etc values instead of the smaller `SCRL_*` parameters for more sensible scroll speeds.

:::

### Input Processors

If you want to apply any [input processors](../input-processors/index.md#input-processors-overview) to `&msc` you can do so by referencing `&msc_input_listener`, e.g.:

```dts
&msc_input_listener {
    input-processors = <&zip_temp_layer 2 2000>;
}
```

## Advanced Configuration

Both `&mmv` and `&msc` are instances of the `"zmk,behavior-input-two-axis"` behavior and can be modified using the [two axis input behavior](../../config/behaviors.md#two-axis-input) configuration properties. The default settings are as follows:

### Mouse Move

```dts
&mmv {
    x-input-code = <INPUT_REL_X>;
    y-input-code = <INPUT_REL_Y>;
    time-to-max-speed-ms = <300>;
    acceleration-exponent = <1>;
};
```

### Mouse Scroll

```dts
&msc {
    x-input-code = <INPUT_REL_HWHEEL>;
    y-input-code = <INPUT_REL_WHEEL>;
    time-to-max-speed-ms = <300>;
    acceleration-exponent = <0>;
};
```
