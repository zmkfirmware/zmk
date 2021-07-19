---
title: System Configuration
sidebar_label: System
---

These are general settings that control how the keyboard behaves and which
features it supports. Several of these settings come from Zephyr and are not
specific to ZMK, but they are listed here because they are relevant to how a
keyboard functions.

See [Configuration Overview](/docs/config/index) for instructions on how to
change these settings.

## Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

### General

| Config                              | Type   | Description                                                                   | Default |
| ----------------------------------- | ------ | ----------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_KEYBOARD_NAME`          | string | The name of the keyboard                                                      |         |
| `CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE` | int    | Milliseconds to wait after a setting change before writing it to flash memory | 60000   |
| `CONFIG_ZMK_WPM`                    | bool   | Enable calculating words per minute                                           | n       |
| `CONFIG_HEAP_MEM_POOL_SIZE`         | int    | Size of the heap memory pool                                                  | 8192    |

### USB

| Config                            | Type   | Description                             | Default         |
| --------------------------------- | ------ | --------------------------------------- | --------------- |
| `CONFIG_USB`                      | bool   | Enable USB drivers                      |                 |
| `CONFIG_USB_DEVICE_VID`           | int    | The vendor ID advertised to USB         | `0x1D50`        |
| `CONFIG_USB_DEVICE_PID`           | int    | The product ID advertised to USB        | `0x615E`        |
| `CONFIG_USB_DEVICE_MANUFACTURER`  | string | The manufacturer name advertised to USB | `"ZMK Project"` |
| `CONFIG_USB_HID_POLL_INTERVAL_MS` | int    | USB polling interval in milliseconds    | 9               |
| `CONFIG_ZMK_USB`                  | bool   | Enable ZMK as a USB keyboard            |                 |
| `CONFIG_ZMK_USB_INIT_PRIORITY`    | int    | USB init priority                       | 50              |

### Bluetooth

See [Zephyr's Bluetooth stack architecture documentation](https://docs.zephyrproject.org/latest/guides/bluetooth/bluetooth-arch.html)
for more information on configuring Bluetooth.

| Config                                                | Type | Description                                                            | Default |
| ----------------------------------------------------- | ---- | ---------------------------------------------------------------------- | ------- |
| `CONFIG_BT`                                           | bool | Enable Bluetooth support                                               |         |
| `CONFIG_BT_MAX_CONN`                                  | int  | Maximum number of simultaneous Bluetooth connections                   | 5       |
| `CONFIG_BT_MAX_PAIRED`                                | int  | Maximum number of paired Bluetooth devices                             | 5       |
| `CONFIG_ZMK_BLE`                                      | bool | Enable ZMK as a Bluetooth keyboard                                     |         |
| `CONFIG_ZMK_BLE_INIT_PRIORITY`                        | int  | BLE init priority                                                      | 50      |
| `CONFIG_ZMK_BLE_THREAD_STACK_SIZE`                    | int  | Stack size of the BLE notify thread                                    | 512     |
| `CONFIG_ZMK_BLE_THREAD_PRIORITY`                      | int  | Priority of the BLE notify thread                                      | 5       |
| `CONFIG_ZMK_BLE_KEYBOARD_REPORT_QUEUE_SIZE`           | int  | Max number of keyboard HID reports to queue for sending over BLE       | 20      |
| `CONFIG_ZMK_BLE_CONSUMER__REPORT_QUEUE_SIZE`          | int  | Max number of consumer HID reports to queue for sending over BLE       | 5       |
| `CONFIG_ZMK_BLE_CLEAR_BONDS_ON_START`                 | bool | Clears all bond information from the keyboard on startup               | n       |
| `CONFIG_ZMK_BLE_PASSKEY_ENTRY`                        | bool | Experimental: require typing passkey from host to pair BLE connection  | n       |
| `CONFIG_ZMK_SPLIT`                                    | bool | Enable split keyboard support                                          | n       |
| `CONFIG_ZMK_SPLIT_BLE`                                | bool | Use BLE to communicate between split keyboard halves                   | y       |
| `CONFIG_ZMK_SPLIT_BLE_ROLE_CENTRAL`                   | bool | `y` for central device, `n` for peripheral                             |         |
| `CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE`    | int  | Max number of key state events to queue when received from peripherals | 5       |
| `CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_STACK_SIZE`          | int  | Stack size of the split peripheral BLE notify thread                   | 512     |
| `CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_PRIORITY`            | int  | Priority of the split peripheral BLE notify thread                     | 5       |
| `CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_POSITION_QUEUE_SIZE` | int  | Max number of key state events to queue to send to the central         | 10      |

### Logging

| Config                   | Type | Description                              | Default |
| ------------------------ | ---- | ---------------------------------------- | ------- |
| `CONFIG_ZMK_USB_LOGGING` | bool | Enable USB CDC ACM logging for debugging | n       |
| `CONFIG_ZMK_LOG_LEVEL`   | int  | Log level for ZMK debug messages         | 4       |
