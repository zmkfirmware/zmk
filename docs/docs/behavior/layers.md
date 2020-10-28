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
activation of the key, the layer is enabled, and immediately open release of the key, the layer is disabled
again.

### Behavior Binding

- Reference: `&mo`
- Parameter: The layer number to enable/disable, e.g. `1`

Example:

```
&mo LOWER
```

## Layer-tap

The "layer-tap" behavior enables a layer when a key is held, and output another key when the key is only tapped for a short time. For more information on the inner workings of layer-tap, see [hold-tap](./hold-tap.md).

### Behavior Binding

- Reference: `&lt`
- Parameter: The layer number to enable when held, e.g. `1`
- Parameter: The keycode to send when tapped, e.g. `A`

Example:

```
&lt LOWER SPC
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
                &tog NAVI &kp KDIV  &kp KMLT  &kp KMIN
                &kp NUM_7 &kp NUM_8 &kp NUM_9 &kp KPLS
                &kp NUM_4 &kp NUM_5 &kp NUM_6 &kp KPLS
                &kp NUM_1 &kp NUM_2 &kp NUM_3 &kp RET
                &kp NUM_0 &kp NUM_0 &kp DOT   &kp RET
			>;
		};

		nav_layer {
			bindings = <
                &tog NAVI &kp KDIV  &kp KMLT  &kp KMIN
                &kp HOME  &kp UARW  &kp PGUP  &kp KPLS
                &kp LARW  &none     &kp RARW  &kp KPLS
                &kp END   &kp DARW  &kp PGDN  &kp RET
                &kp INS   &kp INS   &kp DEL   &kp RET
            >;
		};
	};
};
```

It is possible to use "toggle layer" to have keys that raise and lower the layers as well.
