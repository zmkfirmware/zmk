---
title: Caps Word Behavior
sidebar_label: Caps Word
---

## Summary

The caps word behavior activates caps lock when pressed, but will automatically deactivate it when any key in the break list is pressed, or if the caps word key is pressed again. For smaller keyboards using [mod-taps](/docs/behaviors/mod-tap), this can help avoid repeated alternating holds when typing words in all caps.

### Behavior Binding

- Reference: `&caps_word`

Example:

```
&caps_word
```

### Configuration

#### Break List

By default, the caps word will remain active until space (`SPACE`), tab (`TAB`), enter (`ENTER`), or escape (`ESCAPE`) is pressed. If you would like to override this, you can set a new array of keys in the `break-list` property in your keymap:

```
&caps_word {
    break-list = <SPACE MINUS>;
};

/ {
    keymap {
        ...
    };
};
```


### Multiple Caps Breaks

If you want to use multiple caps breaks with different codes to break the caps, you can add additional caps words instances to use in your keymap:

```
/ {
    prog_caps: behavior_prog_caps_word {
        compatible = "zmk,behavior-caps-word";
        label = "PROG_CAPS";
        #binding-cells = <0>;
        break-list = <SPACE TAB ENTER MINUS>;
    };

    keymap {
        ...
    };
};
```
