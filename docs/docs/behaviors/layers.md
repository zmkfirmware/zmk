---
title: Layer Behaviors
sidebar_label: Layers
---

## Summary

Often, you may want a certain key position to alter which layers are enabled, change the default layer, etc.
Some of those behaviors are still in the works; the ones that are working now are documented here.

## Defines To Refer To Layers

When working with layers, you may have several different key positions with bindings that enable/disable those layers.
To make it easier to refer to those layers in your key bindings, and to change which layers are where later, you can
add a set of `#define`s at the top of your keymap file, and use those layer in your keymap.

For example, if you have three layers, you can add the following to the top of your keymap:

```
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

```
&mo LOWER
```

## Layer-tap

The "layer-tap" behavior enables a layer when a key is held, and outputs a [keypress](key-press.md) when the key is only tapped for a short time.

### Behavior Binding

- Reference: `&lt`
- Parameter: The layer number to enable when held, e.g. `1`
- Parameter: The keycode to send when tapped, e.g. `A`

Example:

```
&lt LOWER SPACE
```

:::info
Functionally, the layer-tap is a [hold-tap](hold-tap.md) of the ["tap-preferred" flavor](hold-tap.md/#flavors) and a [`tapping-term-ms`](hold-tap.md/#tapping-term-ms) of 200 that takes in a [`momentary layer`](#momentary-layer) and a [keypress](key-press.md) as its "hold" and "tap" parameters, respectively.

For users who want to send a different [keycode](../codes/index.mdx) depending on if the same key is held or tapped, see [Mod-Tap](mod-tap.md).

Similarly, for users looking to create a keybind like the layer-tap that depending on how long the key is held, invokes behaviors like [sticky keys](sticky-key.md) or [key toggles](key-toggle.md), see [Hold-Tap](hold-tap.md).

:::

## To Layer

The "to layer" behavior enables a layer and disables _all_ other layers _except_ the default layer.

### Behavior Binding

- Reference: `&to`
- Parameter: The layer number to enable, e.g. `1`

Example:

```
&to 3
```

## Toggle Layer

The "toggle layer" behavior enables a layer until the layer is manually disabled.

### Behavior Binding

- Reference: `&tog`
- Parameter: The layer number to enable/disable, e.g. `1`

Example:

```
&tog LOWER
```

"Toggle layer" for a :

```
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

## Conditional Layers

The "conditional layers" feature enables a particular layer when all layers in a specified set are active.
For more information, see [conditional layers](../features/conditional-layers.md).
