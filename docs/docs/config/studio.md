---
title: ZMK Studio Configuration
sidebar_label: ZMK Studio
---

The following settings affect the ZMK Studio portions of ZMK. See the [ZMK Studio feature](../features/studio.md) for more information on enabling and building with ZMK Studio enabled.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/src/studio/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/studio/Kconfig)

### Lighting and encoders in this fork

A client that implements this fork’s protocol can query lighting capabilities and change settings on an unlocked keyboard. Basic lighting controls require Studio RPC, LED Matrix, and the LED Matrix behavior. Enable `CONFIG_SETTINGS` and `CONFIG_ZMK_LED_MATRIX_EDITOR` for persistent editing of presets and groups. See the [LED Matrix reference](../led-matrix-led-composite-reference.mdx).

With `CONFIG_ZMK_STUDIO` and keymap sensors, a compatible client can read or change the clockwise and counterclockwise bindings for each encoder on each layer. These bindings use the keymap save and discard operations.

### Keymaps

| Config                                 | Type | Description                             | Default |
| -------------------------------------- | ---- | --------------------------------------- | ------- |
| `CONFIG_ZMK_KEYMAP_LAYER_NAME_MAX_LEN` | int  | Max allowable keymap layer display name | 20      |

### Locking

| Config                                    | Type | Description                                                                         | Default |
| ----------------------------------------- | ---- | ----------------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_STUDIO_LOCKING`               | bool | Enable/disable locking for ZMK Studio                                               | y       |
| `CONFIG_ZMK_STUDIO_LOCK_IDLE_TIMEOUT_SEC` | int  | Seconds of inactivity in ZMK Studio before automatically locking                    | 500     |
| `CONFIG_ZMK_STUDIO_LOCK_ON_DISCONNECT`    | bool | Whether to automatically lock again whenever ZMK Studio disconnects from the device | y       |

### Transport/Protocol Details

| Config                                         | Type | Description                                                                   | Default                               |
| ---------------------------------------------- | ---- | ----------------------------------------------------------------------------- | ------------------------------------- |
| `CONFIG_ZMK_STUDIO_TRANSPORT_BLE_PREF_LATENCY` | int  | Lower latency to request while ZMK Studio is active to improve responsiveness | 10                                    |
| `CONFIG_ZMK_STUDIO_RPC_THREAD_STACK_SIZE`      | int  | Stack size per RPC, notification and enabled GATT TX thread                   | 4096                                  |
| `CONFIG_ZMK_STUDIO_RPC_RX_BUF_SIZE`            | int  | Incoming buffer (also fits one ATT write when BLE transport is enabled)       | 96 with lighting editor, otherwise 30 |
| `CONFIG_ZMK_STUDIO_RPC_TX_BUF_SIZE`            | int  | Maximum framed bytes staged per outgoing transfer                             | 64                                    |
