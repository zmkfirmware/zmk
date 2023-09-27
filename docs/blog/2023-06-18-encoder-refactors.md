---
title: "Major Encoder Refactor"
author: Pete Johanson
author_title: Project Creator
author_url: https://gitlab.com/petejohanson
author_image_url: https://www.gravatar.com/avatar/2001ceff7e9dc753cf96fcb2e6f41110
tags: [firmware, zephyr, sensors, encoders]
---

Today, we merged a significant change to the low level sensor code that is used to support encoders. In particular,
this paves the way for completing the work on supporting split peripheral sensors/encoders, and other future sensors
like pointing devices.

As part of the work, backwards compatibility for existing shields has been retained, but only for a grace period to allow out-of-tree shields to move to the new approach for encoders.

Special thanks to [joelspadin] for the _thorough_ code review and testing throughout the development of the refactor.

## Summary of Changes

The following items have been merged:

1. Split configuration of hardware details, and behavior configuration to allow more flexible functionality of sensors/encoders, in particular linear encoders that lack detents/"clicks" as they rotate.
2. Support for upstream Zephyr sensor drivers, including the NRFX QDEC driver that can be used on nRF52 based keyboards.
3. Sensor data handling changes that pave the way for split sensor handling easily.

## Configuration Changes

The major changes to configuration in the devicetree files relates to how the number of steps/triggers for a given encoder are set. In particular, the number of pulses/steps for a given encoder is configured first, allowing ZMK to determine the exact angular degrees of change that is represented by a single pulse on the data lines to that encoder.

Once that angular degrees mapping is completed, now independently there is a configuration setting to control how many triggers of the behavior in the keymap should occur for each full rotation of the sensor. Another way to think of this is "how many degrees of rotation results in a triggering of the sensor behavior in your keymap layer".

Splitting these two parts of the encoder configuration allows greater flexibility, and fine grained control of encoder behavior for linear encoders that don't have fixed detents.

### Old Configuration

Previously, an encoder configuration looked like:

```
    left_encoder: encoder_left {
        compatible = "alps,ec11";
        label = "LEFT_ENCODER";
        a-gpios = <&pro_micro 21 (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
        b-gpios = <&pro_micro 20 (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
        resolution = <4>;
    };
```

Here, the `resolution` property was used to indicate how many encoder pulses should trigger the sensor behavior one time. Next, the encoder is selected in the sensors node:

```
    sensors {
        compatible = "zmk,keymap-sensors";
        sensors = <&left_encoder &right_encoder>;
    };
```

That was the entirety of the configuration for encoders.

### New Configuration

```
    left_encoder: encoder_left {
        compatible = "alps,ec11";
        label = "LEFT_ENCODER";
        a-gpios = <&pro_micro 21 (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
        b-gpios = <&pro_micro 20 (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
        steps = <80>;
    };
```

Here, the `steps` property is now used to indicate how many encoder pulses there are in a single complete rotation of the encoder. Next, the encoder is selected in the sensors node as before, but an additional configuration is used to indicate how many times the encoder should trigger the behavior in your keymap per rotation:

```
    sensors {
        compatible = "zmk,keymap-sensors";
        sensors = <&left_encoder &right_encoder>;
        triggers-per-rotation = <20>;
    };
```

For tactile encoders that have detents, the `triggers-per-rotation` would match the number of detents on the encoder. For linear encoders, the value can be chosen to suit your needs.

## Zephyr Sensor Drivers

The configuration changes bring ZMK's code in line with how upstream Zephyr sensor drivers handle rotations. This has the added advantage of allowing us to leverage other sensor drivers. On Nordic MCUs, like nRF52840, the NRFX QDEC driver can be used, for example:

```
&pinctrl {
    qdec_default: qdec_default {
        group1 {
            psels = <NRF_PSEL(QDEC_A, 1, 11)>,
                    <NRF_PSEL(QDEC_B, 1, 10)>;
            bias-pull-up;
        };
    };
};

// Set up the QDEC hardware based driver and give it the same label as the deleted node.
encoder: &qdec0 {
    status = "okay";
    led-pre = <0>;
    steps = <80>;
    pinctrl-0 = <&qdec_default>;
    pinctrl-names = "default";
};
```

The NRFX QDEC driver has the advantage of supporting optical encoders as well, and although it polls, it does so in hardware without waking the MCU core; initial basic power profiling is promising.

## Split Sensor/Encoder Support

In addition to the refactors for splitting the configuration, the changes merged included refactors designed to simplify and move forward with the long outstanding feature of supporting encoders on the peripheral side of split keyboards. That work is planned as a follow up.

## Deprecation

The old configuration will be supported for a period of one month, and then removed, giving users a grace period to complete the migration to the new separated configuration.

[petejohanson]: https://github.com/petejohanson
[joelspadin]: https://github.com/joelspadin
