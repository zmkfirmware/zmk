---
title: Conditional Layers
---

Conditional layers support activating a particular layer (called the `then-layer`) when all layers
in a specified set (called the `if-layers`) are active. This feature generalizes what's commonly
known as tri-layer support, allowing activation of two layers (usually called "lower" and "raise")
to trigger a third (usually called "adjust").

Another way to think of this feature is as a simple combo system for layers, just like the usual
[combos for behaviors](combos.md).

## Configuration

Conditional layers are configured via a `conditional_layers` node in your `.keymap` file as follows:

```
/ {
    conditional_layers {
        compatible = "zmk,conditional-layers";
        tri_layer {
            if-layers = <1 2>;
            then-layer = <3>;
        };
    };
};
```

Each conditional layer configuration may have whatever name you like, but it's helpful to choose
something self explanatory like `tri_layer`. The following properties are supported:

- `if-layers` specifies a set of layer numbers, all of which must be active for the conditional
  layer to trigger.
- `then-layer` specifies a layer number that should be activated if and only if all the layers
  specified in the `if-layers` property are active.

Therefore, in this example, layer 3 ("adjust") will be activated if and only if both layers 1
("lower") and 2 ("raise") are active.

:::tip
Since higher-numbered layers are processed first, a `then-layer` should generally have a higher
number than its associated `if-layers` so the `then-layer` can be accessed when active.
:::

:::info
Activating a `then-layer` in one conditional layer configuration can trigger the `if-layers`
condition in another configuration, possibly repeatedly.
:::

:::caution
When configured as a `then-layer`, a layer's activation status is entirely controlled by the
conditional layers feature. Even if the layer is activated for another reason (such as a [momentary
layer](../behaviors/layers.md#momentary-layer) behavior), it will be immediately deactivated if the
associated `then-layers` configuration is not met. As such, we recommend avoiding using regular
layer behaviors for `then-layer` targets.
:::
