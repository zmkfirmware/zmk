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

If your keyboard supports this feature, ZMK will display some or all of these states on LEDs.

The default behavior for an indicator LED is as follows:

- If the keyboard is on battery power and idle, the LED is off.
- If the keyboard is not connected to any host, the LED is off.
- If the indicator state is active, the LED is on at full brightness.
- Otherwise, the LED is off.

If you want to change these behaviors, you can set properties on the devicetree nodes for each indicator in your `.keymap` file. The conventional node labels for indicators are as follows:

- `num_lock_indicator`
- `caps_lock_indicator`
- `scroll_lock_indicator`
- `compose_indicator`
- `kana_indicator`

The examples below all use `caps_lock_indicator`. To edit a different indicator, use the relevant label for that indicator instead.

## Idle Behavior

The `on-while-idle` property prevents the LED from turning off when the keyboard on battery power and idle:

```dts
&caps_lock_indicator {
    on-while-idle;
};
```

## LED Brightness

The `active-brightness`, `inactive-brightness`, and `disconnected-brightness` properties control the brightness of the LED when the indicator is active, indicator is not active, and the keyboard is not connected to any host, respectively.

For example, if you want the LED to be off when the indicator is active, 100% brightness when inactive, and 50% brightness when not connected:

```dts
&caps_lock_indicator {
    active-brightness = <0>;
    inactive-brightness = <100>;
    disconnected-brightness = <50>;
};
```

:::note

If the LED is not configured to support brightness control, any value greater than 0 will result in maximum brightness.

For most LEDs, you can enable PWM brightness control, though this will increase power usage slightly. See the [LED indicators hardware integration page](../development/hardware-integration/lighting/led-indicators.md) for details on configuring the LEDs.

:::

## Adding LED Indicator Support to a Keyboard

See the [LED indicators hardware integration page](../development/hardware-integration/lighting/led-indicators.md) for instructions to enable this feature on a keyboard.
