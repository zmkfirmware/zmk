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

## Config

Refer to the [soft off config](../config/power.md#soft-off) for details on enabling soft off.

## Soft Off With Existing Designs

For existing designs, using soft off is as simple as placing the [Soft Off Behavior](../behaviors/soft-off.md) in your keymap and then invoking it.

You can then wake up the keyboard by pressing the reset button once, and repeating this for each side for split keyboards.

## Hardware Changes For New Designs

ZMK's dedicated soft on/off pin feature requires a dedicated GPIO pin to be used to trigger powering off, and to wake the core from the
soft off state when it goes active again later.

### Simple Direct Pin

The simplest way to achieve this is with a push button between a GPIO pin and ground.

### Matrix-Integrated Hardware Combo

Another, more complicated option is to tie two of the switch outputs in the matrix together through an AND gate and connect that to the dedicated GPIO pin. This way you can use a key combination in your existing keyboard matrix to trigger soft on/off. To make this work best, the two switches used should both be driven by the same matrix input pin so that both will be active simultaneously on the AND gate inputs. The alternative is to connect the switch to two MOSFETs that trigger both the regular matrix connect and the connect to the AND gate to ensure both pins are active/high at the same time even if scanning sets them high at different times.

## Firmware Changes For New Designs

Several items work together to make both triggering soft off properly, and setting up the device to _wake_ from soft off work as expected. In addition, some small changes are needed to keep the regular idle deep sleep functionality working.

### Wakeup Sources

Zephyr has general support for the concept of a device as a "wakeup source", which ZMK has not previously used. Adding soft off requires properly updating the existing `kscan` devices with the `wakeup-source` property to ensure they will still work to wake the device from regular inactive deep sleep, e.g.:

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

### GPIO Key

Zephyr's basic GPIO Key concept is used to configure the GPIO pin that will be used for both triggering soft off and waking the device later. Here is an example for a keyboard with a dedicated on/off push button that is a direct wire between the GPIO pin and ground:

```
/ {
    keys {
        compatible = "gpio-keys";
        soft_off_key: soft_off_key {
            gpios = <&gpio0 2 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
        };
    };
};
```

GPIO keys are defined using child nodes under the `gpio-keys` compatible node. Each child needs just one property defined:

- The `gpios` property should be a phandle-array with a fully defined GPIO pin and with the correct pull up/down and active high/low flags set. In the above example the soft on/off would be triggered by pulling the specified pin low, typically by pressing a switch that has the other leg connected to ground.

### Soft Off Behavior Instance

To use the [soft off behavior](../behaviors/soft-off.md) outside of a keymap, add an instance of the behavior to your `.overlay`/`.dts` file:

```
/ {
    behaviors {
        hw_soft_off: hw_soft_off {
            compatible = "zmk,behavior-soft-off";
            #binding-cells = <0>;
            hold-time-ms = <5000>;
        };
    };
};
```

### KScan Sideband Behavior

The kscan sideband behavior driver will be used to trigger the [soft off behavior](../behaviors/soft-off.md) "out of band" from the normal keymap processing. To do so, it will decorate/wrap an underlying kscan driver. What kscan driver will vary for simple direct pin vs. matrix-integrated hardware combo.

#### Simple direct pin

With a simple direct pin setup, the The [direct kscan](../config/kscan.md) driver can be used with a GPIO key, to make a small "side matrix":

```
    soft_off_direct_scan: soft_off_direct_scan {
        compatible = "zmk,kscan-gpio-direct";
        input-keys = <&on_off_key>;
        wakeup-source;
    };
```

With that in place, the kscan sideband behavior will wrap the new driver:

```
/ {
    side_band_behavior_triggers: side_band_behavior_triggers {
        compatible = "zmk,kscan-sideband-behaviors";

        kscan = <&soft_off_direct_scan>;
        auto-enable;
        wakeup-source;

        soft_off {
            column = <0>;
            row = <0>;
            bindings = <&hw_soft_off>;
        };
    };
};
```

Finally, we will list the kscan instance in an additional configuration section so that the ZMK soft off process knows it needs to enable this device as part of the soft off processing so it can _also_ wake the keyboard from soft off when pressed:

```
/ {
    soft_off_wakers {
        compatible = "zmk,soft-off-wakeup-sources";
        wakeup-sources = <&soft_off_direct_scan>;
    };
};
```

Here are the properties for the node:

- The `compatible` property for the node must be `zmk,soft-off-wakeup-sources`.
- The `wakeup-sources` property is a [phandle array](../config/index.md#devicetree-property-types) pointing to all the devices that should be enabled during the shutdown process to be sure they can later wake the keyboard.

#### Matrix-integrated hardware combo

For this case, you will supplement the existing kscan matrix, by adding the additional pin as another entry in
the `row-gpios`/`col-gpios` for whichever pins are used to read the matrix state. For example, for an existing matrix like:

```
    kscan: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        wakeup-source;
        label = "KSCAN";
        debounce-press-ms = <1>;
        debounce-release-ms = <5>;

        diode-direction = "col2row";

        col-gpios
            = <&gpio0 12 (GPIO_ACTIVE_HIGH)>
            , <&gpio1 9  (GPIO_ACTIVE_HIGH)>
            ;
        row-gpios
            = <&gpio0 19 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            , <&gpio0 4  (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            ;
    };
```

you would add another row value:

```
    kscan: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        wakeup-source;
        label = "KSCAN";
        debounce-press-ms = <1>;
        debounce-release-ms = <5>;

        diode-direction = "col2row";

        col-gpios
            = <&gpio0 12 (GPIO_ACTIVE_HIGH)>
            , <&gpio1 9  (GPIO_ACTIVE_HIGH)>
            ;
        row-gpios
            = <&gpio0 19 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            , <&gpio0 4  (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            , <&gpio0 2  (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            ;
    };
```

With that in place, you would decorate the kscan driver:

```
    side_band_behavior_triggers: side_band_behavior_triggers {
        compatible = "zmk,kscan-sideband-behaviors";
        wakeup-source;
        kscan = <&kscan>;
        soft_off {
            column = <0>;
            row = <3>;
            bindings = <&hw_soft_off>;
        };
    };
```

Critically, the `column` and `row` values would correspond to the location of the added entry.

Lastly, which is critical, you would update the `zmk,kscan` chosen value to point to the new kscan instance:

```
    chosen {
        ...
        zmk,kscan = &side_band_behavior_triggers;
        ...
    };
```

Here are the properties for the kscan sideband behaviors node:

- The `compatible` property for the node must be `zmk,kscan-sideband-behaviors`.
- The `kscan` property is a phandle to the inner kscan instance that will have press/release events intercepted.

The child nodes allow setting up the behaviors to invoke directly for a certain row and column:

- The `row` and `column` properties set the values to intercept and trigger the behavior for.
- The `bindings` property references the behavior that should be triggered when the matching row and column event triggers.

### Soft Off Waker

Next, we need to add another device which will be enabled only when the keyboard is going into soft off state, and will configure the previously declared GPIO key with the correct interrupt configuration to wake the device from soft off once it is pressed.

```
/ {
    wakeup_source: wakeup_source {
        compatible = "zmk,gpio-key-wakeup-trigger";

        trigger = <&on_off_key>;
        wakeup-source;
    };
};
```

Here are the properties for the node:

- The `compatible` property for the node must be `zmk,gpio-key-wakeup-trigger`.
- The `trigger` property is a phandle to the GPIO key defined earlier.
- The `wakeup-source` property signals to Zephyr this device should not be suspended during the shutdown procedure.
- An optional `extra-gpios` property contains a list of GPIO pins (including the appropriate flags) to set active before going into power off, if needed to ensure the GPIO pin will trigger properly to wake the keyboard. This is only needed for matrix integrated combos. For those keyboards, the list should include the matrix output needs needed so the combo hardware is properly "driven" when the keyboard is off.

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
