---
title: Split Configuration
sidebar_label: Split
---

These are settings that control how split keyboards behave.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Following [split keyboard](../features/split-keyboards.md) settings are defined in [zmk/app/src/split/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/split/Kconfig).

| Config                          | Type | Description                                | Default |
| ------------------------------- | ---- | ------------------------------------------ | ------- |
| `CONFIG_ZMK_SPLIT`              | bool | Enable split keyboard support              | n       |
| `CONFIG_ZMK_SPLIT_ROLE_CENTRAL` | bool | `y` for central device, `n` for peripheral | n       |

### Central State on Peripherals

Some state only exists on the central, e.g. the active layers. The following settings send it to the peripheral(s), where it is available through the same functions and events as on the central, except for the modifiers and the central battery, so that the built-in display widgets and custom ones work there as well.

| Config                                                       | Type | Description                                                                                            | Default |
| ------------------------------------------------------------ | ---- | ------------------------------------------------------------------------------------------------------ | ------- |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_ENDPOINT`                       | bool | Mirror the selected and the preferred endpoint                                                         | n       |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_BATTERY`                | bool | Expose the central battery level and whether the central is powered over USB                           | n       |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS`                 | bool | Mirror the HID indicators (e.g. Caps Lock) of the active endpoint, selects `CONFIG_ZMK_HID_INDICATORS` | n       |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_MODIFIERS`                      | bool | Mirror the explicitly pressed modifiers                                                                | n       |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_WPM`                            | bool | Mirror the words per minute, selects `CONFIG_ZMK_WPM`                                                  | n       |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_BLE_PROFILE`                    | bool | Mirror the active BLE profile index and whether it is connected and bonded                             | n       |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_LAYER_STATE`                    | bool | Mirror the active layers and the name of the highest active layer                                      | n       |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_STATE_SYNC_COALESCE_MS` | int  | Time to collect central state changes before sending them to the peripherals                           | 50      |

Enable the same settings on the central and on the peripherals. A peripheral ignores state it was not built for and logs a warning.

State is only sent when it changes. Changes within `CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_STATE_SYNC_COALESCE_MS` are sent together, with only the latest value of each kind, and the full state is sent again whenever a peripheral connects. On wired splits, the first state can be lost when both halves power up together and the peripheral's UART is not ready yet, and splits without `detect-gpios` cannot tell when a peripheral restarts. In both cases the central still considers its last values sent, so each value only reaches the peripheral the next time it changes. For example, a peripheral that restarted shows the endpoint as not connected, which keeps indicator LEDs off, until the central's endpoint changes.

When a Bluetooth peripheral loses the connection to the central, its central state returns to the state after boot, as if nothing had been received, and the events are raised for the values that change. The central battery level becomes unknown again without an event, so `zmk_split_peripheral_get_central_battery_level` returns `-ENODATA`. Wired peripherals cannot detect a lost central and keep the last received state. In builds with both transports, switching from Bluetooth to wired disconnects Bluetooth, which also resets the peripheral's state; if that reset happens after the central's resync over wired, the values stay at their boot state until they change.

The following functions and events are available on the peripheral:

