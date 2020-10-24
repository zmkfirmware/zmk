---
title: Endpoint Behavior
sidebar_label: Endpoints
---

## Summary

The endpoint behavior allows selecting whether keyboard input is sent to the
USB or bluetooth connection when both are connected. This allows connecting a
keyboard to USB for power but sending input to a different device over bluetooth.

By default, keyboard input is sent to USB when both endpoints are connected.
Once you select a different endpoint, it will be remembered until you change it again.

## Endpoints Command Defines

Endpoints command defines are provided through the [`dt-bindings/zmk/endpoints.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/endpoints.h)
header, which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/endpoints.h>
```

This allows you to reference the actions defined in this header:

| Define                | Action                                               | Alias     |
| --------------------- | ---------------------------------------------------- | --------- |
| `ENDPOINT_USB_CMD`    | Send keyboard input to USB                           | `END_USB` |
| `ENDPOINT_BLE_CMD`    | Send keyboard input to the current bluetooth profile | `END_BLE` |
| `ENDPOINT_TOGGLE_CMD` | Toggle between USB and BLE                           | `END_TOG` |

## Endpoints Behavior

The endpoints behavior changes the preferred endpoint on press.

### Behavior Binding

- Reference: `&end`
- Parameter #1: Command, e.g. `END_BLE`

### Example:

1. Behavior binding to prefer sending keyboard input to USB

    ```
    &end END_USB
    ```

1. Behavior binding to prefer sending keyboard input to the current bluetooth profile

    ```
    &end END_BLE
    ```

1. Behavior binding to toggle between preferring USB and BLE

    ```
    &end END_TOG
    ```
