---
title: Soft Off Behavior
sidebar_label: Soft Off
---

## Summary

The soft off behavior is used to force the keyboard into an off state. Depending on the specific keyboard hardware, the keyboard can be turned back on again either with a dedicated on/off button that is available, or using the reset button found on the device.

Refer to the [soft off config](../config/power.md#soft-off) for details on enabling soft off in order to use this behavior.

For more information, see the [Soft Off Feature](../features/soft-off.md) page.

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
