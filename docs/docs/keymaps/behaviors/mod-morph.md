---
title: Mod-Morph Behavior
sidebar_label: Mod-Morph
---

## Summary

The mod-morph behavior invokes a different behavior depending on whether any of the specified modifiers are being held during the key press.

- If you tap the key by itself, the first behavior binding is activated.
- If you tap the key while holding (any of) the specified modifier(s), the second behavior binding is activated.

## Mod-Morph

ZMK provides a simple build-in mod-morph behavior.
When pressed with one of `LEFT_SHIFT`, `LEFT_GUI`, `RIGHT_SHIFT`, `RIGHT_GUI` active, a [key press](key-press.md) will be triggered using the second parameter.
Otherwise, a key press with the first parameter will be triggered.

### Behavior Binding

- Reference: `&mm`
- Parameters: The keycode usage IDs from the usage page, e.g. `N4` or `A`

Example:

```dts
&mm A B
```

## Custom Mod-Morph

If you want to trigger other behaviors or morph based on other combinations of modifiers, you can create a new mod-morph behavior.

Below is an example of how to implement a mod-morph that selects which behavior to trigger based on whether a `CTRL` modifier is active.
When `CTRL` is not active, a `&kp` with the first parameter passed to the behavior is triggered.
Otherwise, the layer whose number corresponds to the second parameter passed to the behavior is triggered.

```dts
/ {
    behaviors {
        mm_ctrl: mod_morph_control {
            compatible = "zmk,behavior-mod-morph-param";
            #binding-cells = <2>;
            mods = <(MOD_LCTL|MOD_RCTL)>;
            bindings = <&kp PLACEHOLDER>, <&mo PLACEHOLDER>;
            binding-params = <BINDING_PARAM(1,0) BINDING_PARAM(2,0)>;
        };
    };
};
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

### Binding Parameters

The `binding-params` property determines how the parameters passed to the mod-morph behavior are passed on to the behaviors listed in `bindings`.
It is an array which always has two `BINDING_PARAM(arg1,arg2)` elements - the first corresponds to the first behavior listed in `bindings`, the second corresponding to the second. The number chosen for `argX` determines what the Xth parameter passed to the behavior will be:

- `1`: The Xth parameter passed will be the first parameter that the mod-morph behavior received
- `2`: The Xth parameter passed will be the second parameter that the mod-morph behavior received
- `0`: The Xth parameter will be that which is written in the `bindings` array.

Note that any parameters in `bindings` behaviors which are to be replaced should be set to `PLACEHOLDER`.

Examples:

```dts
binding-params = <BINDING_PARAM(1,2) BINDING_PARAM(0,0)>;
```

The first behavior receives both input parameters, the second receives neither.

```dts
binding-params = <BINDING_PARAM(2,0) BINDING_PARAM(1,0)>;
```

The first behavior receives the second input parameter as its first parameter, the second behavior receives the first input parameter as its first parameter.

```dts
binding-params = <BINDING_PARAM(0,0) BINDING_PARAM(0,0)>;
```

Neither behavior receives any input parameter. Note that mod-morph always requires two parameters to be passed, so you will still need to write e.g. `&my_mm 0 0` in your keymap - the parameters passed will be ignored[^1].

### Advanced Configuration

#### `keep-mods`

When a modifier specified in `mods` is being held, it won't be sent along with the morphed keycode unless it is also specified in `keep-mods`. By default `keep-mods` equals `0`, which means no modifier specified in `mods` will be sent along with the morphed keycode.

For example, the following configuration morphs `LEFT_SHIFT` + `BACKSPACE` into `DELETE`, and morphs `RIGHT_SHIFT` + `BACKSPACE` into `RIGHT_SHIFT` + `DELETE`.

```dts
/ {
    behaviors {
        bspc_del: backspace_delete {
            compatible = "zmk,behavior-mod-morph-param";
            #binding-cells = <2>;
            bindings = <&kp BACKSPACE>, <&kp DELETE>;
            binding-params = <BINDING_PARAM(0,0) BINDING_PARAM(0,0)>;
            mods = <(MOD_LSFT|MOD_RSFT)>;
            keep-mods = <(MOD_RSFT)>;
        };
    };
};
```

#### Trigger conditions with multiple modifiers

Any modifier used in the `mods` property will activate a mod-morph; it isn't possible to require that multiple modifiers are held _together_ in order to activate it.
However, you can nest multiple mod-morph behaviors to achieve more complex decision logic, where you use one (or two) mod-morph behaviors in the `bindings` fields of another mod-morph.

As an example, consider the following two mod-morphs:

```dts
/ {
    behaviors {
        morph_BC: morph_BC {
            compatible = "zmk,behavior-mod-morph-param";
            #binding-cells = <2>;
            bindings = <&kp B>, <&kp C>;
            binding-params = <BINDING_PARAM(0,0) BINDING_PARAM(0,0)>;
            mods = <(MOD_LCTL|MOD_RCTL)>;
        };
        morph_ABC: morph_ABC {
            compatible = "zmk,behavior-mod-morph-param";
            #binding-cells = <2>;
            bindings = <&kp A>, <&morph_BC>;
            binding-params = <BINDING_PARAM(0,0) BINDING_PARAM(0,0)>;
            mods = <(MOD_LSFT|MOD_RSFT)>;
        };
    };
};
```

When you assign `&morph_ABC` to a key position and press it, it will output `A` by default. If you press it while a shift modifier is held it will output `B`, and if you are also holding a control modifier it will output `C` instead.

:::note[Karabiner-Elements (macOS) interfering with mod-morphs]

If the first modified key press sends the modifier along with the morphed keycode and [Karabiner-Elements](https://karabiner-elements.pqrs.org/) is running, disable the "Modify Events" toggle from Karabiner's "Devices" settings page for the keyboard running ZMK.

:::

[^1]: There exists a simplified version of mod-morph without any input parameters, `compatible="zmk,behavior-mod-morph"`:

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
