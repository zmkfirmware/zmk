---
title: Power Management Configuration
sidebar_label: Power Management
---

See [Configuration Overview](index.md) for instructions on how to
change these settings.

## Idle/Sleep

Configuration for entering low power modes when the keyboard is idle.

In the idle state, peripherals such as displays and lighting are disabled, but the keyboard remains connected to Bluetooth so it can immediately respond when you press a key.

In the deep sleep state, the keyboard additionally disconnects from Bluetooth and any external power output is disabled. This state uses very little power, but it may take a few seconds to reconnect after waking.

### Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                          | Type | Description                                           | Default |
| ------------------------------- | ---- | ----------------------------------------------------- | ------- |
| `CONFIG_ZMK_IDLE_TIMEOUT`       | int  | Milliseconds of inactivity before entering idle state | 30000   |
| `CONFIG_ZMK_SLEEP`              | bool | Enable deep sleep support                             | n       |
| `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT` | int  | Milliseconds of inactivity before entering deep sleep | 900000  |

## External Power Control

Driver for enabling or disabling power to peripherals such as displays and lighting. This driver must be configured to use [power management behaviors](../behaviors/power.md).

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
