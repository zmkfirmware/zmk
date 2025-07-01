---
title: Soft Off Behavior
sidebar_label: Soft Off
---

## Summary

The soft off behavior is used to force the keyboard into an off state. Depending on the specific keyboard hardware, the keyboard can be turned back on again either with a dedicated on/off button that is available, or using the reset button found on the device.

Refer to the [soft off config](../../config/power.md#low-power-states) for details on enabling soft off in order to use this behavior.

For more information, see the [soft off section](../../features/low-power-states.md) of the low power states feature page.

### Behavior Binding

- Reference: `&soft_off`

Example:

```
&soft_off
```

### Configuration

#### Hold time

By default, the keyboard will be turned off as soon as the key bound to the behavior is released, even if the key is only tapped briefly. If you would prefer that the key need be held a certain amount of time before releasing, you can set the `hold-time-ms` to a non-zero value in your keymap:

```
&soft_off {
    hold-time-ms = <5000>; // Only turn off it the key is held for 5 seconds or longer.
};

/ {
    keymap {
        ...
    };
};
```

The peripheral half of a [split keyboard](../../features/split-keyboards.md) will always enter the soft off state immediately when triggering the behavior, regardless of the `hold-time-ms` setting. This is to ensure reliability, as otherwise the central may enter the soft off state before notifying the peripheral that it should also do so.

If you wish to change this setting, and thus accept the potential for reliability issues, you may remove the `split-peripheral-off-on-press` flag from the behavior:

```dts
&soft_off {
    /delete-property/ split-peripheral-off-on-press;
};
```
