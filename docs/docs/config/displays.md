---
title: Display Configuration
sidebar_label: Displays
---

See the [displays feature page](../features/displays.md) for more details.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition files:

- [zmk/app/src/display/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/display/Kconfig)
- [zmk/app/src/display/widgets/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/display/widgets/Kconfig)

| Config                                             | Type   | Description                                                                               | Default |
| -------------------------------------------------- | ------ | ----------------------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_DISPLAY`                               | bool   | Enable support for displays                                                               | n       |
| `CONFIG_ZMK_DISPLAY_INVERT`                        | bool   | Invert display colors from black-on-white to white-on-black                               | n       |
| `CONFIG_ZMK_WIDGET_LAYER_STATUS`                   | bool   | Enable a widget to show the highest, active layer                                         | y       |
| `CONFIG_ZMK_WIDGET_BATTERY_STATUS`                 | bool   | Enable a widget to show battery charge information                                        | y       |
| `CONFIG_ZMK_WIDGET_BATTERY_STATUS_SHOW_PERCENTAGE` | bool   | If battery widget is enabled, show percentage instead of icons                            | n       |
| `CONFIG_ZMK_WIDGET_OUTPUT_STATUS`                  | bool   | Enable a widget to show the current output (USB/BLE)                                      | y       |
| `CONFIG_ZMK_WIDGET_WPM_STATUS`                     | bool   | Enable a widget to show words per minute                                                  | n       |
| `CONFIG_ZMK_WIDGET_MODS_STATUS`                    | bool   | Enable a widget to show active modifiers                                                  | n       |
| `CONFIG_ZMK_WIDGET_MODS_STATUS_CHARACTERS`         | string | Characters to show for each modifier, corresponding to Control/Shift/Alt/GUI respectively | "CSAG"  |

Note that WPM and modifiers widgets are both shown on the bottom right of the display and hence can conflict with each other.

Note that `CONFIG_ZMK_DISPLAY_INVERT` setting might not work as expected with custom status screens that utilize images.

If `CONFIG_ZMK_DISPLAY` is enabled, exactly zero or one of the following options must be set to `y`. The first option is used if none are set.

| Config                                      | Description                    |
| ------------------------------------------- | ------------------------------ |
| `CONFIG_ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN` | Use the built-in status screen |
| `CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM`   | Use a custom status screen     |

If `CONFIG_ZMK_DISPLAY` is enabled, exactly zero or one of the following options must be set to `y`. The first option is used if none are set.

| Config                                    | Description                               |
| ----------------------------------------- | ----------------------------------------- |
| `CONFIG_ZMK_DISPLAY_WORK_QUEUE_SYSTEM`    | Use the system main thread for UI updates |
| `CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED` | Use a dedicated thread for UI updates     |

Using a dedicated thread requires more memory but prevents displays with slow updates (e.g. E-paper) from delaying key scanning and other processes. If enabled, the following options configure the thread:

| Config                                           | Type | Description                  | Default |
| ------------------------------------------------ | ---- | ---------------------------- | ------- |
| `CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE` | int  | Stack size for the UI thread | 2048    |
| `CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_PRIORITY`   | int  | Priority for the UI thread   | 5       |

You must also configure the driver for your display. ZMK provides the following display drivers:

- [IL0323](https://github.com/zmkfirmware/zmk/blob/main/app/module/drivers/display/Kconfig.il0323)

Zephyr provides several display drivers as well. Search for the name of your display in [Zephyr's Kconfig options](https://docs.zephyrproject.org/3.5.0/kconfig.html) documentation.

## Devicetree

See the Devicetree bindings for your display. Here are the bindings for common displays:

- [IL0323](https://github.com/zmkfirmware/zmk/blob/main/app/module/dts/bindings/display/gooddisplay%2Cil0323.yaml)
- [SSD1306 (i2c)](https://docs.zephyrproject.org/3.5.0/build/dts/api/bindings/display/solomon,ssd1306fb-i2c.html)
- [SSD1306 (spi)](https://docs.zephyrproject.org/3.5.0/build/dts/api/bindings/display/solomon,ssd1306fb-spi.html)

A full list of drivers provided by Zephyr can be found in [Zephyr's Devicetree bindings index](https://docs.zephyrproject.org/3.5.0/build/dts/api/bindings.html).
