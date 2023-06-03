---
title: Dynamic Macro Behavior
sidebar_label: Dynamic Macros
---

## Summary

The dynamic macro behavior allows creating macros and replaying them by recording key presses. While recording a macro, you can also play another macro.

:::note
Dynamic macros are cleared on reboot.
:::

:::warning
Dynamic macros are memory intensive, and may cause the firmware to crash. It is recommended to use only 1 dynamic macro in your keymap and re-record it if it needs changed. The maximum amount of actions to be recorded can be set with `CONFIG_ZMK_DYNAMIC_MACRO_MAX_ACTIONS` (default 64).
:::

## Dynamic Macro Action Defines

Dynamic macro action defines are provided through the [`dt-bindings/zmk/dynamic-macros.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/dynamic-macros.h) header,
which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/dynamic-macros.h>
```

This will allow you to reference the actions defined in this header such as `PLAY`.

Here is a table describing the action for each define:

| Define   | Action                      |
| -------- | --------------------------- |
| `PLAY`   | Play back a recorded macro  |
| `RECORD` | Toggle recording of a macro |

## Macro Definition

A dynamic macro definition looks like:

```
/ {
    macros {
        dyn-macro: dyn-macro {
            label = "ZM_dynamic-macro";
            compatible = "zmk,behavior-dynamic-macro";
            #binding-cells = <1>;
        };
    };
};
```

The macro can then be bound in your keymap by referencing it by the label `dyn-macro` followed by PLAY or RECORD, e.g.:

```
    / {
    keymap {
        &dyn-macro PLAY &dyn-macro RECORD
        ...
    };
};
```

## Configuration

### Wait Time

The wait time setting controls how long of a delay is introduced between behaviors. By default, a macro will play back at the speed it
was recorded, but it can be overwritten by assigning a value to the `wait-ms` property of the macro, e.g. `wait-ms = <20>;`.

### No Output

By default, keystrokes will still be sent to the host while a dynamic macro is recording. Setting `no-output` will change this and will not send keystrokes to the host while recording.
