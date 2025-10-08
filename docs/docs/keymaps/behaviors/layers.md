---
title: Layer Behaviors
sidebar_label: Layers
---

## Summary

Often, you may want a certain key position to alter which layers are enabled, change the default layer, etc.
Below are the list of behaviors that can be used to activate and deactivate layers.

:::note
Multiple layers can be active at the same time and activating a layer will not deactivate layers higher up in the "layer stack".

Layer numbers start at 0 following the order they are defined in the keymap node, for example `&mo 3` would activate the 4th layer node defined in the keymap.
See [Layers](../index.mdx#layers) for more information.
:::

## Momentary Layer

The "momentary layer" behavior enables a layer while a certain key is pressed. Immediately upon
activation of the key, the layer is enabled, and immediately upon release of the key, the layer is disabled
again.

### Behavior Binding

- Reference: `&mo`
- Parameter: The layer number to enable while held, e.g. `1`

Example:

```dts
&mo 3
```

## Layer-Tap

The ["layer-tap" behavior](hold-tap.mdx#layer-tap) enables a layer when a key is held, and outputs a [key press](key-press.md) when the key is only tapped for a short time.
See linked documentation for details.

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
&tog 3
```

### Toggle On and Toggle Off

For state-independent toggles, there exist two further behaviors:

- `&tog_on`: Toggles a layer on, even if it is already on.
- `&tog_off`: Toggles a layer off, even if it is already off.

Use these just as you would use `&tog`.

## Conditional Layers

The "conditional layers" feature enables a particular layer when all layers in a specified set are active.
For more information, see [conditional layers](../conditional-layers.md).

## Defines to Refer to Layers

When working with layers, you may have several different key positions with bindings that enable/disable those layers.
To make it easier to refer to those layers in your key bindings, and to change which layers are where later, you can
add a set of `#define`s at the top of your keymap file, and use those defines in your keymap.

For example, if you have three layers, you can add the following to the top of your keymap:

```dts
#define DEFAULT 0
#define LOWER   1
#define RAISE   2
```

This allows you to use those defines, e.g. `LOWER` later in your keymap.

```dts
&mo LOWER  // equivalent to &mo 1
```
