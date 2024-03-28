---
title: Caps Word Behavior
sidebar_label: Caps Word
---

## Summary

The caps word behavior behaves similar to a caps lock, but will automatically deactivate when any key not in a continue list is pressed, or if the caps word key is pressed again. For smaller keyboards using [mod-taps](/docs/behaviors/mod-tap), this can help avoid repeated alternating holds when typing words in all caps.

## Caps Word

Applies shift to capitalize alphabetic keys. Remains active but does not apply shift when numeric keys, underscore, backspace, or delete are pressed.

### Behavior Binding

- Reference: `&caps_word`

Example:

```dts
&caps_word
```

## Programmer Word

This is identical to `&caps_word` except that shift is also applied to `MINUS`. This can be useful when programming to type certain symbols. For example, enabling `&prog_word` and typing `some-constant` results in `SOME_CONSTANT`.

### Behavior Binding

- Reference: `&prog_word`

Example:

```dts
&prog_word
```

## Configuration

### Shift List

Caps word will apply shift to the following:

- Alphabetic keys (`A` through `Z`).
- Keys in the `shift-list` property (defaults to empty for `&caps_word` and `MINUS` for `&prog_word`).

You can add more key codes to be shifted by overriding the `shift-list` property in your keymap:

```dts
&caps_word {
    shift-list = <DOT>;
};

/ {
    keymap {
        ...
    };
};
```

### Continue List

Caps word will remain active when any of the following are pressed:

- Alphabetic keys and keys in `shift-list`.
- Numeric keys (`N0` through `N9` and `KP_N0` through `KP_N9`).
- Modifier keys.
- Keys which do not send a key code (such as layer keys).
- Keys in the `continue-list` property (defaults to `UNDERSCORE`, `BACKSPACE`, and `DELETE`).

Any other key code sent will turn off caps word.

You can add more key codes to continue caps word by setting a `continue-list` property in your keymap.

```dts
&caps_word {
    continue-list = <UNDERSCORE MINUS>;
};
```

### No Default Keys

By setting a `no-default-keys` property in your keymap, caps word will not automatically include alphabetic keys in `shift-list` or numeric keys in `continue-list`. This lets you explicitly control which keys are shifted and which continue a word, which may be useful in cases such as when your operating system is set to a non-US keyboard layout.

```dts
&caps_word {
    no-default-keys;
    shift-list = <A B C>;
    continue-list = <D E F>;
};
```

### Applied Modifier(s)

If you would like different or multiple modifiers instead of just `MOD_LSFT`, you can override the `mods` property:

```dts
&caps_word {
    mods = <(MOD_LSFT | MOD_LALT)>;
};
```

### Multiple Caps Breaks

You can add multiple caps words instances with different sets of properties in your keymap:

```dts
/ {
    behaviors {
        caps_sentence: caps_sentence {
            compatible = "zmk,behavior-caps-word";
            #binding-cells = <0>;
            continue-list = <SPACE>;
        };

        ctrl_word: ctrl_word {
            compatible = "zmk,behavior-caps-word";
            #binding-cells = <0>;
            mods = <MOD_LCTL>;
        };
    };

    keymap {
        ...
    };
};
```
