---
title: Caps Lock Behavior
sidebar_label: Caps Lock
---

## Summary

The caps lock behavior provides an improved caps lock key.
Pressing the regular caps lock key toggles the state of caps lock on the host machine, this behavior offers more control on how and when caps lock get activated and deactivated, regardless of the current status on the host machine.

### Behavior Binding

Four pre-configured instances are provided:

- `&caps_on` enables caps lock
- `&caps_off` disables caps lock
- `&caps_hold` enables caps lock while held, and disables it when released
- `&caps_word` enables caps lock, and disables it when a word separator is typed or the key is pressed again

### Configuration

The following options allow to customize the caps lock behavior:

- `enable-on-press`: whether to enable caps lock when the key is pressed
- `disable-on-release`: whether to disable caps lock when the key is released
- `disable-on-next-release`: whether to disable caps lock when the key is released a second time
- `disable-on-keys`: list of keys after which caps lock is disabled

### Examples

The pre-configured `caps_word` enables caps lock when pressed, and disables it when `SPACE`, `TAB`, or `ENTER` is pressed, or the key is pressed again.

```
caps_word: behavior_caps_word {
    compatible = "zmk,behavior-capslock";
    label = "CAPS_WORD";
    #binding-cells = <0>;
    bindings = <&kp CAPSLOCK>;
    enable-on-press;
    disable-on-next-release;
    disable-on-keys = <SPACE TAB ENTER>;
};
```

A key to activate caps lock and disable it only after typing a whole line can be defined as:

```
/ {
    prog_caps: caps_line {
        compatible = "zmk,behavior-capslock";
        label = "CAPS_LINE";
        #binding-cells = <0>;
        bindings = <&kp CAPSLOCK>;
        enable-on-press;
        disable-on-keys = <ENTER>;
    };

    keymap {
        ...
    };
};
```
