---
title: RGB Underglow Behavior
sidebar_label: RGB Underglow
---

## Summary

This page contains [RGB Underglow](../features/underglow.md) behaviors supported by ZMK.

## RGB Action Defines

RGB actions defines are provided through the [`dt-bindings/zmk/rgb.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/rgb.h) header,
which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/rgb.h>
```

This will allow you to reference the actions defined in this header such as `RGB_TOG`.

Here is a table describing the action for each define:

| Define          | Action                                                                                         |
| --------------- | ---------------------------------------------------------------------------------------------- |
| `RGB_ON`        | Turns the RGB feature on                                                                       |
| `RGB_OFF`       | Turns the RGB feature off                                                                      |
| `RGB_TOG`       | Toggles the RGB feature on and off                                                             |
| `RGB_HUI`       | Increases the hue of the RGB feature                                                           |
| `RGB_HUD`       | Decreases the hue of the RGB feature                                                           |
| `RGB_SAI`       | Increases the saturation of the RGB feature                                                    |
| `RGB_SAD`       | Decreases the saturation of the RGB feature                                                    |
| `RGB_BRI`       | Increases the brightness of the RGB feature                                                    |
| `RGB_BRD`       | Decreases the brightness of the RGB feature                                                    |
| `RGB_SPI`       | Increases the speed of the RGB feature effect's animation                                      |
| `RGB_SPD`       | Decreases the speed of the RGB feature effect's animation                                      |
| `RGB_EFF`       | Cycles the RGB feature's effect forwards                                                       |
| `RGB_EFR`       | Cycles the RGB feature's effect reverse                                                        |
| `RGB_COLOR_HSB` | Sets a specific [HSB (HSV)](https://en.wikipedia.org/wiki/HSL_and_HSV) value for the underglow |

## Behavior Binding

- Reference: `&rgb_ug`
- Parameter #1: The RGB action define, e.g. `RGB_TOG` or `RGB_BRI`
- Parameter #2: Only applies to `RGB_COLOR_HSB` and is the HSB representation of the color to set (see below for an example)

:::note HSB Values

When specifying HSB values you'll need to use `RGB_COLOR_HSB(h, s, b)` in your keymap file.

Value Limits:

- Hue values can _not_ exceed 360 (degrees)
- Saturation values can _not_ exceed 100 (percent)
- Brightness values can _not_ exceed 100 (percent)

:::

:::note RGB settings persistence
The RGB settings that are changed via the `&rgb_ug` behavior will be saved to flash storage and hence persist across restarts and firmware flashes.
They will also override the start values set by [`CONFIG_ZMK_RGB_*_START` settings](../config/underglow.md#kconfig).
However the settings will only be saved after [`CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE`](../config/system.md#general) milliseconds in order to reduce potential wear on the flash memory.
:::

## Examples

1. Toggle underglow on/off

   ```
   &rgb_ug RGB_TOG
   ```

1. Set a specific HSB color (green)

   ```
   &rgb_ug RGB_COLOR_HSB(128,100,100)
   ```

## Split Keyboards

RGB underglow behaviors are global: This means that when triggered, they affect both the central and peripheral side of split keyboards.
