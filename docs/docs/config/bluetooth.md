---
title: Bluetooth Configuration
sidebar_label: Bluetooth
---

See the [bluetooth feature page](../features/bluetooth.md) for more details on the general Bluetooth functionality in ZMK.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

| Option                                 | Type | Description                                                                                                                                                                                                                                                                                                                        | Default |
| -------------------------------------- | ---- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_BLE_EXPERIMENTAL_FEATURES` | bool | Enables a combination of settings that are planned to be default in future versions of ZMK. This includes changes to timing on BLE pairing initation, BT Secure Connection passkey entry, restores use of the updated/new LLCP implementation, disables 2M PHY support, and allows overwrite of keys from previously paired hosts. | n       |
| `CONFIG_ZMK_BLE_PASSKEY_ENTRY`         | bool | Enable passkey entry during pairing for enhanced security. (Note: After enabling this, you will need to re-pair all previously paired hosts)                                                                                                                                                                                       | n       |
| `CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION`  | bool | Low level setting for GATT subscriptions. Set to `n` to work around an annoying Windows bug with battery notifications.                                                                                                                                                                                                            | y       |
