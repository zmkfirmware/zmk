---
title: Capslock Behavior
sidebar_label: Capslock
---

## Summary

The capslock behavior provides ways to create improved, configurable Caps Lock keys.
Pressing the regular Caps Lock key _toggles_ the state of caps lock on the host machine, making its effect dependent of said state.
This behavior can account for the current state to offer more control on how and when caps lock get activated and deactivated, regardless of the current status on the connected device.

### Behavior Binding

Three pre-configured instances are provided:

- `&capslock_on` enables caps lock
- `&capslock_off` disables caps lock
- `&capslock_hold` enables caps lock while held, and disables it when released

In order to ensure compatibility with MacOS (which ignores short capslock presses by design), `&capslock_on_mac`, `&capslock_off_mac`, `&capslock_hold_mac` versions using a longer press duration of 95ms are also available.

### Configuration

The following options allow to customize the capslock behavior:

- `enable-on-press`: enable caps lock when the key is pressed
- `disable-on-release`: disable caps lock when the key is released
- `disable-on-next-release`: disable caps lock when the key is released a second time
- `disable-on-keys`: list of keys after which to disable capslock
- `enable-while-keys`: list of keys for which to keep capslock enabled
- `capslock-press-duration`: duration of the capslock key press to generate (in milliseconds)

### Examples

In order to create a `capslock_word` key which enables caps lock when pressed, and disables it when `SPACE`, `TAB`, `ENTER`, or `ESC` is pressed, or the key is pressed again, a custom instance can be defined as follows:

```
capslock_word: behavior_capslock_word {
    compatible = "zmk,behavior-capslock";
    label = "CAPSLOCK_WORD";
    #binding-cells = <0>;
    capslock-press-ms = <5>;
    enable-on-press;
    disable-on-next-release;
    disable-on-keys = <SPACE TAB ENTER ESC>;
};
```

In order to define a similar key using a list keys for which capslock should be kept enabled, akin to the `&caps_word`'s `continue-list`, the `enable-while-keys` proerty can be used instead of `disable-on-keys` (note that, unlike `&caps_word`, this behavior does not use any implicit whitelist so an exhaustive contination list must be provided):

```
capslock_word: behavior_capslock_word {
    compatible = "zmk,behavior-capslock";
    label = "CAPSLOCK_WORD";
    #binding-cells = <0>;
    capslock-press-ms = <5>;
    enable-on-press;
    disable-on-next-release;
    enable-while-keys = <A B C ... Z ...>;
};
```
