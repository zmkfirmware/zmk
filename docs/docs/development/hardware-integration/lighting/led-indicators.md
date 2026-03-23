---
title: LED Indicators
sidebar_label: LED Indicators
description: Lighting system that indicates HID states such as caps lock.
---

ZMK supports the following five LED indicator states from the HID specification:

- Num Lock
- Caps Lock
- Scroll Lock
- Compose
- Kana

To connect these indicator states to LEDs on a keyboard, you must do two things in your board or shield files:

1. Define an LED driver and one or more LEDs to control.
2. Configure ZMK to control those LEDs.

## LED Definitions

The most common setup is for LEDs to be driven directly from a GPIO pin. The examples on this page show how to controls these using the `gpio-leds` or `pwm-leds` drivers, but you can also use any other [LED driver](https://docs.zephyrproject.org/4.1.0/hardware/peripherals/led.html) from Zephyr.

:::warning
LED strip drivers (e.g. WS2812 LEDs) are not currently supported. More work is needed to support the separate API and avoid conflicts with the underglow system.
:::

In your shield's `.overlay` file or board's `.dts` file, create a `gpio-leds` node and define any LEDs that you want to be able to control. The following example defines two LEDs on `&gpio0 1` and `&gpio0 2` which are enabled by driving the pins high:

```dts
/ {
    leds {
        compatible = "gpio-leds";

        num_lock_led: num_lock_led {
            gpios = <&gpio0 1 GPIO_ACTIVE_HIGH>;
        };

        caps_lock_led: caps_lock_led {
            gpios = <&gpio0 2 GPIO_ACTIVE_HIGH>;
        };
    };
};
```

### PWM Brightness Control

The above example only supports LEDs being off or on at full brightness. If you want to be able to reduce the brightness or use multiple brightness levels, you must use `pwm-leds` instead of `gpio-leds`. Note that this will increase power usage slightly when LEDs are enabled compared to using `gpio-leds`.

See the [backlight hardware integration page](backlight.mdx) for an example of configuring PWM LEDs.

## Indicator Definitions

Now that you have some LEDs defined, you can configure ZMK to use them.

In your shield's `.overlay` file or board's `.dts` file, add the following include to the top of the file:

```dts
#include <dt-bindings/zmk/hid_indicators.h>
```

Then, add a `zmk,indicator-leds` node. This node can have any number of child nodes. Each child maps an indicator state to one or more LEDs:

```dts
/ {
    leds {
        compatible = "gpio-leds";

        // LEDs as defined in the previous step
        num_lock_led: num_lock_led { ... };
        caps_lock_led: caps_lock_led { ... };
    };

    indicators {
        compatible = "zmk,indicator-leds";

        num_lock_indicator: num_lock {
            indicator = <HID_INDICATOR_NUM_LOCK>;
            leds = <&num_lock_led>;
        };

        caps_lock_indicator: caps_lock {
            indicator = <HID_INDICATOR_CAPS_LOCK>;
            leds = <&caps_lock_led>;
        };
    };
};
```

The name of each child node is unimportant, but you should give each node a label so users can change their settings in `.keymap` files. Conventionally, you should use one of the following:

- `num_lock_indicator`
- `caps_lock_indicator`
- `scroll_lock_indicator`
- `compose_indicator`
- `kana_indicator`

(If you have a non-standard setup and need to use different labels, you should document this somewhere so users know how to configure them.)

Each child node must have an `indicator` property, which is set to one of the following:

- `HID_INDICATOR_NUM_LOCK`
- `HID_INDICATOR_CAPS_LOCK`
- `HID_INDICATOR_SCROLL_LOCK`
- `HID_INDICATOR_COMPOSE`
- `HID_INDICATOR_KANA`

Each child node must also have an `leds` property, which holds a list of LED nodes to control. In this example, the LEDs are labeled `num_lock_led` and `caps_lock_led`, so the `leds` properties refer to them with `&num_lock_led` and `&caps_lock_led`, respectively.

You can also control multiple LEDs from the same indicator:

```dts
    leds = <&led_1 &led_2>;
```

## LED Behavior

See the [feature page](../../../features/led-indicators.md) and [configuration page](../../../config/led-indicators.md) for details on configuring LED brightness according to the indicator state.
