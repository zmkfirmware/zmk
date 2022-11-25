---
title: LED indicators
sidebar_label: LED indicators
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

ZMK supports the following LED indicators:

- Num Lock
- Caps Lock
- Scroll Lock
- Compose
- Kana

## Enabling LED indicators

To enable LED indicators on your board or shield, simply enable the `CONFIG_ZMK_LED_INDICATORS` configuration values in the `.conf` file of your user config directory as such:

```
CONFIG_ZMK_LED_INDICATORS=y
```

You can also configure the brightness of the LEDs using:

```
CONFIG_ZMK_LED_INDICATORS_BRT=80
```

If your board or shield does not have LED indicators configured, refer to [Adding LED indicators to a Board](#adding-led-indicators-to-a-board).

## Adding LED indicators to a board

You can use any LED driver supported by ZMK or Zephyr, it is recommended to use either `LED PWM` or `LED GPIO` driver.

<Tabs
defaultValue="pwm"
values={[
{label: 'LED PWM driver', value: 'pwm'},
{label: 'LED GPIO driver', value: 'gpio'},
{label: 'Others', value: 'other'},
]}>
<TabItem value="pwm">

First you have to enable the driver by adding the following lines to your `.conf` file:

```
CONFIG_PWM=y
CONFIG_LED_PWM=y
```

Next, you need to enable PWM by adding the following lines to your `.overlay` or `.dts` file:

```
&pwm0 {
	status = "okay";
	ch0-pin = <33>;
	/* ch0-inverted; */
	ch1-pin = <35>;
	/* ch1-inverted; */
};
```

You have to declare a channel for each LED. A single PWM module has a fixed number of channels depending on your SoC, if you have many LEDs you may want to use `&pwm1` or ` &pwm2` as well.

The value `chX-pin` represents the pin that controls the LEDs. With nRF52 boards, you can calculate the value to use in the following way: you need the hardware port and run it through a function.
**32 \* X + Y** = `<Pin number>` where X is first part of the hardware port "PX.01" and Y is the second part of the hardware port "P1.Y".

For example, _P1.13_ would give you _32 \* 1 + 13_ = `<45>` and _P0.15_ would give you _32 \* 0 + 15_ = `<15>`.

If you need to invert the signal, you may want to enable `chX-inverted`.

Then you have to add the following lines to your `.overlay` or `.dts` file inside the root devicetree node:

```
/ {
    led_indicators {
        compatible = "pwm-leds";
        label = "LED indicators";
        led_capslock: led_capslock {
            pwms = <&pwm0 33>;
            label = "Caps lock LED";
        };
        led_numlock: led_numlock {
            pwms = <&pwm0 35>;
            label = "Num lock LED";
        };
    };
};
```

The value inside `pwms` must be the same as you used before.

Finally you need to add each indicator to the `chosen` element of the root devicetree node:

```
chosen {
    ...
    zmk,led-capslock = &led_capslock;
    zmk,led-numlock = &led_numlock;
};
```

Currently, the supported indicators are:

- Num Lock: `zmk,led-numlock`
- Caps Lock: `zmk,led-capslock`
- Scroll Lock: `zmk,led-scrolllock`
- Compose: `zmk,led-compose`
- Kana: `zmk,led-kana`

</TabItem>
<TabItem value="gpio">

First you have to enable the driver by adding the following lines to your `.conf` file:

```
CONFIG_LED_GPIO=y
```

Next you have to add the following lines to your `.overlay` or `.dts` file inside the root devicetree node:

```
/ {
    led_indicators {
        compatible = "gpio-leds";
        label = "LED indicators";
        led_capslock: led_capslock {
            gpios = <&gpio1 1 GPIO_ACTIVE_HIGH>;
            label = "Caps lock LED";
        };
        led_numlock: led_numlock {
            gpios = <&gpio1 3 GPIO_ACTIVE_HIGH>;
            label = "Num lock LED";
        };
    };
};
```

Finally you need to add each indicator to the `chosen` element of the root devicetree node:

```
chosen {
    ...
    zmk,led-capslock = &led_capslock;
    zmk,led-numlock = &led_numlock;
};
```

Currently, the supported indicators are:

- Num Lock: `zmk,led-numlock`
- Caps Lock: `zmk,led-capslock`
- Scroll Lock: `zmk,led-scrolllock`
- Compose: `zmk,led-compose`
- Kana: `zmk,led-kana`

</TabItem>
<TabItem value="other">

If you have any other driver or you want to implement custom behaviors, such as a display widget, you can write a listener that implements the behavior you want, for example:

```c
#include <zmk/led_indicators.h>

static int led_indicators_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_led_indicator_changed(eh);
    zmk_led_indicators_flags_t leds = ev->leds;

    if (leds & ZMK_LED_CAPSLOCK_BIT) {
        // do something
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static ZMK_LISTENER(led_indicators_listener, led_indicators_listener);
static ZMK_SUBSCRIPTION(led_indicators_listener, zmk_led_indicator_changed);
```

</TabItem>
</Tabs>
