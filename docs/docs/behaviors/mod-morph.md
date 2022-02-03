---
title: Mod-Morph Behavior
sidebar_label: Mod-Morph
---

## Summary

The Mod-Morph behavior sends a different keypress, depending on whether a specified modifier is being held during the keypress.

- If you tap the key by itself, the first keycode is sent.
- If you tap the key while holding the specified modifier, the second keycode is sent.

## Mod-Morph

The Mod-Morph behavior acts as one of two keycodes, depending on if the required modifier is being held during the keypress.

When the modifier is being held it is sent along with the morphed keycode. This can cause problems when the morphed keycode and modifier have an existing relationship (such as `shift-delete` or `ctrl-v` on many operating systems).

As a remedy, you can add the optional attribute `masked_mods`, containing
the bitwise OR of the modifiers that should be disabled while the key is held,
so that they are not included in the report sent to the host.

### Configuration

An example of how to implement the mod-morph "Grave Escape":

```
/ {
    behaviors {
        gresc: grave_escape {
            compatible = "zmk,behavior-mod-morph";
            label = "GRAVE_ESCAPE";
            #binding-cells = <0>;
            bindings = <&kp ESC>, <&kp GRAVE>;
            mods = <(MOD_LGUI|MOD_LSFT|MOD_RGUI|MOD_RSFT)>;
            masked_mods = <(MOD_LGUI|MOD_LSFT)>; // don't send left modifiers
        };
    };

    keymap {
        ...
    };
};
```

Note that this specific mod-morph exists in ZMK by default using code `&gresc`.

### Behavior Binding

- Reference: `&gresc`
- Parameter: None

Example:

```
&gresc
```

### Mods

This is how you determine what modifiers will activate the morphed version of the keycode.

Available Modifiers:

- `MOD_LSFT`
- `MOD_RSFT`
- `MOD_LCTL`
- `MOD_RCTL`
- `MOD_LALT`
- `MOD_RALT`
- `MOD_LGUI`
- `MOD_RGUI`

Example:

```
mods = <(MOD_LGUI|MOD_LSFT|MOD_RGUI|MOD_RSFT)>;
```
