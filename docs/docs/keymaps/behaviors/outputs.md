---
title: Output Selection Behavior
sidebar_label: Output Selection
---

## Summary

The output behavior allows selecting whether keyboard output is sent to the
USB or bluetooth connection when both are connected. This allows connecting a
keyboard to USB for power but outputting to a different device over bluetooth.

By default, output is sent to USB when both USB and BLE are connected.
Once you select a different output, it will be remembered until you change it again.

By default, if USB is selected but only BLE is available or vice versa the keyboard will output to the connected output. If this behavior is not desired you can change it so the keyboard will insist on using the selected output even if it not available using [`CONFIG_ZMK_ENDPOINT_DISABLE_FALLBACK`](../../config/system.md#general)

:::note[Powering the keyboard via USB]
ZMK is not always able to detect if the other end of a USB connection accepts keyboard input or not.
So if you are using USB only to power your keyboard (for example with a charger or a portable power bank), you will want
to select the BLE output through below behavior to be able to send keystrokes to the selected bluetooth profile.
:::

## Output Command Defines

Output command defines are provided through the [`dt-bindings/zmk/outputs.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/outputs.h)
header, which is added at the top of the keymap file:

```dts
#include <dt-bindings/zmk/outputs.h>
```

This allows you to reference the actions defined in this header:

| Define    | Action                                          |
| --------- | ----------------------------------------------- |
| `OUT_USB` | Prefer sending to USB                           |
| `OUT_BLE` | Prefer sending to the current bluetooth profile |
| `OUT_TOG` | Toggle between USB and BLE                      |

## Output Selection Behavior

The output selection behavior changes the preferred output on press.

### Behavior Binding

- Reference: `&out`
- Parameter #1: Command, e.g. `OUT_BLE`

:::note[Output selection persistence]
The endpoint that is selected by the `&out` behavior will be saved to flash storage and hence persist across restarts and firmware flashes.
However it will only be saved after [`CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE`](../../config/system.md#general) milliseconds in order to reduce potential wear on the flash memory.
:::

### Examples

1. Behavior binding to prefer sending keyboard output to USB

   ```dts
   &out OUT_USB
   ```

1. Behavior binding to prefer sending keyboard output to the current bluetooth profile

   ```dts
   &out OUT_BLE
   ```

1. Behavior binding to toggle between preferring USB and BLE

   ```dts
   &out OUT_TOG
   ```
