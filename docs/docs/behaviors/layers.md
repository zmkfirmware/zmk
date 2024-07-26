---
title: Layer Behaviors
sidebar_label: Layers
---

## Summary

Often, you may want a certain key position to alter which layers are enabled, change the default layer, etc.
Some of those behaviors are still in the works; the ones that are working now are documented here.

:::note
Multiple layers can be active at the same time and activating a layer will not deactivate layers higher up in the "layer stack". See [Layers](../features/keymaps.mdx#layers) for more information.
:::

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
- Parameter: The layer number to enable while held, e.g. `1`

Example:

```dts
&mo LOWER
```

## Layer-Tap

The "layer-tap" behavior enables a layer when a key is held, and outputs a [keypress](key-press.md) when the key is only tapped for a short time.

### Behavior Binding

- Reference: `&lt`
- Parameter: The layer number to enable while held, e.g. `1`
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

The "toggle layer" behavior enables a layer if it is currently disabled, or disables it if enabled.

### Behavior Binding

- Reference: `&tog`
- Parameter: The layer number to enable/disable, e.g. `1`

Example:

```dts
&tog LOWER
```

## Conditional Layers

The "conditional layers" feature enables a particular layer when all layers in a specified set are active.
For more information, see [conditional layers](../features/conditional-layers.md).
