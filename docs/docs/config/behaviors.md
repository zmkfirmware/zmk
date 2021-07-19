---
title: Behavior Configuration
sidebar_label: Behaviors
---

Some behaviors have properties to adjust how they behave. These can also be used
as templates to create custom behaviors when none of the built-in behaviors do
what you want.

See [Configuration Overview](/docs/config/index) for instructions on how to
change these settings.

See the [zmk/app/dts/behaviors/](https://github.com/zmkfirmware/zmk/tree/main/app/dts/behaviors)
folder for all default behaviors.

## Hold-Tap

Creates a custom behavior that triggers one behavior when a key is held or a
different one when the key is tapped.

See the [hold-tap behavior documentation](/docs/behaviors/hold-tap) for more details and examples.

### Devicetree

Applies to: `compatible = "zmk,behavior-hold-tap"`

| Property          | Type          | Description                                                                    | Default            |
| ----------------- | ------------- | ------------------------------------------------------------------------------ | ------------------ |
| `label`           | string        | Unique label for the node                                                      |                    |
| `#binding-cells`  | int           | Must be `<2>`                                                                  |                    |
| `bindings`        | phandle array | A list of two behaviors: one for hold and one for tap                          |                    |
| `tapping-term-ms` | int           | How long in milliseconds the key must be held to trigger a hold                |                    |
| `quick-tap-ms`    | int           | Tap twice within this period in milliseconds to trigger a tap, even when held  | -1                 |
| `flavor`          | string        | Adjusts how the behavior chooses between hold and tap                          | `"hold-preferred"` |
| `retro-tap`       | bool          | Triggers the tap behavior on release if no other key was pressed during a hold | false              |

The `flavor` property may be one of:

- `"hold-preferred"`
- `"balanced"`
- `"tap-preferred"`

See the [hold-tap behavior documentation](/docs/behaviors/hold-tap) for an explanation of each flavor.

| Node  | Behavior                                      |
| ----- | --------------------------------------------- |
| `&lt` | [Layer-tap](/docs/behaviors/layers#layer-tap) |
| `&mt` | [Mod-tap](/docs/behaviors/mod-tap)            |

## Mod-Morph

Creates a custom behavior that triggers one behavior when a key is pressed without
certain modifiers held or a different one if certain modifiers are held.

### Devicetree

Applies to: `compatible = "zmk,behavior-mod-morph"`

| Property         | Type          | Description                                                                         |
| ---------------- | ------------- | ----------------------------------------------------------------------------------- |
| `label`          | string        | Unique label for the node                                                           |
| `#binding-cells` | int           | Must be `<0>`                                                                       |
| `bindings`       | phandle array | A list of two behaviors: one for normal press and one for mod morphed press         |
| `mods`           | int           | A bit field of modifiers which will switch to the morph behavior if any are pressed |

See [dt-bindings/zmk/modifiers.h](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/modifiers.h)
for a list of modifiers.

You can use the following nodes to tweak the default behaviors:

| Node     | Behavior     |
| -------- | ------------ |
| `&gresc` | Grave escape |

## Sticky Key

Creates a custom behavior that triggers a behavior and keeps it pressed it until
another key is pressed and released.

See the [sticky key behavior](/docs/behaviors/sticky-key) and [sticky layer behavior](/docs/behaviors/sticky-layer)
documentation for more details and examples.

### Devicetree

Applies to: `compatible = "zmk,behavior-sticky-key"`

| Property           | Type          | Description                                                              | Default |
| ------------------ | ------------- | ------------------------------------------------------------------------ | ------- |
| `label`            | string        | Unique label for the node                                                |         |
| `#binding-cells`   | int           | Must match the number of parameters the `bindings` behavior uses         |         |
| `bindings`         | phandle array | A behavior (without parameters) to trigger                               |         |
| `release-after-ms` | int           | Releases the key after this many milliseconds if no other key is pressed | 1000    |
| `quick-release`    | bool          | Release the sticky key on the next key press instead of release          | false   |

You can use the following nodes to tweak the default behaviors:

| Node  | Behavior                                     |
| ----- | -------------------------------------------- |
| `&sk` | [Sticky key](/docs/behaviors/sticky-key)     |
| `&sl` | [Sticky layer](/docs/behaviors/sticky-layer) |
