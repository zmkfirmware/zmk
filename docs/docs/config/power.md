---
title: Power Management Configuration
sidebar_label: Power Management
---

See [Configuration Overview](index.md) for instructions on how to
change these settings.

## Low Power States

Configuration for entering [low power states](../features/low-power-states.md) when the keyboard is idle.

### Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                          | Type | Description                                                         | Default |
| ------------------------------- | ---- | ------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_IDLE_TIMEOUT`       | int  | Milliseconds of inactivity before entering idle state               | 30000   |
| `CONFIG_ZMK_SLEEP`              | bool | Enable deep sleep support                                           | n       |
| `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT` | int  | Milliseconds of inactivity before entering deep sleep               | 900000  |
| `CONFIG_ZMK_PM_SOFT_OFF`        | bool | Enable soft off functionality from the keymap or dedicated hardware | n       |

## External Power Control

Driver for enabling or disabling power to peripherals such as displays and lighting. This driver must be configured to use [power management behaviors](../keymaps/behaviors/power.md).

### Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                 | Type | Description                                     | Default |
| ---------------------- | ---- | ----------------------------------------------- | ------- |
| `CONFIG_ZMK_EXT_POWER` | bool | Enable support to control external power output | y       |

### Devicetree

Applies to: `compatible = "zmk,ext-power-generic"`

| Property        | Type       | Description                                                   |
| --------------- | ---------- | ------------------------------------------------------------- |
| `control-gpios` | GPIO array | List of GPIOs which should be active to enable external power |
| `init-delay-ms` | int        | number of milliseconds to delay after initializing the driver |

## GPIO Key Wakeup Trigger

A device similar to a [kscan](./kscan.md) which will be enabled only when the keyboard is entering [soft off](../features/low-power-states.md#soft-off) state. This is used to configure a GPIO key to wake the keyboard from [soft off](../features/low-power-states.md#soft-off) once it is pressed.

### Devicetree

Applies to: `compatible = "zmk,gpio-key-wakeup-trigger"`

Definition file: [zmk/app/dts/bindings/zmk,gpio-key-wakeup-trigger.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Cgpio-key-wakeup-trigger.yaml)

| Property        | Type       | Description                                                                                   |
| --------------- | ---------- | --------------------------------------------------------------------------------------------- |
| `trigger`       | phandle    | Phandle to a GPIO key to be used to wake from soft off                                        |
| `wakeup-source` | bool       | Mark this device as able to wake the keyboard                                                 |
| `extra-gpios`   | GPIO array | list of GPIO pins (including the appropriate flags) to set active before going into power off |

The `wakeup-source` property should always be present for this node to be useful. The `extra-gpios` property should be used to ensure the GPIO pin will trigger properly to wake the keyboard. For example, for a `col2row` matrix kscan, these are the column pins relevant for soft off.

## Soft Off Wakeup Sources

Selects a list of devices to enable during [soft off](../features/low-power-states.md#soft-off), allowing those with `wakeup-source` as a property to wake the keyboard.

### Devicetree

Applies to: `compatible = "zmk,soft-off-wakeup-sources"`

| Property         | Type          | Description                                                                                       |
| ---------------- | ------------- | ------------------------------------------------------------------------------------------------- |
| `wakeup-sources` | phandle array | List of devices to enable during the shutdown process to be sure they can later wake the keyboard |
