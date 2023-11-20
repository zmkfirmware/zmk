---
title: Backlight
sidebar_label: Backlight
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

Backlight is a feature used to control an array of LEDs, usually placed through or under switches.

:::info
Unlike [RGB Underglow](underglow.md), backlight can only control single color LEDs. Additionally, because backlight LEDs all receive the same power, it's not possible to dim individual LEDs.
:::

## Enabling Backlight

To enable backlight on your board or shield, add the following line to your `.conf` file of your user config directory as such:

```ini
CONFIG_ZMK_BACKLIGHT=y
```

If your board or shield does not have backlight configured, refer to [Adding Backlight to a board or a shield](#adding-backlight-to-a-board-or-a-shield).

## Configuring Backlight

There are various Kconfig options used to configure the backlight feature. These can all be set in the `.conf` file.

| Option                               | Description                                           | Default |
| ------------------------------------ | ----------------------------------------------------- | ------- |
| `CONFIG_ZMK_BACKLIGHT_BRT_STEP`      | Brightness step in percent                            | 20      |
| `CONFIG_ZMK_BACKLIGHT_BRT_START`     | Default brightness in percent                         | 40      |
| `CONFIG_ZMK_BACKLIGHT_ON_START`      | Default backlight state                               | y       |
| `CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE` | Turn off backlight when keyboard goes into idle state | n       |
| `CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB`  | Turn off backlight when USB is disconnected           | n       |

## Adding Backlight to a board or a shield

<Tabs
defaultValue="boardpin"
values={[
{label: 'Adding to a board', value: 'boardpin'},{label: 'Adding to a shield', value: 'shieldpin'},
]}>

<TabItem value="boardpin">

First, you must enable PWM by adding the following lines to your `Kconfig.defconfig` file:

```
if ZMK_BACKLIGHT

config PWM
    default y

config LED_PWM
    default y

endif # ZMK_BACKLIGHT
```

When using the pinctrl API you should create a pinctrl.dtsi file if it's not created already, and include it at the beginning of the `board.dts` file. You also need to add `CONFIG_PINCTRL=y` to board_defconfig if it's not already enabled. To use some of the definitions for the PWM node you need to include `#include <dt-bindings/led/led.h>` at the beginning of the `board.dts` file.

A pinctrl file has an `&pinctrl` node that encompasses all pinctrl settings including for I2C or SPI peripherals (e.g. WS2812 LEDs, Battery fuel gauges):

```
&pinctrl {
    // Other pinctrl definitions for other hardware
    pwm0_default: pwm0_default {
        group1 {
            psels = <NRF_PSEL(PWM_OUT0, 1, 13)>;
        };
    };
    pwm0_sleep: pwm0_sleep {
        group1 {
            psels = <NRF_PSEL(PWM_OUT0, 1, 13)>;
            low-power-enable;
        };
    };
};
```

The pin number is handled differently depending on the MCU in question, on nRF boards it is defined as (PWM_OUTX, Y, Z) where X is the PWM channel used (usually 0) Y is first part of the hardware port "PY.01" and Z is the second part of the hardware port "P1.Z".

For example, _P1.13_ would give you `(PWM_OUT0, 1, 13)` and _P0.15_ would give you `(PWM_OUT0, 0, 15)`.

Then you add the PWM device inside the `board.dts` file and assign the pinctrl definitions to it:

```
&pwm0 {
    status = "okay";
    pinctrl-0 = <&pwm0_default>;
    pinctrl-1 = <&pwm0_sleep>;
    pinctrl-names = "default", "sleep";
};
```

Then you have to add the following lines inside the root devicetree node on the same file as before:

```
/ {
    backlight: pwmleds {
        compatible = "pwm-leds";
        pwm_led_0 {
            pwms = <&pwm0 0 PWM_MSEC(10) PWM_POLARITY_NORMAL>;
        };
    };
};
```

The value inside `pwm_led_0` after `&pwm0` must be the channel number. Since (`PWM_OUT0`) is defined in the pinctrl node the channel in this example is 0.

In this example `PWM_MSEC(10)` is the period of the PWM waveform, some drive circuitry might require different values, it could also be altered in the event the drive frequency is audible.

If your board uses a P-channel MOSFET to control backlight instead of a N-channel MOSFET, you may want to change `PWM_POLARITY_NORMAL` for `PWM_POLARITY_INVERTED`.

Finally you need to add backlight to the `chosen` element of the root devicetree node:

```
/ {
    chosen {
        zmk,backlight = &backlight;
    };
};
```

</TabItem>
<TabItem value="shieldpin">

You must first add a `boards/` directory within your shield folder. For each board that supports the shield you must create a `<board>.defconfig` file and a `<board>.overlay` file inside the `boards/` folder. To use some of the definitions for the PWM node you need to include `#include <dt-bindings/led/led.h>` at the beginning of the `<board>.overlay` file.

Inside your `<board>.defconfig` file, add the following lines:

```
if ZMK_BACKLIGHT

config PWM
    default y

config LED_PWM
    default y

endif # ZMK_BACKLIGHT
```

Then add the following lines to your `.overlay` file:

```
&pinctrl {
    // Other pinctrl definitions for other hardware
    pwm0_default: pwm0_default {
        group1 {
            psels = <NRF_PSEL(PWM_OUT0, 1, 13)>;
        };
    };
    pwm0_sleep: pwm0_sleep {
        group1 {
            psels = <NRF_PSEL(PWM_OUT0, 1, 13)>;
            low-power-enable;
        };
    };
};
```

The pin number is handled differently depending on the MCU in question, on nRF boards it is defined as (PWM_OUTX, Y, Z) where X is the PWM channel used (usually 0) Y is first part of the hardware port "PY.01" and Z is the second part of the hardware port "P1.Z".

For example, _P1.13_ would give you `(PWM_OUT0, 1, 13)` and _P0.15_ would give you `(PWM_OUT0, 0, 15)`.

Then you add the PWM device inside the `.overlay` file and assign the pinctrl definitions to it:

```
&pwm0 {
    status = "okay";
    pinctrl-0 = <&pwm0_default>;
    pinctrl-1 = <&pwm0_sleep>;
    pinctrl-names = "default", "sleep";
};
```

Then you have to add the following lines inside the root devicetree node on the same file as before:

```
/ {
    backlight: pwmleds {
        compatible = "pwm-leds";
        pwm_led_0 {
            pwms = <&pwm0 0 PWM_MSEC(10) PWM_POLARITY_NORMAL>;
        };
    };
};
```

In this example `PWM_MSEC(10)` is the period of the PWM waveform, some drive circuitry might require different values, it could also be altered in the event the drive frequency is audible.

If your board uses a P-channel MOSFET to control backlight instead of a N-channel MOSFET, you may want to change `PWM_POLARITY_NORMAL` for `PWM_POLARITY_INVERTED`.

The value inside `pwm_led_0` after `&pwm0` must be the channel number. Since (`PWM_OUT0`) is defined in the pinctrl node the channel in this example is 0.

Finally you need to add backlight to the `chosen` element of the root devicetree node:

```
/ {
    chosen {
        zmk,backlight = &backlight;
    };
};
```

</TabItem>
</Tabs>

### Multiple backlight LEDs

It is possible to control multiple backlight LEDs at the same time. This is useful if, for example, you have a Caps Lock LED connected to a different pin and you want it to be part of the backlight.

In order to do that, first you need to enable a PWM for each pin in the pinctrl node:

```
&pinctrl {
    // Other Pinctrl definitions go here
    pwm0_default: pwm0_default {
        group1 {
            psels = <NRF_PSEL(PWM_OUT0, 0, 20)>, // LED 0
                    <NRF_PSEL(PWM_OUT1, 0, 22)>, // LED 1
                    <NRF_PSEL(PWM_OUT2, 0, 24)>; // LED 2
        };
    };
    pwm0_sleep: pwm0_sleep {
        group1 {
            psels = <NRF_PSEL(PWM_OUT0, 0, 20)>, // LED 0
                    <NRF_PSEL(PWM_OUT1, 0, 22)>, // LED 1
                    <NRF_PSEL(PWM_OUT2, 0, 24)>; // LED 2
            low-power-enable;
        };
    };
};
```

This part will vary based on your MCU as different MCUs have a different number of modules, channels and configuration options.

Then you can simply add each of your LED to the backlight node in the same manner as for one LED, using the channel number definitions in the pinctrl node:

```dts
backlight: pwmleds {
    compatible = "pwm-leds";
    label = "Backlight LEDs";
    pwm_led_0: pwm_led_0 {
        pwms = <&pwm0 0 PWM_MSEC(10) PWM_POLARITY_NORMAL>;
    };
    pwm_led_1: pwm_led_1 {
        pwms = <&pwm0 1 PWM_MSEC(10) PWM_POLARITY_NORMAL>;
    };
    pwm_led_2: pwm_led_2 {
        pwms = <&pwm0 2 PWM_MSEC(10) PWM_POLARITY_NORMAL>;
    };
};
```
