---
title: Soft Off Feature
sidebar_label: Soft Off
---

Similar to the deep sleep feature that sends the keyboard into a low power state after a certain period of inactivity, the soft off feature is used to turn the keyboard on and off explicitly. Depending on the keyboard, this may be through a dedicated on/off push button, or merely through an additional binding in the keymap to turn the device off and the existing reset button to turn the device back on.

The feature is intended as an alternative to using a hardware switch to physically cut power from the battery to the keyboard. This can be useful for existing PCBs not designed for wireless that don't have a power switch, or for new designs that favor a push button on/off like found on other devices.

:::note

The power off is accomplished by putting the MCU into a "soft off" state. Power is _not_ technically removed from the entire system, but the device will only be woken from the state by a few possible events.

:::

Once powered off, the keyboard will only wake up when:

- You press the same button/sequence that you pressed to power off the keyboard, or
- You press a reset button found on the keyboard.

## Soft Off With Existing Designs

For existing designs, using soft off is as simple as placing the [Soft Off Behavior](../behaviors/soft-off.md) in your keymap and then invoking it. For splits, at least for now, you'll need to place it somewhere on each side of your keymap and trigger on both sides, starting from the peripheral side first.

You can then wake up the keyboard by pressing the reset button once, and repeating this for each side for split keyboards.

## Adding Soft On/Off To New Designs

### Hardware Design

ZMK's soft on/off requires a dedicated GPIO pin to be used to trigger powering off, and to wake the core from the
soft off state when it goes active again later.

#### Simple Direct Pin

The simplest way to achieve this is with a push button between a GPIO pin and ground.

#### Matrix-Integrated Hardware Combo

Another, more complicated option is to tie two of the switch outputs in the matrix together through an AND gate and connect that to the dedicated GPIO pin. This way you can use a key combination in your existing keyboard matrix to trigger soft on/off. To make this work best, the two switches used should both be driven by the same matrix input pin so that both will be active simultaneously on the AND gate inputs. The alternative is to use a combination of diodes and capacitors to ensure both pins are active/high at the same time even if scanning sets them high at different times.

### Firmware Changes

Several items work together to make both triggering soft off properly, and setting up the device to _wake_ from soft off work as expected.

#### GPIO Key

Zephyr's basic GPIO Key concept is used to configure the GPIO pin that will be used for both triggering soft off and waking the device later. Here is an example for a keyboard with a dedicated on/off push button that is a direct wire between the GPIO pin and ground:

```
/ {
    keys {
        compatible = "gpio-keys";
        wakeup_key: wakeup_key {
            gpios = <&gpio0 2 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
        };
    };
};
```

GPIO keys are defined using child nodes under the `gpio-keys` compatible node. Each child needs just one property defined:

- The `gpios` property should be a phandle-array with a fully defined GPIO pin and with the correct pull up/down and active high/low flags set. In the above example the soft on/off would be triggered by pulling the specified pin low, typically by pressing a switch that has the other leg connected to ground.

#### Behavior Key

Next, we will create a new "behavior key". Behavior keys are an easy way to tie a keymap behavior to a GPIO key outside of the normal keymap processing. They do _not_ do the normal keymap processing, so they are only suitable for use with basic behaviors, not complicated macros, hold-taps, etc.

In this case, we will be creating a dedicated instance of the [Soft Off Behavior](../behaviors/soft-off.md) that will be used only for our hardware on/off button, then binding it to our key:

```
/ {
    behaviors {
        hw_soft_off: behavior_hw_soft_off {
            compatible = "zmk,behavior-soft-off";
            #binding-cells = <0>;
            label = "HW_SO";
            hold-time-ms = <5000>;
        };
    };

    soft_off_behavior_key {
        compatible = "zmk,behavior-key";
        bindings = <&hw_soft_off>;
        key = <&wakeup_key>;
    };
};
```

Here are the properties for the behavior key node:

- The `compatible` property for the node must be `zmk,behavior-key`.
- The `bindings` property is a phandle to the soft off behavior defined above.
- The `key` property is a phandle to the GPIO key defined earlier.

If you have set up your on/off to be controlled by a matrix-integrated combo, the behavior key needs use a different driver that will handle detecting the pressed state when the pin is toggled by the other matrix kscan driver:

```
/ {
    soft_off_behavior_key {
        compatible = "zmk,behavior-key-scanned";
        status = "okay";
        bindings = <&hw_soft_off>;
        key = <&wakeup_key>;
    };
};
```

Note that the only difference from the `soft_off_behavior_key` definition for GPIO keys above is the `compatible` value of `zmk,behavior-key-scanned`.

#### Wakeup Sources

Zephyr has general support for the concept of a device as a "wakeup source", which ZMK has not previously used. Adding soft off requires properly updating the existing `kscan` devices with the `wakeup-source` property, e.g.:

```
/ {
    kscan0: kscan_0 {
        compatible = "zmk,kscan-gpio-matrix";
        label = "KSCAN";
        diode-direction = "col2row";
        wakeup-source;

        ...
    };
};
```

#### Soft Off Waker

Next, we need to add another device which will be enabled only when the keyboard is going into soft off state, and will configure the previously declared GPIO key with the correct interrupt configuration to wake the device from soft off once it is pressed.

```
/ {
    wakeup_source: wakeup_source {
        compatible = "zmk,wakeup-trigger-key";

        trigger = <&wakeup_key>;
        wakeup-source;
    };
};
```

Here are the properties for the node:

- The `compatible` property for the node must be `zmk,wakeup-trigger-key`.
- The `trigger` property is a phandle to the GPIO key defined earlier.
- The `wakeup-source` property signals to Zephyr this device should not be suspended during the shutdown procedure.
- An optional `output-gpios` property contains a list of GPIO pins (including the appropriate flags) to set active before going into power off, if needed to ensure the GPIO pin will trigger properly to wake the keyboard. This is only needed for matrix integrated combos. For those keyboards, the list should include the matrix output needs needed so the combo hardware is properly "driven" when the keyboard is off.

Once that is declared, we will list it in an additional configuration section so that the ZMK soft off process knows it needs to enable this device as part of the soft off processing:

```
/ {
    soft_off_wakers {
        compatible = "zmk,soft-off-wakeup-sources";
        wakeup-sources = <&wakeup_source>;
    };
};
```

Here are the properties for the node:

- The `compatible` property for the node must be `zmk,soft-off-wakeup-sources`.
- The `wakeup-sources` property is a [phandle array](../config/index.md#devicetree-property-types) pointing to all the devices that should be enabled during the shutdown process to be sure they can later wake the keyboard.
