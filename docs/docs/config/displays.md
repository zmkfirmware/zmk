---
title: Display Configuration
sidebar_label: Displays
---

See the [displays feature page](/docs/features/displays) for more details.

See [Configuration Overview](/docs/config/index) for instructions on how to
change these settings.

## Kconfig

Definition files:

- [zmk/app/src/display/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/display/Kconfig)
- [zmk/app/src/display/widgets/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/display/widgets/Kconfig)

| Config                             | Type | Description                                          | Default |
| ---------------------------------- | ---- | ---------------------------------------------------- | ------- |
| `CONFIG_ZMK_DISPLAY`               | bool | Enable support for displays                          | n       |
| `CONFIG_ZMK_WIDGET_LAYER_STATUS`   | bool | Enable a widget to show the highest, active layer    | y       |
| `CONFIG_ZMK_WIDGET_BATTERY_STATUS` | bool | Enable a widget to show battery charge information   | y       |
| `CONFIG_ZMK_WIDGET_OUTPUT_STATUS`  | bool | Enable a widget to show the current output (USB/BLE) | y       |
| `CONFIG_ZMK_WIDGET_WPM_STATUS`     | bool | Enable a widget to show words per minute             | n       |

If `CONFIG_ZMK_DISPLAY` is enabled, exactly one of the following options must be set to `y`:

| Config                                      | Type | Description                    | Default |
| ------------------------------------------- | ---- | ------------------------------ | ------- |
| `CONFIG_ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN` | bool | Use the built-in status screen | y       |
| `CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM`   | bool | Use a custom status screen     | n       |

You must also configure the Zephyr driver for your display. Here are the Kconfig options for common displays.

- [SSD1306](https://docs.zephyrproject.org/latest/reference/kconfig/CONFIG_SSD1306.html)

## Devicetree

See the Zephyr Devicetree bindings for your display. Here are the bindings for common displays:

- [SSD1306 (i2c)](https://docs.zephyrproject.org/latest/reference/devicetree/bindings/solomon,ssd1306fb-i2c.html)
- [SSD1306 (spi)](https://docs.zephyrproject.org/latest/reference/devicetree/bindings/solomon,ssd1306fb-spi.html)

A full list of supported drivers can be found in [Zephyr's Devicetree bindings index](https://docs.zephyrproject.org/latest/reference/devicetree/bindings.html).
