---
title: Split Configuration
sidebar_label: Split
---

These are settings that control how split keyboards behave.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Following [split keyboard](../features/split-keyboards.md) settings are defined in [zmk/app/src/split/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/split/Kconfig).

| Config                                       | Type | Description                                                              | Default |
| -------------------------------------------- | ---- | ------------------------------------------------------------------------ | ------- |
| `CONFIG_ZMK_SPLIT`                           | bool | Enable split keyboard support                                            | n       |
| `CONFIG_ZMK_SPLIT_ROLE_CENTRAL`              | bool | `y` for central device, `n` for peripheral                               | n       |
| `CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS` | bool | Enable split keyboard support for passing indicator state to peripherals | n       |

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

Wired splits require a properly configured UART to function. If writing a shield, you may be able to use the standard UART already provided by the board, e.g. `&pro_micro_serial`. See [predefined nodes](../development/hardware-integration/pinctrl.mdx#predefined-nodes) for details on the UART node labels provided by various interconnects. If you are creating your own board, or using custom pins for the UART, see the documentation on [pin control](../development/hardware-integration/pinctrl.mdx#additional-examples) to configure the pins for your UART.

Once you have a properly configured UART device, it needs to be assigned in a new node with a compatible value of `"zmk,wired-split"`. For example:

```dts
/ {
    wired_split {
        compatible = "zmk,wired-split";
        device = <&pro_micro_serial>;
    };
};
```
