---
title: Combo Configuration
sidebar_label: Combos
---

See the [Combos informational page](../keymaps/combos.md) for more details and examples.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                                | Type | Description                                                  | Default |
| ------------------------------------- | ---- | ------------------------------------------------------------ | ------- |
| `CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS` | int  | Maximum number of combos that can be active at the same time | 4       |

## Devicetree

Applies to: `compatible = "zmk,combos"`

Definition file: [zmk/app/dts/bindings/zmk,combos.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Ccombos.yaml)

The `zmk,combos` node itself has no properties. It should have one child node per combo.

Each child node can have the following properties:

| Property                | Type          | Description                                                                                                                               | Default       |
| ----------------------- | ------------- | ----------------------------------------------------------------------------------------------------------------------------------------- | ------------- |
| `bindings`              | phandle-array | A [behavior](../keymaps/index.mdx#behaviors) to run when the combo is triggered                                                           |               |
| `key-positions`         | array         | A list of key position indices for the keys which should trigger the combo                                                                |               |
| `timeout-ms`            | int           | All the keys in `key-positions` must be pressed within this time in milliseconds to trigger the combo                                     | 50            |
| `require-prior-idle-ms` | int           | If any non-modifier key is pressed within `require-prior-idle-ms` before a key in the combo, the key will not be considered for the combo | -1 (disabled) |
| `slow-release`          | bool          | Releases the combo when all keys are released instead of when any key is released                                                         | false         |
| `layers`                | array         | A list of layers on which the combo may be triggered. `-1` allows all layers.                                                             | `<-1>`        |
