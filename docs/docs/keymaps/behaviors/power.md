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

| Define                 | Action                      | Alias    |
| ---------------------- | --------------------------- | -------- |
| `EXT_POWER_OFF_CMD`    | Disable the external power. | `EP_OFF` |
| `EXT_POWER_ON_CMD`     | Enable the external power.  | `EP_ON`  |
| `EXT_POWER_TOGGLE_CMD` | Toggle the external power.  | `EP_TOG` |

### Behavior Binding

- Reference: `&ext_power`
- Parameter#1: Command, e.g `EP_ON`

:::note[External power state persistence]
The on/off state that is set by the `&ext_power` behavior will be saved to flash storage and hence persist across restarts and firmware flashes.
However it will only be saved after [`CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE`](../../config/system.md#general) milliseconds in order to reduce potential wear on the flash memory.
:::

### Example:

1. Behavior binding to enable the external power

   ```dts
   &ext_power EP_ON
   ```

1. Behavior binding to disable the external power

   ```dts
   &ext_power EP_OFF
   ```

1. Behavior binding to toggle the external power

   ```dts
   &ext_power EP_TOG
   ```

## Split Keyboards

Power management behaviors are [global](../../features/split-keyboards.md#global-locality-behaviors): This means that when triggered, they affects both the central and peripheral side of split keyboards.
