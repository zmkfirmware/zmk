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

| Define   | Action                                   |
| -------- | ---------------------------------------- |
| `BL_TOG` | Toggles the backlight on and off         |
| `BL_ON`  | Turn on backlight on and off             |
| `BL_OFF` | Toggles the backlight feature on and off |
| `BL_INC` | Increase backlight brightness            |
| `BL_DEC` | Decrease backlight brightness            |

## Behavior Binding

- Reference: `&bl`
- Parameter #1: The backlight action define, e.g. `BL_TOG` or `BL_INC`

### Examples

1. Toggle backlight on/off

   ```
   &bl BL_TOG
   ```

1. Increase backlight brightness

   ```
   &bl BL_INC
   ```
