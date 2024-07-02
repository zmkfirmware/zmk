---
title: Layer Behaviors
sidebar_label: Layers
---

## Summary

Often, you may want a certain key position to alter which layers are enabled, change the default layer, etc.
Some of those behaviors are still in the works; the ones that are working now are documented here.

## Defines to Refer to Layers

When working with layers, you may have several different key positions with bindings that enable/disable those layers.
To make it easier to refer to those layers in your key bindings, and to change which layers are where later, you can
add a set of `#define`s at the top of your keymap file, and use those layer in your keymap.

For example, if you have three layers, you can add the following to the top of your keymap:

```dts
#define DEFAULT 0
#define LOWER   1
#define RAISE   2
```

This allows you to use those defines, e.g. `LOWER` later in your keymap.

## Momentary Layer

The "momentary layer" behavior enables a layer while a certain key is pressed. Immediately upon
activation of the key, the layer is enabled, and immediately upon release of the key, the layer is disabled
again.

### Behavior Binding

- Reference: `&mo`
- Parameter: The layer number to enable/disable, e.g. `1`

Example:

```dts
&mo LOWER
```

## Layer-Tap

The "layer-tap" behavior enables a layer when a key is held, and outputs a [keypress](key-press.md) when the key is only tapped for a short time.

### Behavior Binding

- Reference: `&lt`
- Parameter: The layer number to enable when held, e.g. `1`
- Parameter: The keycode to send when tapped, e.g. `A`

Example:

```dts
&lt LOWER SPACE
```

### Configuration

You can configure a different tapping term or tweak other properties noted in the [hold-tap](hold-tap.mdx#advanced-configuration) documentation page in your keymap:

```dts
&lt {
    tapping-term-ms = <200>;
};

/ {
    keymap {
        ...
    };
};
```

:::info
Functionally, the layer-tap is a [hold-tap](hold-tap.mdx) of the ["tap-preferred" flavor](hold-tap.mdx#flavors) and a [`tapping-term-ms`](hold-tap.mdx#tapping-term-ms) of 200 that takes in a [`momentary layer`](#momentary-layer) and a [keypress](key-press.md) as its "hold" and "tap" parameters, respectively.

For users who want to send a different [keycode](../codes/index.mdx) depending on if the same key is held or tapped, see [Mod-Tap](mod-tap.md).

Similarly, for users looking to create a keybind like the layer-tap that depending on how long the key is held, invokes behaviors like [sticky keys](sticky-key.md) or [key toggles](key-toggle.md), see [Hold-Tap](hold-tap.mdx).

:::

## To Layer

The "to layer" behavior enables a layer and disables _all_ other layers _except_ the default layer.

### Behavior Binding

- Reference: `&to`
- Parameter: The layer number to enable, e.g. `1`

Example:

```dts
&to 3
```

## Toggle Layer

The "toggle layer" behavior enables a layer until the layer is manually disabled.

### Behavior Binding

- Reference: `&tog`
- Parameter: The layer number to enable/disable, e.g. `1`

Example:

```dts
&tog LOWER
```

"Toggle layer" for a :

```dts
#define DEFAULT 0
#define NAVI    1

#define NONE 0

/ {
    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &tog NAVI       &kp KP_DIVIDE   &kp KP_MULTIPLY &kp KP_MINUS
                &kp NUMBER_7    &kp NUMBER_8    &kp NUMBER_9    &kp KP_PLUS
                &kp NUMBER_4    &kp NUMBER_5    &kp NUMBER_6    &kp KP_PLUS
                &kp NUMBER_1    &kp NUMBER_2    &kp NUMBER_3    &kp RETURN
                &kp NUMBER_0    &kp NUMBER_0    &kp DOT         &kp RETURN
            >;
        };

        nav_layer {
            bindings = <
                &tog NAVI       &kp KP_DIVIDE   &kp KP_MULTIPLY &kp KP_MINUS
                &kp HOME        &kp UP          &kp PAGE_UP     &kp KP_PLUS
                &kp LEFT        &none           &kp RIGHT       &kp KP_PLUS
                &kp END         &kp DOWN        &kp PAGE_DOWN   &kp RETURN
                &kp INSERT      &kp INSERT      &kp DEL         &kp RETURN
            >;
        };
    };
};
```

It is possible to use "toggle layer" to have keys that raise and lower the layers as well.

## Momentary Layer Lock

Even if you mostly use [momentary layers](#momentary-layer) instead of `&to` or `&tog`, it's occasionally useful to permanently enable a layer without needing to hold anything down. Instead of creating an additional `&tog` or `&to` binding for each such layer, you can use `&molock`.

If `&molock` is pressed while any number of `&mo` bindings are being held, those momentary layers will not be deactivated when the corresponding `&mo` key is released. As a result, those momentary layers become "locked" until that `&mo` key is pressed and released a second time or the layer becomes deactivated by some other means (e.g. a `&tog` binding for that layer or a `&to` binding for any other one).

If `&molock` is pressed while no `&mo` bindings are being held, it triggers a user-configurable fallback behavior. The default fallback behavior returns to the base layer (`&to 0`), deactivating any locked momentary layers in the process.

### Behavior Binding

- Reference: `&molock`

Example:

```dts
&molock
```

Lock a symbol layer:

```dts
#define BASE 0
#define SYMS 1

/ {
    keymap {
        compatible = "zmk,keymap";

        base_layer {
            bindings = <&mo SYMS  &kp Z     &kp M      &kp K  >;
        };
        symbol_layer {
            bindings = <&trans    &kp PLUS  &kp MINUS  &molock>;
        };
    };
};
```

Holding down the leftmost key (`&mo SYMS`), then pressing and releasing the rightmost key (`&molock`), will lock the symbol layer. Even after releasing the leftmost key, the symbol layer remains active.

To return to the base layer, press and release either the leftmost key (triggering the `&mo SYMS` behavior a second time) or the rightmost key (triggering the default fallback behavior for `&molock`).

### Configuration

You can configure a different fallback behavior by overriding the `bindings` property of the built-in `&molock` behavior. For example, to return to layer 1 (instead of layer 0):

```dts
&molock {
    bindings = <&to 1>;
};
```

You can also create any number of custom `&molock` behaviors by using `compatible = "zmk,behavior-momentary-layer-lock"` like so:

```dts
// Presses F if triggered while no momentary layers are active
kp_molock: kp_molock {
    compatible = "zmk,behavior-momentary-layer-lock";
    bindings = <&kp F>;
};
```

## Conditional Layers

The "conditional layers" feature enables a particular layer when all layers in a specified set are active.
For more information, see [conditional layers](../features/conditional-layers.md).
