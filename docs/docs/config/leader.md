---
title: Leader Configuration
sidebar_label: Leader
---

See the [Leader key behavior page](../keymaps/behaviors/leader-key.md) for more details and examples.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                                    | Type | Description                                         | Default |
| ----------------------------------------- | ---- | --------------------------------------------------- | ------- |
| `CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE` | int  | Maximum number of key positions per leader sequence | 4       |
| `CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY` | int  | Maximum number of sequences allowed per leader key  | 5       |

`CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE` sets the maximum length of a leader sequence.

If `CONFIG_ZMK_LEADER_MAX_SEQUENCES_PER_KEY` is 5, you can have up to 5 separate leader sequences each for each leader key.
