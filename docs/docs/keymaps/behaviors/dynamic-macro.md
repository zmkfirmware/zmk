---
title: Dynamic Macro Behavior
sidebar_label: Dynamic Macro
---

## Summary

The dynamic macro behavior allows management of dynamically recorded macros. By default, ZMK supports one "slot"
to record to, though this may be increased.

## Dynamic Macro Command Defines

Dynamic Macro command defines are provided through the [`dt-bindings/zmk/dynamic_macro.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/dynamic_macro.h) header,
which is added at the top of the keymap file:

```dts
#include <dt-bindings/zmk/dynamic_macro.h>
```

This will allow you to reference the actions defined in this header such as `DM_REC`.

Here is a table describing the command for each define:

| Define   | Action                                                |
| -------- | ----------------------------------------------------- |
| `DM_REC` | Begins recording to the given macro slot.             |
| `DM_PLY` | Executes the recorded macro for the given macro slot. |
| `DM_STP` | Stops recording to the given macro slot.              |

## Dynamic Macro Behavior

The dynamic macro behavior completes a dynamic macro action given on press.

### Examples

1. Behavior binding to begin recording to the specified macro slot (passed parameters are [zero based](https://en.wikipedia.org/wiki/Zero-based_numbering)):

   ```dts
   &bt DM_REC 0
   ```

1. Behavior binding to stop recording to the specified macro slot:

   ```dts
   &bt DM_STP 0
   ```

1. Behavior binding to execute the macro recorded in the specified macro slot:

   ```dts
   &bt DM_PLY 0
   ```

### Configuration

The number of macro slots, event length, and tap delay can be configured. See [`behaviors.md`](../../config/behaviors.md#dynamic-macro) for more details.

:::note[Number of Slots]
Please note there is one available macro slot by default. If you need to adjust the number of available slots, set `CONFIG_BEHAVIOR_DYNAMIC_MACRO_MAX_SLOTS`.
:::

:::note[Number of Events]
Please note that the number of events that can be recorded to a macro is limited. If you need to adjust the number of events to record, set `CONFIG_BEHAVIOR_DYNAMIC_MACRO_MAX_EVENTS`.
:::

:::note[Delay Between Events]
An artificial delay is added between each event. If the delay needs to be adjusted, set `CONFIG_BEHAVIOR_DYNAMIC_MACRO_TAP_DELAY`.
:::
