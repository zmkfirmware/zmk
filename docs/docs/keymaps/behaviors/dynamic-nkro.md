---
title: Dynamic NKRO Behavior
sidebar_label: Dynamic NKRO
---

## Summary

The dynamic NKRO behavior switches the keyboard's HID report type between full
N-key roll over (NKRO) and "boot keyboard" compatible 6-key roll over (HKRO) at
runtime, so a single firmware build can use NKRO day to day and fall back to HKRO
for hosts that only understand boot keyboards, such as some BIOS/UEFI screens.

This behavior requires [`CONFIG_ZMK_HID_REPORT_TYPE_DYNAMIC`](../../config/system.md#hid)
to be enabled, which compiles in both report types. The `HKRO`/`NKRO` config
choice sets the mode used on the very first boot; after that the selected mode is
remembered.

:::note[Why it reboots]
A HID report descriptor can't change while the keyboard is enumerated with a host.
Switching modes therefore saves the new mode to flash and reboots to apply it, so
expect a brief disconnect when you use this behavior.
:::

## Dynamic NKRO Command Defines

Command defines are provided through the [`dt-bindings/zmk/dynamic_nkro.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/dynamic_nkro.h)
header, which is added at the top of the keymap file:

```dts
#include <dt-bindings/zmk/dynamic_nkro.h>
```

This allows you to reference the actions defined in this header:

| Define          | Action                   |
| --------------- | ------------------------ |
| `DYN_NKRO_TOG`  | Toggle between NKRO/HKRO |
| `DYN_NKRO_NKRO` | Switch to NKRO           |
| `DYN_NKRO_HKRO` | Switch to HKRO           |

## Dynamic NKRO Behavior

The dynamic NKRO behavior changes the active report mode on press. If the
keyboard is already in the requested mode, nothing happens.

### Behavior Binding

- Reference: `&dyn_nkro`
- Parameter #1: Command, e.g. `DYN_NKRO_TOG`

### Examples

1. Toggle between NKRO and HKRO

   ```dts
   &dyn_nkro DYN_NKRO_TOG
   ```

1. Switch to full NKRO

   ```dts
   &dyn_nkro DYN_NKRO_NKRO
   ```

1. Switch to HKRO

   ```dts
   &dyn_nkro DYN_NKRO_HKRO
   ```
