---
title: Mod-Morph Behavior
sidebar_label: Mod-Morph
---

## Summary

The mod-morph behavior invokes a different behavior depending on whether any of the specified modifiers are being held during the key press.

- If you tap the key by itself, the first behavior binding is activated.
- If you tap the key while holding (any of) the specified modifier(s), the second behavior binding is activated.

## Mod-Morph

### Configuration

Below is an example of how to implement the mod-morph "Grave Escape". When assigned to a key, pressing the key on its own will send an
Escape keycode but pressing it while a shift or GUI modifier is held sends the grave `` ` `` keycode instead:

```dts
/ {
    behaviors {
        gresc: grave_escape {
            compatible = "zmk,behavior-mod-morph";
            #binding-cells = <0>;
            bindings = <&kp ESC>, <&kp GRAVE>;
            mods = <(MOD_LGUI|MOD_LSFT|MOD_RGUI|MOD_RSFT)>;
        };
    };
};
```

Note that this specific mod-morph exists in ZMK by default using the binding `&gresc`.

### Behavior Binding

- Reference: `&gresc`
- Parameter: None

Example:

```dts
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

```dts
mods = <(MOD_LGUI|MOD_LSFT|MOD_RGUI|MOD_RSFT)>;
```

### Advanced Configuration

#### `keep-mods`

When a modifier specified in `mods` is being held, it won't be sent along with the morphed keycode unless it is also specified in `keep-mods`. By default `keep-mods` equals `0`, which means no modifier specified in `mods` will be sent along with the morphed keycode.

For example, the following configuration morphs `LEFT_SHIFT` + `BACKSPACE` into `DELETE`, and morphs `RIGHT_SHIFT` + `BACKSPACE` into `RIGHT_SHIFT` + `DELETE`.

```dts
/ {
    behaviors {
        bspc_del: backspace_delete {
            compatible = "zmk,behavior-mod-morph";
            #binding-cells = <0>;
            bindings = <&kp BACKSPACE>, <&kp DELETE>;
            mods = <(MOD_LSFT|MOD_RSFT)>;
            keep-mods = <(MOD_RSFT)>;
        };
    };
};
```

:::note[Karabiner-Elements (macOS) interfering with mod-morphs]

If the first modified key press sends the modifier along with the morphed keycode and [Karabiner-Elements](https://karabiner-elements.pqrs.org/) is running, disable the "Modify Events" toggle from Karabiner's "Devices" settings page for the keyboard running ZMK.

:::
