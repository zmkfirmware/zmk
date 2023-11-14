---
title: Soft Power Off Behavior
sidebar_label: Soft Power Off
---

## Summary

There are two available behaviors that can be used to manually control the power state of the keyboard. Both options will put the keyboard into deep sleep mode, but they differ in timing.

The first behavior is a soft power-off, which will immediately lock and power off the keyboard.
The second behavior adds a delay before entering the deep sleep mode. This delay allows
the kscan driver to enable interrupts (if `CONFIG_ZMK_KSCAN_DIRECT_POLLING` set to `n`), so
the keyboard can be activated simply by typing. It's important to note that the wake-up feature may not be available on all platforms.

## Soft Power Off

The soft power-off behavior will immediately put the keyboard into deep sleep mode, disabling the RGB underglow, backlight, and display.

However, please note that this behavior may not function as expected if USB logging is enabled and the keyboard is not connected to a host via USB.

### Behavior Binding

- Reference: `&soft_poweroff`
- Parameters: None

Example:

```
&soft_poweroff
```

## Sleep

The sleep behavior functions similarly to the soft power-off, with the addition of a two-second delay before entering the deep sleep state. This delay is particularly useful for keyboards that support wake by PORT events/interrupts, such as those based on NRF52840, as it gives user time to release all keys and allows the kscan driver to configure the necessary interrupts for waking up the keyboard if polling is disabled.

When the sleep behavior is triggered, the keyboard will enter the same deep sleep mode
as it would if `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT` was exceeded. For more information
on this mode, please refer to [Idle/Sleep](../config/power.md).

A known limitation is that if any key is pressed just before going into deep sleep mode,
the kscan driver may not switch to interrupt mode, preventing the keyboard from waking up by typing.
In such cases, the sleep behavior is identical to the soft power-off behavior.

### Behavior Binding

- Reference: `&sleep`
- Parameters: None

Example:

```
&sleep
```

## Split Keyboards

Soft power off behaviors are global: This means that when triggered, they affect both the central and peripheral side of split keyboards.
