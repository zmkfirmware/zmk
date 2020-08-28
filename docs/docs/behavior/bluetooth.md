---
title: Bluetooth Behavior
sidebar_label: Bluetooth
---

## Summary

The bluetooth behavior allows management of various settings and states related to the bluetooth connection(s)
between the keyboard and the host.

## Bluetooth Command Defines

Bluetooth command defines are provided through the [`dt-bindings/zmk/bt.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/bt.h) header,
which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/bt.h>
```

This will allow you to reference the actions defined in this header such as `BT_IDENT_CLR_CMD`.

Here is a table describing the command for each define:

| Define              | Action                                                |
| ------------------- | ----------------------------------------------------- |
| `BT_IDENT_CLR_CMD`  | Clear paired keyboards (for the current identity)[^1] |
| `BT_IDENT_NEXT_CMD` | Switch to the next identity[^1]                       |
| `BT_IDENT_PREV_CMD` | Switch to the previous identity                       |
| `BT_IDENT_SEL_CMD`  | Switch to a specific numbered identity                |
| `BT_RST_CMD`        | Hard reset of all bluetooth bonds                     |

Because the `BT_IDENT_SEL_CMD` command takes an additional parameter, the numeric index of the identity to select, _all_ the commands for the bluetooth behavior must take that additional parameter, and ignore the value. To make this easier,
there are alias defines that add those default parameters for you:

| Define          | Action                                                                                  |
| --------------- | --------------------------------------------------------------------------------------- |
| `BT_IDENT_CLR`  | Alias for `BT_IDENT_CLR_CMD 0` to clear paired keyboards (for the current identity)[^1] |
| `BT_IDENT_NEXT` | Alias for `BT_IDENT_NEXT_CMD 0` to switch to the next identity[^1]                      |
| `BT_IDENT_PREV` | Alias for `BT_IDENT_PREV_CMD 0` to switch to the previous identity                      |
| `BT_IDENT_SEL`  | Alias for `BT_IDENT_SEL_CMD` to switch to a specific numbered identity                  |
| `BT_RST`        | Alias for `BT_RST_CMD 0` to reset all bonds[^2]                                         |

[^1]: Multiple keyboard identities/profiles is only support on non-split keyboards as of this time.
[^2]: This may interrupt pairing between both halves of a split keyboard. For split keyboards, it is preferred to use the [/docs/bond-reset] combo to clear bonds on both devices

## Bluetooth Behavior

The bluetooth behavior completes an bluetooth action given on press.

### Behavior Binding

- Reference: `&bt`
- Parameter #1: The bluetooth command define, e.g. `BT_IDENT_CLR_CMD` or `BT_RST_CMD`
- Parameter #2: The parameter for the command, when used to identify an identity/profile to select, e.g. `0`

### Examples

1. Behavior to clear the paired host:

   ```
   &bt BT_IDENT_CLR
   ```

1. Behavior to hard reset all bonded devices[^2]:

   ```
   &bt BT_IDENT_CLR
   ```

Examples for non-split keyboards with multiple identities:

1. Behavior to switch to the next identity:

   ```
   &bt BT_IDENT_NEXT
   ```

1. Behavior to switch to the specific numbered identity:

   ```
   &bt BT_IDENT_SEL 1
   ```
