---
title: Stenography
---

## Summary

Stenography is a way of writing used by Court Reporters and live captioners to
write at over 200 words per minute, using a chorded keyboard.

The [Open Steno Project](https://www.openstenoproject.org) has developed an
open-source application named Plover to translate steno strokes.

Plover supports any NKRO keyboard, but to not have to enable/disable Plover all
the time when wanting to use a normal keyboard usually serial stenography
protocols are used for many hobbyist steno machines. However we can't use those
protocols over BLE, so instead ZMK comes with an implementation of the [Plover
HID](https://github.com/dnaq/plover-machine-hid) protocol that can be used over both
USB and BLE and doesn't require any driver support from the operating system.

### Configuration

First enable the Plover HID protocol in your keymaps `.conf` file by adding the line:

```
CONFIG_ZMK_PLOVER_HID=y
```

Then add normal keypress behaviors to your keymap, but with usage codes that start with `PLV_`, .e.g.

```
plover_layer {
    bindings = <
      &kp &PLV_S1  &kp PLV_TL  &kp PLV_PL  &kp PLV_HL  &kp PLV_ST1  &PLV_ST3  &kp PLV_LR  &kp PLV_TR  &kp PLV_DR
      &kp &PLV_S2  &kp PLV_KL  &kp PLV_WL  &kp PLV_RL  &kp PLV_ST2  &PLV_ST4  &kp PLV_GR  &kp PLV_SR  &kp PLV_ZR
    >;
};
```

To support alternate stenography systems the Plover HID protocol also supports the keys `PLV_X1` to `PLV_X26`.

After reflashing your keyboard with a stenography enabled keymap you might need to restart your computer or restart the bluetooth
service so that it updates the HID descriptors of your computer.

Then install the
[plover-machine-hid](https://github.com/dnaq/plover-machine-hid) plugin to
plover according to its installation instructions.