| Config                                        | Functions                                                                                                                      | Events                                                         |
| --------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ | -------------------------------------------------------------- |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_ENDPOINT`        | `zmk_endpoint_get_selected`, `zmk_endpoint_get_preferred`, `zmk_endpoint_get_preferred_transport`, `zmk_endpoint_is_connected` | `zmk_endpoint_changed`                                         |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_BATTERY` | `zmk_split_peripheral_get_central_battery_level`                                                                               | `zmk_central_battery_state_changed`                            |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS`  | `zmk_hid_indicators_get_current_profile`                                                                                       | `zmk_hid_indicators_changed`                                   |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_MODIFIERS`       | `zmk_hid_get_explicit_mods`                                                                                                    | `zmk_modifiers_state_changed`                                  |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_WPM`             | `zmk_wpm_get_state`                                                                                                            | `zmk_wpm_state_changed`                                        |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_BLE_PROFILE`     | `zmk_ble_active_profile_index`, `zmk_ble_active_profile_is_connected`, `zmk_ble_active_profile_is_open`                        | `zmk_ble_active_profile_changed`, with `profile` set to `NULL` |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_LAYER_STATE`     | `zmk_keymap_highest_layer_active`, `zmk_keymap_layer_active`, `zmk_keymap_layer_index_to_id`, `zmk_keymap_layer_name`          | `zmk_layer_state_changed`                                      |

With `CONFIG_ZMK_SPLIT_PERIPHERAL_MODIFIERS`, key presses that do not change the modifiers send nothing. The central never raises `zmk_modifiers_state_changed`: widgets there listen to `zmk_keycode_state_changed` and read `zmk_hid_get_explicit_mods`, while on peripherals the sync raises `zmk_modifiers_state_changed` for the pressed and the released modifiers. `CONFIG_ZMK_SPLIT_PERIPHERAL_WPM` sends at most one update per second while typing, when the value changes.

With `CONFIG_ZMK_SPLIT_PERIPHERAL_ENDPOINT`, the peripheral starts as not connected until the central reports its endpoint, and a Bluetooth peripheral returns to not connected while it has lost the central. Changing only the preferred transport, while the selected endpoint stays the same, raises no event on the central, so the peripheral sees it on the next endpoint or profile change. Functions that depend on the central's own endpoint configuration, such as `zmk_endpoint_instance_to_index` and `ZMK_ENDPOINT_COUNT`, the helpers `zmk_endpoint_instance_eq` and `zmk_endpoint_instance_to_str`, and those that change the endpoint or send reports are not available on peripherals.

Only the name of the highest active layer is sent, truncated to 17 bytes without splitting a UTF-8 character. `zmk_keymap_layer_index_to_id` only maps the highest active layer; the others are assumed to be in keymap order. A layer renamed at runtime, e.g. with ZMK Studio, gets its new name on peripherals with the next layer change.

### Bluetooth Splits

Following bluetooth [split keyboard](../features/split-keyboards.md) settings are defined in [zmk/app/src/split/bluetooth/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/split/bluetooth/Kconfig).

| Config                                                  | Type | Description                                                                | Default                                    |
| ------------------------------------------------------- | ---- | -------------------------------------------------------------------------- | ------------------------------------------ |
| `CONFIG_ZMK_SPLIT_BLE`                                  | bool | Use BLE to communicate between split keyboard halves                       | y                                          |
| `CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS`              | int  | Number of peripherals that will connect to the central                     | 1                                          |
| `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING`   | bool | Enable fetching split peripheral battery levels to the central side        | n                                          |
| `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_PROXY`      | bool | Enable central reporting of split battery levels to hosts                  | n                                          |
| `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_QUEUE_SIZE` | int  | Max number of battery level events to queue when received from peripherals | `CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS` |
| `CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE`      | int  | Max number of key state events to queue when received from peripherals     | 5                                          |
| `CONFIG_ZMK_SPLIT_BLE_CENTRAL_SPLIT_RUN_STACK_SIZE`     | int  | Stack size of the BLE split central write thread                           | 512                                        |
| `CONFIG_ZMK_SPLIT_BLE_CENTRAL_SPLIT_RUN_QUEUE_SIZE`     | int  | Max number of behavior run events to queue to send to the peripheral(s)    | 5                                          |
| `CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_STACK_SIZE`            | int  | Stack size of the BLE split peripheral notify thread                       | 756                                        |
| `CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_PRIORITY`              | int  | Priority of the BLE split peripheral notify thread                         | 5                                          |
| `CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_POSITION_QUEUE_SIZE`   | int  | Max number of key state events to queue to send to the central             | 10                                         |

#### Input Split

The following setting only applies when using BLE split together with [input split devices](pointing.md#input-split), such as for pointing devices attached to split peripherals:

| Config                                  | Type | Description                                                     | Default |
| --------------------------------------- | ---- | --------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_INPUT_SPLIT_MSG_QUEUE_SIZE` | int  | Max number of input split events to queue for BLE notifications | 32      |

