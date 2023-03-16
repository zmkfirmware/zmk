---
title: Soft On/Off
sidebar_label: Soft On/Off
---

If your keyboard has been built with support for it, ZMK has the ability to be powered on/off with the push
of a button, instead of using a power switch to cut the battery circuit.

:::note

The power off is accomplished by putting the MCU into a "soft off" state. Power is _not_ technically removed from the entire system, but the device will only be woken from the state by a few possible events.

:::

Once powered off, the keyboard will only wake up when:

- You press the same button/sequence that you pressed to power off the keyboard, or
- You press a reset button found on the keyboard

## Adding Soft On/Off to a Board

### Hardware Design

ZMK's soft on/off requires a dedicated GPIO pin to be used to trigger powering off, and to wake the core from the
soft off state when it goes active again later.

The simplest way to achieve this is with a push button between a GPIO pin and ground.

Another, more complicated option is to tie two of the switch outputs in the matrix together through an AND gate and connect that to the dedicated GPIO pin. This way you can use a key combination in your existing keyboard matrix to trigger soft on/off.

### Firmware Changes

To add soft on/off to a board or shield, a new node needs to be added to your devicetree source. Here is an example complete node:

```
/ {
    ...

    soft_on_off {
        compatible = "zmk,soft-on-off-gpio";
        input-gpios = <&gpio0 6 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
        wakeup-sources = <&kscan0>;
    };
};
```

Here are the requirements for the node:

- The `compatible` property for the node must be `zmk,soft-on-off-gpio`.
- The `input-gpios` property is a phandle to the GPIO pin, with the correct pull up/down and active high/low flag set. In the above example the soft on/off would be triggered by pulling the pin low.
- The `wakeup-sources` is a list of devices that needs to be shutdown before going into power off state, to ensure
  that _only_ our soft on/off device will wake up the keyboard later. Typically this points to the `kscan` node that
  is configured for the keyboard, which can also wake the keyboard from a ["deep sleep from idle"](../config/power.md#idlesleep) state.
- An optional `output-gpios` property contains a list of GPIO pins to set active before going into power off, if needed to ensure the GPIO pin will trigger properly to wake the keyboard.
