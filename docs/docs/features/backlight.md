---
title: Backlight
sidebar_label: Backlight
---

Backlight is a feature used to control array of LEDs, usually placed through or under switches. Unlike [RGB Underglow](underglow.md), backlight currently allows only one color per LED, also LEDs are not addressable, so you can't control individual LEDs.

## Enabling Backlight

To enable backlight on your board or shield, simply enable the `CONFIG_ZMK_BACKLIGHT` configuration values in the `.conf` file of your user config directory as such:

```
CONFIG_ZMK_BACKLIGHT=y
```

If your board or shield does not have backlight configured, refer to [Adding Backlight to a Board](#adding-backlight-to-a-board).

## Configuring Backlight

There are various Kconfig options used to configure the backlight feature. These can all be set in the `.conf` file.

| Option                               | Description                                           | Default |
| ------------------------------------ | ----------------------------------------------------- | ------- |
| `CONFIG_ZMK_BACKLIGHT_BRT_STEP`      | Brightness step in percent                            | 20      |
| `CONFIG_ZMK_BACKLIGHT_BRT_START`     | Default brightness in percent                         | 40      |
| `CONFIG_ZMK_BACKLIGHT_ON_START`      | Default backlight state                               | y       |
| `CONFIG_ZMK_BACKLIGHT_AUTO_OFF_IDLE` | Turn off backlight when keyboard goes into idle state | y       |
| `CONFIG_ZMK_BACKLIGHT_AUTO_OFF_USB`  | Turn off backlight when USB is disconnected           | n       |

## Adding Backlight to a Board

Backlight is always added to a board, not a shield.
If you have a shield with backlight, you must add a `boards/` directory within your shield folder to define the backlight individually for each board that supports the shield.
Inside the `boards/` folder, you define a `<board>.overlay` for each different board.

First, you need to enable PWM by adding the following lines to your `.overlay` file:

```
&pwm0 {
	status = "okay";
	ch0-pin = <45>;
	/* ch0-inverted; */
};
```

The value `ch0-pin` represents the pin that controls the LEDs. To calculate the value to use, you need a bit of math. You need the hardware port and run it through a function.
**32 \* X + Y** = `<Pin number>` where X is first part of the hardware port "PX.01" and Y is the second part of the hardware port "P1.Y".

For example, _P1.13_ would give you _32 \* 1 + 13_ = `<45>` and _P0.15_ would give you _32 \* 0 + 15_ = `<15>`.

If your board uses a P-channel MOSFET to control backlight instead of a N-channel MOSFET, you may want to enable `ch0-inverted`.

Then you have to add the following lines to your `.dtsi` file inside the root devicetree node:

```
backlight: pwmleds {
    compatible = "pwm-leds";
    label = "Backlight LEDs";
    pwm_led_0 {
        pwms = <&pwm0 45>;
    };
};
```

The value inside `pwm_led_0` must be the same as you used before.

:::info
Note that every LED inside of the backlight node will be treated as a backlight LED, so if you have other PWM LEDs you need to declare them in a separate node. Refer to [Multiple backlight LEDs](#multiple-backlight-leds) if you have multiple backlight LEDs.
:::

Finally you need to add backlight to the `chosen` element of the root devicetree node:

```
chosen {
    ...
    zmk,backlight = &backlight;
};
```

### Multiple backlight LEDs

It is possible to control multiple backlight LEDs at the same time. This is useful if, for example, you have a Caps Lock LED connected to a different pin and you want it to be part of the backlight.

In order to do that, first you need to enable PWM for each pin:

```
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

```
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
