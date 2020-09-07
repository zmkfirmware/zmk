---
title: Lighting Behavior
sidebar_label: Lighting
---

## Summary

Lighting is often used for either aesthetics or for the practical purposes of lighting up keys in the dark.
Currently ZMK supports RGB underglow, which can be changed and configured using its behavior.

## RGB Action Defines

RGB actions defines are provided through the [`dt-bindings/zmk/rgb.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/rgb.h) header,
which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/rgb.h>
```

This will allow you to reference the actions defined in this header such as `RGB_TOG`.

Here is a table describing the action for each define:

| Define    | Action                                                    |
| --------- | --------------------------------------------------------- |
| `RGB_TOG` | Toggles the RGB feature on and off                        |
| `RGB_HUI` | Increases the hue of the RGB feature                      |
| `RGB_HUD` | Decreases the hue of the RGB feature                      |
| `RGB_SAI` | Increases the saturation of the RGB feature               |
| `RGB_SAD` | Decreases the saturation of the RGB feature               |
| `RGB_BRI` | Increases the brightness of the RGB feature               |
| `RGB_BRD` | Decreases the brightness of the RGB feature               |
| `RGB_SPI` | Increases the speed of the RGB feature effect's animation |
| `RGB_SPD` | Decreases the speed of the RGB feature effect's animation |
| `RGB_EFF` | Cycles the RGB feature's effect forwards                  |
| `RGB_EFR` | Cycles the RGB feature's effect reverse                   |

## RGB Underglow

The "RGB underglow" behavior completes an RGB action given on press.

### Behavior Binding

- Reference: `&rgb_ug`
- Parameter: The RGB action define, e.g. `RGB_TOG` or `RGB_BRI`

Example:

```
&rgb_ug RGB_TOG
```
