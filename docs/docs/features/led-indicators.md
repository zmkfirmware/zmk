---
title: LED Indicators
sidebar_label: LED Indicators
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

See the [backlight hardware integration page](../development/hardware-integration/lighting/backlight.mdx) for an example of configuring PWM LEDs.

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

Each child node must have an `indicator` property, which is set to one of the following:

- `HID_INDICATOR_NUM_LOCK`
- `HID_INDICATOR_CAPS_LOCK`
- `HID_INDICATOR_SCROLL_LOCK`
- `HID_INDICATOR_COMPOSE`
- `HID_INDICATOR_KANA`

Each child node must also have an `leds` property, which holds a list of LED nodes to control. In this example, the LEDs are labeled `num_lock_led` and `caps_lock_led`, so the `leds` properties refer to them with `&num_lock_led` and `&caps_lock_led`, respectively.

Adding the `num_lock_indicator` and `caps_lock_indicator` labels is not necessary, but this will allow users of your keyboard to more easily adjust the indicator behaviors in their `.keymap` files, e.g.:

```dts
&num_lock_indicator {
    on-while-idle;
};
```

If you want an LED to only be lit when multiple indicator states are active at the same time, you can use `|` to combine multiple values:

```dts
    indicator = <(HID_INDICATOR_NUM_LOCK | HID_INDICATOR_CAPS_LOCK)>;
```

You can also control multiple LEDs from the same indicator:

```dts
    leds = <&led_1 &led_2>;
```

### LED Behavior

The default behavior for an indicator LED is as follows:

- If the keyboard is idle and on battery power, the LED is off.
- If the keyboard is not connected to any host, the LED is off.
- If the indicator state is active, the LED is on at full brightness.
- Otherwise, the LED is off.

You can add some properties to each indicator node to change these behaviors:

The `on-while-idle` property prevents the LED from turning off when the keyboard is idle and on battery power:

```dts
    caps_lock_indicator: caps_lock {
        ...
        on-while-idle;
    };
```

The `active-brightness`, `inactive-brightness`, and `disconnected-brightness` properties control the brightness of the LED when the indicator is active, indicator is not active, and the keyboard is not connected to any host, respectively.

For example, if you want the LED to be off when the indicator is active, 100% brightness when inactive, and 50% brightness when not connected:

```dts
    caps_lock_indicator: caps_lock {
        ...
        active-brightness = <0>;
        inactive-brightness = <100>;
        disconnected-brightness = <50>;
    };
```

:::note

When using the `gpio-leds` driver, any brighness >= 1 will be treated as 100%. Use [PWM control](#pwm-brightness-control) if you want brightnesses in between 0% and 100%.

:::
