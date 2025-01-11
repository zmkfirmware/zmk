---
title: Reset Behaviors
sidebar_label: Reset
---

## Summary

There are two available behaviors that can be used to trigger a reset of the keyboard.
The first is a soft reset, that will simply reset and re-run the currently flashed
firmware; the second when triggered will reset into the bootloader, allowing you to
flash a new firmware to the keyboard.

## Reset

The basic reset behavior will reset the keyboard and re-run the firmware flashed
to the device

### Behavior Binding

- Reference: `&sys_reset`
- Parameters: None

Example:

```dts
&sys_reset
```

## Bootloader Reset

The bootloader reset behavior will reset the keyboard and put it into bootloader mode, allowing
you to flash a new firmware.

### Behavior Binding

- Reference: `&bootloader`
- Parameters: None

Example:

```dts
&bootloader
```

## Split Keyboards

Both basic and bootloader reset behaviors are [source-specific](../../features/split-keyboards.md#source-locality-behaviors): This means that it affects the side of the keyboard that contains the behavior binding for split keyboards. For example if you press a key with the `&sys_reset` binding on the left half of the keyboard, the left half will be reset. If you want to be able to reset both sides you can put the bindings on both sides of the keyboard and activate it on the side you would like to reset.

Note that [combos](../combos.md) always invoke reset behavior bindings on the [central part](../../features/split-keyboards.md#central-and-peripheral-roles) of the split keyboard.
