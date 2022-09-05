---
title: RGB Underglow Configuration
sidebar_label: RGB Underglow
---

See the [RGB Underglow feature page](../features/underglow.md) for more details, including instructions for adding underglow support to a board.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

RGB underglow depends on [Zephyr's LED strip driver](https://github.com/zephyrproject-rtos/zephyr/tree/main/drivers/led_strip), which provides additional Kconfig options.

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                                   | Type | Description                                               | Default |
| ---------------------------------------- | ---- | --------------------------------------------------------- | ------- |
| `CONFIG_ZMK_RGB_UNDERGLOW`               | bool | Enable RGB underglow                                      | n       |
| `CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER`     | bool | Underglow toggling also controls external power           | y       |
| `CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE` | bool | Turn off RGB underglow when keyboard goes into idle state | n       |
| `CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_USB`  | bool | Turn off RGB underglow when USB is disconnected           | n       |
| `CONFIG_ZMK_RGB_UNDERGLOW_HUE_STEP`      | int  | Hue step in degrees (0-359) used by RGB actions           | 10      |
| `CONFIG_ZMK_RGB_UNDERGLOW_SAT_STEP`      | int  | Saturation step in percent used by RGB actions            | 10      |
| `CONFIG_ZMK_RGB_UNDERGLOW_BRT_STEP`      | int  | Brightness step in percent used by RGB actions            | 10      |
| `CONFIG_ZMK_RGB_UNDERGLOW_HUE_START`     | int  | Default hue in degrees (0-359)                            | 0       |
| `CONFIG_ZMK_RGB_UNDERGLOW_SAT_START`     | int  | Default saturation percent (0-100)                        | 100     |
| `CONFIG_ZMK_RGB_UNDERGLOW_BRT_START`     | int  | Default brightness in percent (0-100)                     | 100     |
| `CONFIG_ZMK_RGB_UNDERGLOW_SPD_START`     | int  | Default effect speed (1-5)                                | 3       |
| `CONFIG_ZMK_RGB_UNDERGLOW_EFF_START`     | int  | Default effect index from the effect list (see below)     | 0       |
| `CONFIG_ZMK_RGB_UNDERGLOW_ON_START`      | bool | Default on state                                          | y       |

Values for `CONFIG_ZMK_RGB_UNDERGLOW_EFF_START`:

| Value | Effect      |
| ----- | ----------- |
| 0     | Solid color |
| 1     | Breathe     |
| 2     | Spectrum    |
| 3     | Swirl       |

:::note
The `*_START` settings only determine the initial underglow state. Any changes you make with the [underglow behavior](../behaviors/underglow.md) are saved to flash after a one minute delay and will be used after that.
:::

## Devicetree

ZMK does not have any Devicetree properties of its own. See the Devicetree bindings for [Zephyr's LED strip drivers](https://github.com/zephyrproject-rtos/zephyr/tree/main/dts/bindings/led_strip).

See the [RGB underglow feature page](../features/underglow.md) for examples of the properties that must be set to enable underglow.
