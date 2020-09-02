---
title: Bluetooth Behavior
sidebar_label: Bluetooth
---

## Summary

The bluetooth behavior allows management of various settings and states related to the bluetooth connection(s)
between the keyboard and the host. As of right now, there is only one such action support, but in the future
more will be added.

## Bluetooth Command Defines

Bluetooth command defines are provided through the [`dt-bindings/zmk/bt.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/bt.h) header,
which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/bt.h>
```

This will allow you to reference the actions defined in this header such as `BT_CLEAR_BONDS_CMD`.

Here is a table describing the command for each define:

| Define               | Action                                                    |
| -------------------- | --------------------------------------------------------- |
| `BT_CLEAR_BONDS_CMD` | Clear bond information between the keyboard and host [^1] |

Because future bluetooth commands will take an additional parameter, it is recommended to use
the following alias in your keymap to avoid having to change it later.

| Define           | Action                                                                 |
| ---------------- | ---------------------------------------------------------------------- |
| `BT_CLEAR_BONDS` | Alias for `BT_CLEAR_BONDS_CMD 0` to clear the bond to the current host |

## Bluetooth Behavior

The bluetooth behavior completes an bluetooth action given on press.

### Behavior Binding

- Reference: `&bt`
- Parameter #1: The bluetooth command define, e.g. `BT_CLEAR_BONDS_CMD`
- Parameter #2: (Reserved for future bluetooth command types)

### Examples

1. Behavior to clear the paired host:

   ```
   &bt BT_CLEAR_BONDS
   ```
