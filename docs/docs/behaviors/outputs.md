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

:::note Powering the keyboard via USB
ZMK is not always able to detect if the other end of a USB connection accepts keyboard input or not.
So if you are using USB only to power your keyboard (for example with a charger or a portable power bank), you will want
to select the BLE output through below behavior to be able to send keystrokes to the selected bluetooth profile.
:::

## Output Command Defines

Output command defines are provided through the [`dt-bindings/zmk/outputs.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/outputs.h)
header, which is added at the top of the keymap file:

```
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

### Examples

1. Behavior binding to prefer sending keyboard output to USB

   ```
   &out OUT_USB
   ```

1. Behavior binding to prefer sending keyboard output to the current bluetooth profile

   ```
   &out OUT_BLE
   ```

1. Behavior binding to toggle between preferring USB and BLE

   ```
   &out OUT_TOG
   ```
   
### Split keyboard example and default behaviour

In split keyboard left / right nicenano settings, the problem of *unresponsive bluetooth connection* can occur. If usb cable is connected on the left, the USB mode will be selected, and bluetooth mode turned off, no matter if the USB is connected to your laptop or usb power source. 
If usb cable is connected to the right nicenano, only the bluetooth mode will be activated and the usb mode will be turned off.

The USB mode is indicated on an optional OLED by a USB icon on the main or left OLED display.
The Bluetooth mode is indicated either by a GEAR icon (pairing mode) or Wifi icon (connected mode).
However, due to the operating system design, there are situations where the Wifi icon could show connected Bluetooth keyboard and no input is shown, for instance after firmware update on the nicenano.

USB mode will disable the .keymap &bt BT_CLR &bt BT_SEL 0 ... commands and they will be unresponsive.
Bluetooth mode will enable the .keymap &bt commands.

For troubleshooting bluetooth, it is recommended to connect to the right nicenano, issue a BT_CLR command, delete the bluetooth fevice in the operating system and issue BT_SEL 1 to start t he pairing process on another bt profile for the os to notice.
