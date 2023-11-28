---
title: Capslock Behavior
sidebar_label: Capslock
---

## Summary

The capslock behavior provides ways to create improved, configurable Caps Lock keys.
Pressing the regular Caps Lock key _toggles_ the state of caps lock on the host machine, making its effect dependent of said state.
This behavior can account for the current state to offer more control on how and when caps lock get activated and deactivated, regardless of the current status on the connected device.

### Behavior Binding

Four pre-configured instances are provided:

- `&capslock_on` enables caps lock
- `&capslock_off` disables caps lock
- `&capslock_hold` enables caps lock while held, and disables it when released
- `&capslock_word` enables caps lock, and disables it when a word separator is typed or the key is pressed again

In order to ensure compatibility with MacOS (which ignores short capslock presses by design), `&capslock_on_mac`, `&capslock_off_mac`, `&capslock_hold_mac`, `&capslock_word_mac` versions using a longer press duration of 95ms are also available.

### Configuration

The following options allow to customize the capslock behavior:

- `enable-on-press`: enable caps lock when the key is pressed
- `disable-on-release`: disable caps lock when the key is released
- `disable-on-next-release`: disable caps lock when the key is released a second time
- `disable-on-keys`: list of keys after which to disable capslock
- `capslock-press-duration`: duration of the capslock key press to generate (in milliseconds)

### Examples

The pre-configured `capslock_word` (which enables caps lock when pressed, and disables it when `SPACE`, `TAB`, `ENTER`, or `ESC` is pressed, or the key is pressed again) is defined as follow:

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

A key to activate caps lock and disable it only after typing a whole line can be defined as follow:

```
/ {
    capslock_line: capslock_line {
        compatible = "zmk,behavior-capslock";
        label = "CAPSLOCK_LINE";
        #binding-cells = <0>;
        enable-on-press;
        disable-on-keys = <ENTER ESC>;
    };

    keymap {
        ...
    };
};
```
