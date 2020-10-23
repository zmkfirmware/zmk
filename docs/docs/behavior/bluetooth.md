---
title: Bluetooth Behavior
sidebar_label: Bluetooth
---

## Summary

The bluetooth behavior allows management of various settings and states related to the bluetooth connection(s)
between the keyboard and the host. By default, ZMK supports five "profiles" for selecting which bonded host
computer/laptop/keyboard should receive the keyboard input; many of the commands here operation on those profiles.

## Bluetooth Command Defines

Bluetooth command defines are provided through the [`dt-bindings/zmk/bt.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/bt.h) header,
which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/bt.h>
```

This will allow you to reference the actions defined in this header such as `BT_CLR_CMD`.

Here is a table describing the command for each define:

| Define       | Action                                                                                         |
| ------------ | ---------------------------------------------------------------------------------------------- |
| `BT_CLR_CMD` | Clear bond information between the keyboard and host for the selected profile.                 |
| `BT_NXT_CMD` | Switch to the next profile, cycling through to the first one when the end is reached.          |
| `BT_PRV_CMD` | Switch to the previous profile, cycling through to the last one when the beginning is reached. |
| `BT_SEL_CMD` | Select the 0-indexed profile by number.                                                        |

Because at least one bluetooth commands takes an additional parameter, it is recommended to use
the following aliases in your keymap to avoid having to specify an ignored second parameter:

| Define   | Action                                                                           |
| -------- | -------------------------------------------------------------------------------- |
| `BT_CLR` | Alias for `BT_CLR_CMD 0` to clear the current profile's bond to the current host |
| `BT_NXT` | Alias for `BT_NXT_CMD 0` to select the next profile                              |
| `BT_PRV` | Alias for `BT_PRV_CMD 0` to select the previous profile                          |
| `BT_SEL` | Alias for `BT_SEL_CMD` to select the given profile, e.g. `&bt BT_SEL 1`          |

## Bluetooth Behavior

The bluetooth behavior completes an bluetooth action given on press.

### Behavior Binding

- Reference: `&bt`
- Parameter #1: The bluetooth command define, e.g. `BT_CLR_CMD`
- Parameter #2: (Reserved for future bluetooth command types)

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
