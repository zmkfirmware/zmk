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

- Reference: `&reset`
- Parameters: None

Example:

```
&reset
```

## Bootloader Reset

The bootloader reset behavior will reset the keyboard and put it into bootloader mode, allowing
you to flash a new firmware.

### Behavior Binding

- Reference: `&bootloader`
- Parameters: None

Example:

```
&bootloader
```
