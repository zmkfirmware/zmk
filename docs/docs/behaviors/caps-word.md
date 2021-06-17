---
title: Caps Word Behavior
sidebar_label: Caps Word
---

## Summary

The caps word behavior behaves similar to a caps lock, but will automatically deactivate when one of the configured "break keycodes" is pressed, or if the caps word key is pressed again. For smaller keyboards, using [mod-taps](/docs/behaviors/mod-tap), this can help avoid repeated alternating holds when typing words in all caps.

### Behavior Binding

- Reference: `&caps_word`

Example:

```
&caps_word
```


### Configuration

#### Break Codes

By default, the caps word will deactivate when the user types space, comma, or a period. If you would like to override this, you can set a new
array of keys in the `breaks` property in your keymap:


```
&caps_word {
    breaks = <SPACE COMMA SEMI>;
};

/ {
    keymap {
        ...
    };
};
```

#### Applied Modifier(s)

In addition, if you would like *multiple* modifiers, instead of just `MOD_LSFT`, you can override the `mods` property:

```
&caps_word {
    mods = <MOD_LSFT | MOD_LALT>;
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
        breaks = <SPACE>;
    };

    keymap {
        ...
    };
};
```