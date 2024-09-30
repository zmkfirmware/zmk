---
title: EC11 Encoders
sidebar_label: Encoders
---

EC11 encoder support can be added to your board or shield by adding the appropriate lines to your board/shield's configuration (.conf), devicetree (.dtsi/.overlay), and keymap (.keymap) files.

## Configuration File

In your configuration file you will need to add the following lines so that the encoders can be enabled/disabled:

```ini
# Uncomment to enable encoder
# CONFIG_EC11=y
# CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
```

These should be commented by default for encoders that are optional/can be swapped with switches, but can be uncommented if encoders are part of the default design.

:::note
If building locally for split boards, you may need to add these lines to the specific half's configuration file as well as the combined configuration file, see the [configuration overview](../../config/index.md) for details.
:::

## Devicetree File

In your devicetree file you will need to define each sensor with their properties. For split keyboards, do this in the .dtsi file that is shared by all parts; otherwise do it in the .dts (for boards) or .overlay (shields) file, see [configuration overview](../../config/index.md#devicetree-files) for details. Add the following lines:

```dts
    left_encoder: encoder_left {
        compatible = "alps,ec11";
        a-gpios = <PIN_A (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
        b-gpios = <PIN_B (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
        steps = <80>;
        status = "disabled";
    };
```

Here you need to replace `PIN_A` and `PIN_B` with the appropriate pins that your PCB utilizes for the encoder(s). See shield overlays section in the [new shield guide](new-shield.mdx#shield-overlays) on the appropriate node label and pin number to use for GPIOs.

The `steps` property should corresponded to the documented pulses per rotation for the encoders used on the keyboard, typically found on the datasheet of the component. If users use different encoders when they build, the value can be overridden in their keymap.

Add additional encoders as necessary by duplicating the above lines, replacing `left` with whatever you would like to call your encoder, and updating the pins.

Once you have defined the encoder sensors, you will have to add them to the list of sensors:

```dts
    sensors: sensors {
        compatible = "zmk,keymap-sensors";
        sensors = <&left_encoder &right_encoder>;
        triggers-per-rotation = <20>;
    };
```

In this example, a `left_encoder` and `right_encoder` are both added. Additional encoders can be added with spaces separating each, and the order they are added here determines the order in which you define their behavior in your keymap.

In addition, a default value for the number of times the sensors trigger the bound behavior per full rotation is set via the `triggers-per-rotation` property. See [Encoders Config](../../config/encoders.md#devicetree) for more details.

Add the following lines to the .dts/.overlay file that contains the encoder to enable it:

```dts
&left_encoder {
    status = "okay";
};
```

Make sure to add this to the .dts/.overlay file, rather than any shared (.dtsi) files.

## Keymap

Add the following line to your keymap file to add default encoder behavior bindings:

```dts
sensor-bindings = <&inc_dec_kp C_VOL_UP C_VOL_DN>;
```

Add additional bindings as necessary to match the default number of encoders on your board. See the [Encoders](../../features/encoders.md) and [Keymaps](../../keymaps/index.mdx) documentation pages for more details.
