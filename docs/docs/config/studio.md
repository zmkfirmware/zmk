---
title: ZMK Studio Configuration
sidebar_label: ZMK Studio
---

:::warning[Beta Feature]

ZMK Studio is in beta. Although every effort has been made to provide a stable experience, you may still encounter issues during use. Please report any issues to [GitHub Issues](https://github.com/zmkfirmware/zmk-studio/issues).

:::

The following settings affect the ZMK Studio portions of ZMK. See the [ZMK Studio feature](../features/studio.md) for more information on enabling and building with ZMK Studio enabled.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/src/studio/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/studio/Kconfig)

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

| Config                                         | Type | Description                                                                   | Default |
| ---------------------------------------------- | ---- | ----------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_STUDIO_TRANSPORT_BLE_PREF_LATENCY` | int  | Lower latency to request while ZMK Studio is active to improve responsiveness | 10      |
| `CONFIG_ZMK_STUDIO_RPC_THREAD_STACK_SIZE`      | int  | Stack size for the dedicated RPC thread                                       | 1800    |
| `CONFIG_ZMK_STUDIO_RPC_RX_BUF_SIZE`            | int  | Number of bytes available for buffering incoming messages                     | 30      |
| `CONFIG_ZMK_STUDIO_RPC_TX_BUF_SIZE`            | int  | Number of bytes available for buffering outgoing messages                     | 64      |
