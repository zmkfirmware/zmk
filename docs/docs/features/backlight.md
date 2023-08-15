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
defaultValue="shieldnopin"
values={[
{label: 'Adding to a board without using Pinctrl', value: 'boardnopin'},{label: 'Adding to a board using Pinctrl', value: 'boardpin'},
{label: 'Adding to a shield without using Pinctrl', value: 'shieldnopin'},{label: 'Adding to a shield using Pinctrl', value: 'shieldpin'},
]}>
<TabItem value="boardnopin">

First, you must enable PWM by adding the following lines to your `Kconfig.defconfig` file:

```kconfig
if ZMK_BACKLIGHT

config PWM
    default y

config LED_PWM
    default y

endif # ZMK_BACKLIGHT
```

Since the Zephyr 3.2 update the PWM driver has been revised, permitting the use of the pinctrl API to select the pins. For now the old method of pin select is also functional

If you do not want to use pinctrl add the following lines to your `.dts` file:

```dts
&pwm0 {
    status = "okay";
    ch0-pin = <45>;
};
```

The value `ch0-pin` represents the pin that controls the LEDs. With nRF52 boards, you can calculate the value to use in the following way: you need the hardware port and run it through a function.
**32 \* X + Y** = `<Pin number>` where X is first part of the hardware port "PX.01" and Y is the second part of the hardware port "P1.Y".

For example, _P1.13_ would give you _32 \* 1 + 13_ = `<45>` and _P0.15_ would give you _32 \* 0 + 15_ = `<15>`.

Then you have to add the following lines inside the root devicetree node on the same file as before:

```dts
/ {
    backlight: pwmleds {
        compatible = "pwm-leds";
        label = "Backlight LEDs";
        pwm_led_0 {
            pwms = <&pwm0 45 10000 PWM_POLARITY_NORMAL>;
            label = "Backlight LED 0";
        };
    };
};
```

The value inside `pwm_led_0` must be the same as you used before.

In this example `10000` is the period of the PWM waveform, some drive circuitry might require different values, it could also be altered in the event the drive frequency is audible/

If your board uses a P-channel MOSFET to control backlight instead of a N-channel MOSFET, you may want to change `PWM_POLARITY_NORMAL` for `PWM_POLARITY_INVERTED`.

:::info
Note that every LED inside of the backlight node will be treated as a backlight LED, so if you have other PWM LEDs you need to declare them in a separate node. Refer to [Multiple backlight LEDs](#multiple-backlight-leds) if you have multiple backlight LEDs.
:::

Finally you need to add backlight to the `chosen` element of the root devicetree node:

```dts
/ {
    chosen {
        zmk,backlight = &backlight;
    };
};
```

</TabItem>
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

When using the pinctrl API you should create a pinctrl.dtsi file if it's not created already, and include it at the beginning of the `board.dts` file. You also need to add `CONFIG_PINCTRL=y` to board_defconfig if it's not already enabled

A pinctrl file has an `&pinctrl` node that encompasses all pinctrl settings including for I2C or SPI peripherals (e.g. WS2812 LEDs, Battery fuel gauges)

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
            psels = <NRF_PSEL(PWM_OUT0, 0, 45)>;
            low-power-enable;
        };
    };
}
```

The pin number is calculated in the same way as for the non pinctrl configuration.

The value `45` in the example represents the pin that controls the LEDs. With nRF52 boards, you can calculate the value to use in the following way: you need the hardware port and run it through a function.
**32 \* X + Y** = `<Pin number>` where X is first part of the hardware port "PX.01" and Y is the second part of the hardware port "P1.Y".

For example, _P1.13_ would give you _32 \* 1 + 13_ = `<PWM_OUT0, 0, 45>` and _P0.15_ would give you _32 \* 0 + 15_ = `<PWM_OUT0, 0, 15>`.

Then you add the PWM device inside the `board.dts` file and assign the pinctrl definitions to it

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
            pwms = <&pwm0 0 10000 PWM_POLARITY_NORMAL>;
        };
    };
};
```

In this example `10000` is the period of the PWM waveform, some drive circuitry might require different values, it could also be altered in the event the drive frequency is audible/

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
<TabItem value="shieldnopin">

Since the Zephyr 3.2 update the PWM driver has been revised, permitting the use of the pinctrl API to select the pins. For now the old method of pin select is also functional

You must first add a `boards/` directory within your shield folder. For each board that supports the shield you must create a `<board>.defconfig` file and a `<board>.overlay` file inside the `boards/` folder.

Inside your `<board>.defconfig` file, add the following lines:

```kconfig
if ZMK_BACKLIGHT

config PWM
    default y

config LED_PWM
    default y

endif # ZMK_BACKLIGHT
```

Then add the following lines to your `.overlay` file:

```dts
&pwm0 {
    status = "okay";
    ch0-pin = <45>;
};
```

