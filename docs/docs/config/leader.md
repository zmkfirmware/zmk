---
title: Leader Configuration
sidebar_label: Leader
---

See the [Leader feature page](../features/leader.md) for more details and examples.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                                    | Type | Description                                                       | Default |
| ----------------------------------------- | ---- | ----------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE` | int  | Maximum number of leader sequences that use the same key position | 4       |
| `CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY` | int  | Maximum number of keys to press to activate a leader sequence     | 5       |

`CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE` sets the maximum length of a leader sequence.

If `CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY` is 5, you can have 5 separate leader sequences that start with position `0`, 5 leader sequences that start with position `1`, and so on.

## Devicetree

Applies to: `compatible = "zmk,leader-sequences"`

Definition file: [zmk/app/dts/bindings/zmk,leader-sequences.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Cleader-sequences.yaml)

The `zmk,leader-sequences` node itself has no properties. It should have one child node per leader sequence.

Each child node can have the following properties:

| Property            | Type          | Description                                                                                                                                           | Default |
| ------------------- | ------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- | ------- |
| `bindings`          | phandle-array | A [behavior](../features/keymaps.md#behaviors) to run when the leader sequence is triggered                                                           |         |
| `key-positions`     | array         | A list of key position indices for the keys which should trigger the leader sequence                                                                  |         |
| `immediate-trigger` | bool          | Triggers the leader sequence when all keys are pressed instead of waiting for the timeout (only applicable when one leader sequence overlaps another) | false   |
| `layers`            | array         | A list of layers on which the leader sequence may be triggered. `-1` allows all layers.                                                               | `<-1>`  |

The `key-positions` array must not be longer than the `CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE` setting, which defaults to 4. If you want a leader sequence that triggers when pressing 5 keys, then you must change the setting to 5.
