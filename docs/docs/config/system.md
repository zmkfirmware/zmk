---
title: System Configuration
sidebar_label: System
---

These are general settings that control how the keyboard behaves and which features it supports. Several of these settings come from Zephyr and are not specific to ZMK, but they are listed here because they are relevant to how a keyboard functions.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

### General

| Config                      | Type   | Description                                  | Default |
| --------------------------- | ------ | -------------------------------------------- | ------- |
| `CONFIG_ZMK_KEYBOARD_NAME`  | string | The name of the keyboard (max 16 characters) |         |
| `CONFIG_ZMK_WPM`            | bool   | Enable calculating words per minute          | n       |
| `CONFIG_HEAP_MEM_POOL_SIZE` | int    | Size of the heap memory pool                 | 8192    |

:::info

Because ZMK enables [the Zephyr setting](https://docs.zephyrproject.org/3.5.0/kconfig.html#CONFIG_BT_DEVICE_NAME_DYNAMIC) that allows for runtime modification of the device BT name,
changing `CONFIG_ZMK_KEYBOARD_NAME` requires [clearing the stored settings](./settings.md#clearing-persisted-settings) on the controller in order to take effect.

:::

### HID

:::warning[Refreshing the HID descriptor]

Making changes to any of the settings in this section modifies the HID report descriptor and requires it to be [refreshed](../features/bluetooth.md#refreshing-the-hid-descriptor).

:::

| Config                                       | Type | Description                                                      | Default |
| -------------------------------------------- | ---- | ---------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_HID_INDICATORS`                  | bool | Enable receipt of HID/LED indicator state from connected hosts   | n       |
| `CONFIG_ZMK_HID_CONSUMER_REPORT_SIZE`        | int  | Number of consumer keys simultaneously reportable                | 6       |
| `CONFIG_ZMK_HID_SEPARATE_MOD_RELEASE_REPORT` | bool | Send modifier release event **after** non-modifier release event | n       |

Exactly zero or one of the following options may be set to `y`. The first is used if none are set.

| Config                            | Description                                                                                           |
| --------------------------------- | ----------------------------------------------------------------------------------------------------- |
| `CONFIG_ZMK_HID_REPORT_TYPE_HKRO` | Enable `CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE` key roll over.                                           |
| `CONFIG_ZMK_HID_REPORT_TYPE_NKRO` | Enable full N-key roll over. This may prevent the keyboard from working with some BIOS/UEFI versions. |

:::note[NKRO usages]

By default the NKRO max usage is set so as to maximize compatibility, however certain less frequently used keys (F13-F24 and INTL1-8) will not work with it. One solution is to set `CONFIG_ZMK_HID_KEYBOARD_NKRO_EXTENDED_REPORT=y`, however this is known to break compatibility with Android and thus not enabled by default.

:::

If `CONFIG_ZMK_HID_REPORT_TYPE_HKRO` is enabled, it may be configured with the following options:

| Config                                | Type | Description                                       | Default |
| ------------------------------------- | ---- | ------------------------------------------------- | ------- |
| `CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE` | int  | Number of keyboard keys simultaneously reportable | 6       |

If `CONFIG_ZMK_HID_REPORT_TYPE_NKRO` is enabled, it may be configured with the following options:

| Config                                         | Type | Description                                                          | Default |
| ---------------------------------------------- | ---- | -------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_HID_KEYBOARD_NKRO_EXTENDED_REPORT` | bool | Enable less frequently used key usages, at the cost of compatibility | n       |

Exactly zero or one of the following options may be set to `y`. The first is used if none are set.

| Config                                        | Description                                                                          |
| --------------------------------------------- | ------------------------------------------------------------------------------------ |
| `CONFIG_ZMK_HID_CONSUMER_REPORT_USAGES_FULL`  | Enable all consumer key codes, but may have compatibility issues with some host OSes |
| `CONFIG_ZMK_HID_CONSUMER_REPORT_USAGES_BASIC` | Prevents using some consumer key codes, but allows compatibility with more host OSes |

### USB

| Config                            | Type   | Description                             | Default         |
| --------------------------------- | ------ | --------------------------------------- | --------------- |
| `CONFIG_USB`                      | bool   | Enable USB drivers                      |                 |
| `CONFIG_USB_DEVICE_VID`           | int    | The vendor ID advertised to USB         | `0x1D50`        |
| `CONFIG_USB_DEVICE_PID`           | int    | The product ID advertised to USB        | `0x615E`        |
| `CONFIG_USB_DEVICE_MANUFACTURER`  | string | The manufacturer name advertised to USB | `"ZMK Project"` |
| `CONFIG_USB_HID_POLL_INTERVAL_MS` | int    | USB polling interval in milliseconds    | 1               |
| `CONFIG_ZMK_USB`                  | bool   | Enable ZMK as a USB keyboard            |                 |
| `CONFIG_ZMK_USB_BOOT`             | bool   | Enable USB Boot protocol support        | n               |
| `CONFIG_ZMK_USB_INIT_PRIORITY`    | int    | USB init priority                       | 50              |

:::note[USB Boot protocol support]

By default USB Boot protocol support is disabled, however certain situations such as the input of Bitlocker pins or FileVault passwords may require it to be enabled.

:::

### Bluetooth

See [Zephyr's Bluetooth stack architecture documentation](https://docs.zephyrproject.org/3.5.0/connectivity/bluetooth/bluetooth-arch.html)
for more information on configuring Bluetooth.

| Config                                      | Type | Description                                                           | Default |
| ------------------------------------------- | ---- | --------------------------------------------------------------------- | ------- |
| `CONFIG_BT`                                 | bool | Enable Bluetooth support                                              |         |
| `CONFIG_BT_BAS`                             | bool | Enable the Bluetooth BAS (battery reporting service)                  | y       |
| `CONFIG_BT_MAX_CONN`                        | int  | Maximum number of simultaneous Bluetooth connections                  | 5       |
| `CONFIG_BT_MAX_PAIRED`                      | int  | Maximum number of paired Bluetooth devices                            | 5       |
| `CONFIG_ZMK_BLE`                            | bool | Enable ZMK as a Bluetooth keyboard                                    |         |
| `CONFIG_ZMK_BLE_CLEAR_BONDS_ON_START`       | bool | Clears all bond information from the keyboard on startup              | n       |
| `CONFIG_ZMK_BLE_CONSUMER_REPORT_QUEUE_SIZE` | int  | Max number of consumer HID reports to queue for sending over BLE      | 5       |
| `CONFIG_ZMK_BLE_KEYBOARD_REPORT_QUEUE_SIZE` | int  | Max number of keyboard HID reports to queue for sending over BLE      | 20      |
| `CONFIG_ZMK_BLE_INIT_PRIORITY`              | int  | BLE init priority                                                     | 50      |
| `CONFIG_ZMK_BLE_THREAD_PRIORITY`            | int  | Priority of the BLE notify thread                                     | 5       |
| `CONFIG_ZMK_BLE_THREAD_STACK_SIZE`          | int  | Stack size of the BLE notify thread                                   | 768     |
| `CONFIG_ZMK_BLE_PASSKEY_ENTRY`              | bool | Experimental: require typing passkey from host to pair BLE connection | n       |

Note that `CONFIG_BT_MAX_CONN` and `CONFIG_BT_MAX_PAIRED` should be set to the same value. On a split keyboard they should only be set for the central and must be set to one greater than the desired number of bluetooth profiles.

### Logging

| Config                   | Type | Description                              | Default |
| ------------------------ | ---- | ---------------------------------------- | ------- |
| `CONFIG_ZMK_USB_LOGGING` | bool | Enable USB CDC ACM logging for debugging | n       |
| `CONFIG_ZMK_LOG_LEVEL`   | int  | Log level for ZMK debug messages         | 4       |

## Snippets

:::danger
Using these snippets can erase the SoftDevice on your board.
Erasing the SoftDevice will prevent the board from using firmware built without these snippets.

Flashing such firmware **will** totally brick the board, disabling the USB flashing functionality.
The only way to restore functionality after that is to re-flash the bootloader.

Re-flashing a bootloader built without the SoftDevice will require firmware built with these snippets.
:::

[Snippets](https://docs.zephyrproject.org/3.5.0/build/snippets/index.html) are a way to save common configuration separately when it applies to multiple different applications.

Enable snippets by adding `snippet: <snippet>` to your `build.yaml` for the appropriate board:

```yaml
- board: nrfmicro_13_52833
  snippet: nrf52833-nosd
  shield: corne_left
```

For local builds, add `-S <snippet>` to your build command. For example:

```sh
west build -b nrfmicro_13_52833 -S nrf52833-nosd -- -DSHIELD=corne_left
```

ZMK implements the following system configuration snippets:

### nrf52833-nosd

Definition: [zmk/app/snippets/nrf52833-nosd](https://github.com/zmkfirmware/zmk/blob/main/app/snippets/nrf52833-nosd)

On memory-constrained nRF52833 boards this snippet will extend the code partition to overwrite the Nordic SoftDevice.
This gives 428KB for the code partition as opposed to 280KB with the Nordic SoftDevice.

The added memory allows the nRF52833 to fit displays and other memory-intensive features.

### nrf52840-nosd

Definition: [zmk/app/snippets/nrf52840-nosd](https://github.com/zmkfirmware/zmk/blob/main/app/snippets/nrf52840-nosd)

On nRF52840 boards this snippet will overwrite the Nordic SoftDevice, extending both the code and storage partitions.
This gives 844KB/128KB for the code/storage partitions as opposed to 792KB/32KB with the Nordic SoftDevice.

Firmware built with this snippet can work on boards after accidentally erasing the SoftDevice.
It can also be useful for especially memory-intensive applications.
