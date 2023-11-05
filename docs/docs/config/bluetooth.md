---
title: Bluetooth Configuration
sidebar_label: Bluetooth
---

See the [bluetooth feature page](../features/bluetooth.md) for more details on the general Bluetooth functionality in ZMK.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

| Option                                | Type | Description                                                                                                             | Default |
| ------------------------------------- | ---- | ----------------------------------------------------------------------------------------------------------------------- | ------- |
| `CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION` | bool | Low level setting for GATT subscriptions. Set to `n` to work around an annoying Windows bug with battery notifications. | y       |

### Security

The following options can be used to mitigate man-in-the-middle (MITM) attacks during initial pairing. These require features which are not necessarily present on all keyboards, so they are not enabled by default.

| Option                           | Type | Description                             | Default              |
| -------------------------------- | ---- | --------------------------------------- | -------------------- |
| `CONFIG_ZMK_BLE_PASSKEY_CONFIRM` | bool | Enable pairing with numeric comparison. | n                    |
| `CONFIG_ZMK_BLE_PASSKEY_DISPLAY` | bool | Enable passkey display during pairing.  | `CONFIG_ZMK_DISPLAY` |
| `CONFIG_ZMK_BLE_PASSKEY_ENTRY`   | bool | Enable passkey entry during pairing.    | n                    |

`CONFIG_ZMK_BLE_PASSKEY_ENTRY` enables pairing with passkey entry on the keyboard. The host will display a 6-digit passkey which you must type on the keyboard, then press `ENTER` to complete pairing. Pressing `BACKSPACE` will delete the last digit, and pressing `ESCAPE` will cancel pairing. This requires that you have the digits `N0`-`N9` or `KP_N0`-`KP_N9` and `ENTER` or `KP_ENTER` bound to your keymap.

`CONFIG_ZMK_BLE_PASSKEY_DISPLAY` enables pairing with passkey entry on the host. The keyboard will display a 6-digit passkey which you must enter on the host to complete pairing. This requires only that your keyboard has a display, so it is automatically enabled with `CONFIG_ZMK_DISPLAY`.

`CONFIG_ZMK_BLE_PASSKEY_CONFIRM` enables pairing with numeric comparison. Both the host and keyboard will display a 6-digit passkey. You must check that they match, then press `ENTER` on they keyboard and confirm on the host as well to complete pairing. Pressing `ESCAPE` will cancel pairing. This requires that your keyboard has a display and you have `ENTER` or `KP_ENTER` bound to your keymap.

If multiple of the above options are enabled, the host will typically try to use numeric comparison first, then fallback to one of the passkey entry options.
