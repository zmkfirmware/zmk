---
title: Backlight Behavior
sidebar_label: Backlight
---

## Summary

This page contains [backlight](../features/backlight.md) behaviors supported by ZMK.

## Backlight Action Defines

Backlight actions defines are provided through the [`dt-bindings/zmk/backlight.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/backlight.h) header,
which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/backlight.h>
```

This will allow you to reference the actions defined in this header such as `BL_TOG`.

Here is a table describing the action for each define:

| Define     | Action                      |
| ---------- | --------------------------- |
| `BL_ON`    | Turn on backlight           |
| `BL_OFF`   | Turn off backlight          |
| `BL_TOG`   | Toggle backlight on and off |
| `BL_INC`   | Increase brightness         |
| `BL_DEC`   | Decrease brightness         |
| `BL_CYCLE` | Cycle brightness            |
| `BL_SET`   | Set a specific brightness   |

## Behavior Binding

- Reference: `&bl`
- Parameter #1: The backlight action define, e.g. `BL_TOG` or `BL_INC`
- Parameter #2: Only applies to `BL_SET`and is the brightness value

:::note Backlight settings persistence
The backlight settings that are changed via the `&bl` behavior will be saved to flash storage and hence persist across restarts and firmware flashes.
They will also override the start values set by [`CONFIG_ZMK_BACKLIGHT_*_START` settings](../config/backlight.md#kconfig).
However the settings will only be saved after [`CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE`](../config/system.md#general) milliseconds in order to reduce potential wear on the flash memory.
:::

### Examples

1. Toggle backlight on/off

   ```
   &bl BL_TOG
   ```

1. Sets a specific brightness

   ```
   &bl BL_SET 50
   ```

## Split Keyboards

Backlight behaviors are global: This means that when triggered, they affect both the central and peripheral side of split keyboards.