### Wired Splits

Hardware UARTs have a few different modes/approaches to sending and receiving data, with different levels of complexity and performance. Not all hardware nor drivers support all modes, so ZMK has code to support different interaction modes with the UART as needed. The default mode should be properly selected based on the platform's report support, but you can choose to override the mode if needed.

- Polling Mode - The least efficient mode, this requires the MCU to constantly poll the UART to see if more data has been received, taking time away from other processing. This is basic mode supported by all UART drivers.
- Interrupt Mode - This mode allows the MCU to do other processing until the UART raises an interrupt to signal new data has been received. On platforms where this is combined with a FIFO, there is even less superfluous processing, and high speeds can be achieved while allowing other processing to continue. Examples:
  - RP2040
  - nRF52
- Async (DMA) Mode - Similar to interrupt mode, data reception can occur without involving the MCU. Additionally, larger volumes can be copied directly into accessible memory without the use of the MCU, allowing even further efficiency/rates without tying up the MCU. Only some drivers support this mode (and the current Zephyr 3.5 version of the nRF52 UART has some bugs that prevent its use). Examples:
  - SAM0 (e.g. SAMD21)
  - STM32 (e.g. stm32f072)

Following wired [split keyboard](../features/split-keyboards.md) settings are defined in [zmk/app/src/split/wired/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/split/wired/Kconfig).

| Config                                       | Type | Description                                                       | Default                                                       |
| -------------------------------------------- | ---- | ----------------------------------------------------------------- | ------------------------------------------------------------- |
| `CONFIG_ZMK_SPLIT_WIRED`                     | bool | Use wired connection to communicate between split keyboard halves | y (if devicetree is set appropriately)                        |
| `CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC`     | bool | Async (DMA) mode                                                  | y if the driver supports it (excluding nRF52 with known bugs) |
| `CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT` | bool | Interrupt mode                                                    | y if the hardware supports it                                 |
| `CONFIG_ZMK_SPLIT_WIRED_UART_MODE_POLLING`   | bool | Polling mode                                                      | y if neither other mode is supported                          |

#### Async (DMA) Mode

The following settings only apply when using wired split in async (DMA) mode:

| Config                                    | Type | Description                                                 | Default |
| ----------------------------------------- | ---- | ----------------------------------------------------------- | ------- |
| `CONFIG_ZMK_SPLIT_WIRED_ASYNC_RX_TIMEOUT` | int  | RX Timeout (in microseconds) before reporting received data | 20      |

#### Polling Mode

The following settings only apply when using wired split in polling mode:

| Config                                     | Type | Description                                          | Default |
| ------------------------------------------ | ---- | ---------------------------------------------------- | ------- |
| `CONFIG_ZMK_SPLIT_WIRED_POLLING_RX_PERIOD` | int  | Number of ticks between calls to poll for split data | 10      |

## Devicetree

### Wired Split

Wired splits require a properly configured UART to function. If writing a shield, you may be able to use the standard UART already provided by the board, e.g. `&pro_micro_serial`. See [predefined nodes](../hardware-integration/pinctrl.mdx#predefined-nodes) for details on the UART node labels provided by various interconnects. If you are creating your own board, or using custom pins for the UART, see the documentation on [pin control](../hardware-integration/pinctrl.mdx#additional-examples) to configure the pins for your UART.

Once you have a properly configured UART device, it needs to be assigned in a new node with a compatible value of `"zmk,wired-split"`. For example:

```dts
/ {
    wired_split {
        compatible = "zmk,wired-split";
        device = <&pro_micro_serial>;
    };
};
```
