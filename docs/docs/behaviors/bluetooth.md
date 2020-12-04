---
title: Bluetooth Behavior
sidebar_label: Bluetooth
---

## Summary

The bluetooth behavior allows management of various settings and states related to the bluetooth connection(s)
between the keyboard and the host. By default, ZMK supports five "profiles" for selecting which bonded host
computer/laptop/keyboard should receive the keyboard input; many of the commands here operation on those profiles.

:::note Number of Profiles
Please note there are only five available Bluetooth profiles by default. If you need to increase the number of available profiles you can set `CONFIG_BT_MAX_CONN` in your `zmk-config` `.conf` file.
:::

## Bluetooth Command Defines

Bluetooth command defines are provided through the [`dt-bindings/zmk/bt.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/bt.h) header,
which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/bt.h>
```

This will allow you to reference the actions defined in this header such as `BT_CLR`.

Here is a table describing the command for each define:

| Define   | Action                                                                                                                                                    |
| -------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `BT_CLR` | Clear bond information between the keyboard and host for the selected profile.                                                                            |
| `BT_NXT` | Switch to the next profile, cycling through to the first one when the end is reached.                                                                     |
| `BT_PRV` | Switch to the previous profile, cycling through to the last one when the beginning is reached.                                                            |
| `BT_SEL` | Select the 0-indexed profile by number. Please note: this definition must include a number as an argument in the keymap to work correctly. eg. `BT_SEL 0` |

## Bluetooth Behavior

The bluetooth behavior completes an bluetooth action given on press.

### Behavior Binding

- Reference: `&bt`
- Parameter #1: The bluetooth command define, e.g. `BT_CLR`
- Parameter #2: Only applies to `BT_SEL` and is the 0-indexed profile by number

### Examples

1. Behavior binding to clear the paired host for the selected profile:

   ```
   &bt BT_CLR
   ```

1. Behavior binding to select the next profile:

   ```
   &bt BT_NXT
   ```

1. Behavior binding to select the previous profile:

   ```
   &bt BT_PRV
   ```

1. Behavior binding to select the 2nd profile (passed parameters are [zero based](https://en.wikipedia.org/wiki/Zero-based_numbering)):

   ```
   &bt BT_SEL 1
   ```
