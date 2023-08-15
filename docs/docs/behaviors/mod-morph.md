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
        };
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

### Advanced configuration

`keep-mods`

When a modifier specified in `mods` is being held, it won't be sent along with the morphed keycode unless it is also specified in `keep-mods`. By default `keep-mods` equals `0`, which means no modifier specified in `mods` will be sent along with the morphed keycode.

For example, the following configuration morphs `LEFT_SHIFT` + `BACKSPACE` into `DELETE`, and morphs `RIGHT_SHIFT` + `BACKSPACE` into `RIGHT_SHIFT` + `DELETE`.

```
/ {
    behaviors {
        bspc_del: backspace_delete {
            compatible = "zmk,behavior-mod-morph";
            label = "BACKSPACE_DELETE";
            #binding-cells = <0>;
            bindings = <&kp BACKSPACE>, <&kp DELETE>;
            mods = <(MOD_LSFT|MOD_RSFT)>;
            keep-mods = <(MOD_RSFT)>;
        };
    };
};
```
