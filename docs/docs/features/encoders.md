---
title: Encoders
sidebar_label: Encoders
---

Existing support for encoders in ZMK is focused around the five pin EC11 rotary encoder with push button design used in the majority of current keyboard and macropad designs.

:::note
Encoders are currently only support on the left/central sides of splits. For progress on this, see [#728](https://github.com/zmkfirmware/zmk/pull/728).
:::

## Enabling EC11 Encoders

To enable encoders for boards that have existing encoder support, uncomment the `CONFIG_EC11=y` and `CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y` lines in your board's .conf file in your `zmk-config/config` folder. Save and push your changes, then download and flash the new firmware.

## Customizing EC11 Encoder Behavior

Encoder behavior in ZMK is configured in two different locations as the push button and rotation behaviors are handled in two separate ways.

### Push Button

Keyboards and macropads with encoder support will typically take the two EC11 pins responsible for the push button and include them as part of the matrix for the keys. To configure what is sent by the push button, find the encoder's position in the keyboard matrix and assign it a behavior the same as you would any other key.

### Rotation

Rotation is handled separately as a type of sensor. The behavior for this is set in `sensor-bindings`, which is defined in each keymap layer in the following format:

```
sensor-bindings = <BINDING CW_KEY CCW_KEY>;
```

- `BINDING`, for now, has only one behavior available; `&inc_dec_kp` for key presses (see [Key Press](../behaviors/key-press.md) for details on available keycodes).
- `CW_KEY` is the keycode activated by a clockwise turn.
- `CCW_KEY` is the keycode activated by a counter-clockwise turn.

Additional encoders can be configured by adding more `BINDING CW_KEY CCW_KEY` sets immediately after the first.

As an example, a complete `sensor-bindings` for a Kyria with two encoders could look like:

```
sensor-bindings = <&inc_dec_kp C_VOL_UP C_VOL_DN &inc_dec_kp PG_UP PG_DN>;
```

Here, the left encoder is configured to control volume up and down while the right encoder sends either Page Up or Page Down.

## Adding Encoder Support

See the [New Keyboard Shield](../development/new-shield.md#encoders) documentation for how to add or modify additional encoders to your shield.
