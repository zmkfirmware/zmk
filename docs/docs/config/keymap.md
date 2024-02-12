---
title: Keymap Configuration
sidebar_label: Keymap
---

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Keymap

### Devicetree

Applies to: `compatible = "zmk,keymap"`

Definition file: [zmk/app/dts/bindings/zmk,keymap.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Ckeymap.yaml)

The `zmk,keymap` node itself has no properties. It should have one child node per layer of the keymap, starting with the default layer (layer 0).

Each child node can have the following properties:

| Property          | Type          | Description                                                             |
| ----------------- | ------------- | ----------------------------------------------------------------------- |
| `display-name`    | string        | Name for the layer on displays                                          |
| `bindings`        | phandle-array | List of [key behaviors](../features/keymaps.mdx#behaviors), one per key |
| `sensor-bindings` | phandle-array | List of sensor behaviors, one per sensor                                |

Items for `bindings` must be listed in the order the keys are defined in the [keyboard scan configuration](kscan.md).

Items for `sensor-bindings` must be listed in the order the [sensors](#keymap-sensors) are defined.

## Keymap Sensors

### Devicetree

Applies to: `compatible = "zmk,keymap-sensors"`

| Property  | Type     | Description          |
| --------- | -------- | -------------------- |
| `sensors` | phandles | List of sensor nodes |

The following types of nodes can be used as a sensor:

- [`alps,ec11`](encoders.md#ec11-encoders)
