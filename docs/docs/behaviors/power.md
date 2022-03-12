---
title: Power Management Behaviors
sidebar_label: Power Management
---

## Summary

These page contains some of the power management behaviors currently supported by ZMK.

## External Power Control

The External power control behavior allows enabling or disabling the VCC power output
to save power. Some of the LEDs will consume power even in OFF state. To preserve
battery life in this scenario, some controller boards have support to disable the
external power completely.

The following boards currently support this feature:

- nRFMicro
- nice!nano

## External Power Control Command Defines

External power control command defines are provided through the [`dt-bindings/zmk/ext_power.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/ext_power.h) header,
which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/ext_power.h>
```

This will allow you to reference the actions defined in this header such as `EXT_POWER_OFF_CMD`.

Here is a table describing the command for each define:

| Define                 | Action                      | Alias    |
| ---------------------- | --------------------------- | -------- |
| `EXT_POWER_OFF_CMD`    | Disable the external power. | `EP_OFF` |
| `EXT_POWER_ON_CMD`     | Enable the external power.  | `EP_ON`  |
| `EXT_POWER_TOGGLE_CMD` | Toggle the external power.  | `EP_TOG` |

### Behavior Binding

- Reference: `&ext_power`
- Parameter#1: Command, e.g `EP_ON`

### Example:

1. Behavior binding to enable the external power

   ```
   &ext_power EP_ON
   ```

1. Behavior binding to disable the external power

   ```
   &ext_power EP_OFF
   ```

1. Behavior binding to toggle the external power

   ```
   &ext_power EP_TOG
   ```

## Split Keyboards

Power management behaviors are global: This means that when triggered, they affects both the central and peripheral side of split keyboards.
