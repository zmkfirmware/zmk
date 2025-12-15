---
title: LED Indicators Configuration
sidebar_label: LED Indicators
---

See the [LED indicators feature page](../features/led-indicators.md) for more details.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition files:

- [zmk/app/src/indicators/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/indicators/Kconfig)

| Config                                    | Type | Description                                         | Default |
| ----------------------------------------- | ---- | --------------------------------------------------- | ------- |
| `CONFIG_ZMK_INDICATOR_LEDS_INIT_PRIORITY` | int  | Indicator LED device driver initialization priority | 91      |

`CONFIG_ZMK_INDICATOR_LEDS_INIT_PRIORITY` must be set to a larger value than `CONFIG_LED_INIT_PRIORITY`.

## Indicator LED Driver

This driver maps HID indicator states to [LED API](https://docs.zephyrproject.org/4.1.0/hardware/peripherals/led.html) devices.

### Devicetree

Applies to: `compatible = "zmk,indicator-leds"`

Definition file: [zmk/app/dts/bindings/indicators/zmk,indicator-leds.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/indicators/zmk%2Cindicator-leds.yaml)

| Property                  | Type     | Description                                                           | Default |
| ------------------------- | -------- | --------------------------------------------------------------------- | ------- |
| `indicator`               | int      | The `HID_INDICATOR_*` value to indicate                               |         |
| `leds`                    | phandles | One or more LED devices to control                                    |         |
| `active-brightness`       | int      | LED brightness in percent when the indicator is active                | 100     |
| `inactive-brightness`     | int      | LED brightness in percent when the indicator is not active            | 0       |
| `disconnected-brightness` | int      | LED brightness in percent when the keyboard is not connected          | 0       |
| `on-while-idle`           | bool     | Keep LEDs enabled even when the keyboard is idle and on battery power | false   |

The `indicator` property must be one of the `HID_INDICATOR_*` values defined in [zmk/app/include/dt-bindings/zmk/hid_indicators.h](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/hid_indicators.h). You may also combine values with `|` to make the LED only be lit when multiple indicator states are active at the same time.
