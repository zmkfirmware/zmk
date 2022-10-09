---
title: Caps Lock Behavior
sidebar_label: Caps Lock
---

## Summary

This set of behaviors offers enhahced versions of the caps lock key.


## Caps On

Enables caps lock when pressed, regarless of the current caps lock state.

### Behavior Binding
- Reference: `&caps_on`

Example:

```
&caps_on
```


## Caps Off

Disables caps lock when released, regarless of the current caps lock state.

### Behavior Binding
- Reference: `&caps_off`

Example:

```
&caps_off
```


## Caps Hold

Enables caps lock when pressed, and disables it when released.

### Behavior Binding
- Reference: `&caps_hold`

Example:

```
&caps_hold
```


## Caps Word

Enables caps lock when pressed, and deactivate it when any key in the break list is pressed, or if the caps word key is pressed again.

### Behavior Binding
- Reference: `&caps_word`

Example:

```
&caps_word
```

### Configuration

#### Word Separators

By default, the caps word will remain active until space (`SPACE`), tab (`TAB`), enter (`ENTER`), or escape (`ESCAPE`) is pressed. If you would like to override this, you can set a new array of keys in the `disable-on-keys` property in your keymap:

```
&caps_word {
    disable-on-keys = <SPACE MINUS>;
};

/ {
    keymap {
        ...
    };
};
```

### Multiple Caps Words

If you want to use multiple caps words with different word separators, you can add additional caps word instances to use in your keymap:

```
/ {
    prog_caps: behavior_prog_caps_word {
        compatible = "zmk,behavior-capslock";
        label = "CAPS_WORD";
        #binding-cells = <0>;
        bindings = <&kp CAPSLOCK>;
        enable-on-press;
        disable-on-second-press;
        disable-on-keys = <SPACE TAB ENTER ESCAPE>;
    };

    keymap {
        ...
    };
};
```
