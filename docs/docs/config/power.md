---
title: Power Management Configuration
sidebar_label: Power Management
---

See [Configuration Overview](index.md) for instructions on how to
change these settings.

## Low Power States

Configuration for entering [low power states](../features/low-power-states.mdx) when the keyboard is idle.

### Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                          | Type | Description                                                         | Default |
| ------------------------------- | ---- | ------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_IDLE_TIMEOUT`       | int  | Milliseconds of inactivity before entering idle state               | 30000   |
| `CONFIG_ZMK_SLEEP`              | bool | Enable deep sleep support                                           | n       |
| `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT` | int  | Milliseconds of inactivity before entering deep sleep               | 900000  |
| `CONFIG_ZMK_PM_SOFT_OFF`        | bool | Enable soft off functionality from the keymap or dedicated hardware | n       |

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
