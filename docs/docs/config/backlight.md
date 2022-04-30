---
title: Backlight Configuration
sidebar_label: Backlight
---

See the [backlight feature page](/docs/features/backlight) for more details, including instructions for adding backlight support to a board.

See [Configuration Overview](/docs/config) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Option                               | Type | Description                                           | Default |
| ------------------------------------ | ---- | ----------------------------------------------------- | ------- |
| `CONFIG_ZMK_BACKLIGHT`               | bool | Enables LED backlight                                 | n       |
| `CONFIG_ZMK_BACKLIGHT_BRT_STEP`      | int  | Brightness step in percent                            | 20      |
| `CONFIG_ZMK_BACKLIGHT_BRT_START`     | int  | Default brightness in percent                         | 40      |
| `CONFIG_ZMK_BACKLIGHT_ON_START`      | bool | Default backlight state                               | y       |
| `CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE` | bool | Turn off backlight when keyboard goes into idle state | n       |
| `CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB`  | bool | Turn off backlight when USB is disconnected           | n       |

## Devicetree

Applies to: [`/chosen` node](https://docs.zephyrproject.org/latest/build/dts/intro.html#aliases-and-chosen-nodes)

| Property        | Type | Description                                  |
| --------------- | ---- | -------------------------------------------- |
| `zmk,backlight` | path | The node for the backlight LED driver to use |

See the Zephyr devicetree bindings for LED drivers:

- [gpio-leds](https://docs.zephyrproject.org/latest/build/dts/api/bindings/gpio/gpio-leds.html)
- [pwm-leds](https://docs.zephyrproject.org/latest/build/dts/api/bindings/led/pwm-leds.html)

See the [backlight feature page](/docs/features/backlight) for examples of the properties that must be set to enable backlighting.