The value `ch0-pin` represents the pin that controls the LEDs. With nRF52 boards, you can calculate the value to use in the following way: you need the hardware port and run it through a function.
**32 \* X + Y** = `<Pin number>` where X is first part of the hardware port "PX.01" and Y is the second part of the hardware port "P1.Y".

For example, _P1.13_ would give you _32 \* 1 + 13_ = `<45>` and _P0.15_ would give you _32 \* 0 + 15_ = `<15>`.

Then you have to add the following lines inside the root devicetree node on the same file:

```dts
/ {
    backlight: pwmleds {
        compatible = "pwm-leds";
        label = "Backlight LEDs";
        pwm_led_0 {
            pwms = <&pwm0 45 10000 PWM_POLARITY_NORMAL>;
            label = "Backlight LED 0";
        };
    };
};
```

The value inside `pwm_led_0` must be the same as you used before.

In this example `10000` is the period of the PWM waveform, some drive circuitry might require different values, it could also be altered in the event the drive frequency is audible.

If your board uses a P-channel MOSFET to control backlight instead of a N-channel MOSFET, you may want to change `PWM_POLARITY_NORMAL` for `PWM_POLARITY_INVERTED`.

:::info
Note that every LED inside of the backlight node will be treated as a backlight LED, so if you have other PWM LEDs you need to declare them in a separate node. Refer to [Multiple backlight LEDs](#multiple-backlight-leds) if you have multiple backlight LEDs.
:::

Finally you need to add backlight to the `chosen` element of the root devicetree node:

```dts
/ {
    chosen {
        zmk,backlight = &backlight;
    };
}:
```

Optionally, on Pro Micro compatible shields you can add a LED GPIO node to your devicetree, this could be useful if you want your shield to be compatible with newer or untested boards. To do that you have to enable `CONFIG_LED_GPIO` in your `.conf` file and then add the following lines inside the root devicetree node of your `.dtsi` or `.dts` file:

```dts
/ {
    backlight: gpioleds {
        compatible = "gpio-leds";
        label = "Backlight LEDs";
        gpio_led_0 {
            gpios = <&pro_micro 20 GPIO_ACTIVE_HIGH>;
            label = "Backlight LED 0";
        };
    };
};
```

If no suitable `<board>.overlay` file is found, this node will act as a fallback, however, without PWM, backlight has limited functionality.

</TabItem>
<TabItem value="shieldpin">

You must first add a `boards/` directory within your shield folder. For each board that supports the shield you must create a `<board>.defconfig` file and a `<board>.overlay` file inside the `boards/` folder.

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
            psels = <NRF_PSEL(PWM_OUT0, 0, 45)>;
        };
    };
    pwm0_sleep: pwm0_sleep {
        group1 {
            psels = <NRF_PSEL(PWM_OUT0, 0, 45)>;
            low-power-enable;
        };
    };
}
```

The pin number is calculated in the same way as for the non pinctrl configuration.

The value `45` in the example represents the pin that controls the LEDs. With nRF52 boards, you can calculate the value to use in the following way: you need the hardware port and run it through a function.
**32 \* X + Y** = `<Pin number>` where X is first part of the hardware port "PX.01" and Y is the second part of the hardware port "P1.Y".

For example, _P1.13_ would give you _32 \* 1 + 13_ = `<PWM_OUT0, 0, 45>` and _P0.15_ would give you _32 \* 0 + 15_ = `<PWM_OUT0, 0, 15>`.

Then you add the PWM device inside the `.overlay` file and assign the pinctrl definitions to it

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
            pwms = <&pwm0 0 10000 PWM_POLARITY_NORMAL>;
        };
    };
};
```

In this example `10000` is the period of the PWM waveform, some drive circuitry might require different values, it could also be altered in the event the drive frequency is audible/

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
</Tabs>

### Multiple backlight LEDs

It is possible to control multiple backlight LEDs at the same time. This is useful if, for example, you have a Caps Lock LED connected to a different pin and you want it to be part of the backlight.

In order to do that, first you need to enable PWM for each pin:

```dts
&pwm0 {
    status = "okay";
    ch0-pin = <45>; /* LED 0 */
    ch1-pin = <46>; /* LED 1 */
    ch2-pin = <47>; /* LED 2 */
    ...
};
```

This part may vary based on your MCU as different MCUs may have a different number of modules and channels.

Then you can simply add each of your LED to the backlight node:

```dts
backlight: pwmleds {
    compatible = "pwm-leds";
    label = "Backlight LEDs";
    pwm_led_0 {
        pwms = <&pwm0 45>; /* LED 0 */
    };
    pwm_led_1 {
        pwms = <&pwm0 46>; /* LED 1 */
    };
    pwm_led_2 {
        pwms = <&pwm0 47>; /* LED 2 */
    };
    ...
};
```
