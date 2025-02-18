---
title: Combo Configuration
sidebar_label: Combos
---

See the [Combos informational page](../keymaps/combos.md) for more details and examples.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                                    | Type | Description                                                           | Default |
| ----------------------------------------- | ---- | --------------------------------------------------------------------- | ------- |
| `ZMK_COMBO_MAX_TRIGGER_NUM`               | int  | Upper bound of numbers that can be passed to a combo trigger behavior | 20      |
| `CONFIG_ZMK_COMBO_MAX_PRESSED_COMBOS`     | int  | Maximum number of combos that can be active at the same time          | 4       |
| `CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER` | int  | Maximum number of active combos that use the same trigger             | 5       |
| `CONFIG_ZMK_COMBO_MAX_TRIGGERS_PER_COMBO` | int  | Maximum number of triggers required to activate a combo               | 4       |

If `CONFIG_ZMK_COMBO_MAX_COMBOS_PER_TRIGGER` is 5, you can have 5 separate combos that use trigger `0`, 5 combos that use trigger `1`, and so on.

If you want a combo that triggers after activating 5 different [combo triggers](../keymaps/behaviors/combo-trigger.md), you must set `CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO` to 5.

## Devicetree

Applies to: `compatible = "zmk,combos"`

Definition file: [zmk/app/dts/bindings/zmk,combos.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Ccombos.yaml)

The `zmk,combos` node itself has no properties. It should have one child node per combo.

Each child node can have the following properties:

| Property                | Type          | Description                                                                                                                                       | Default       |
| ----------------------- | ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- | ------------- |
| `bindings`              | phandle-array | A [behavior](../keymaps/index.mdx#behaviors) to run when the combo is triggered                                                                   |               |
| `triggers`              | array         | A list of trigger ids which should trigger the combo                                                                                              |               |
| `timeout-ms`            | int           | All the triggers in `triggers` must be activated within this time in milliseconds to trigger the combo                                            | 50            |
| `require-prior-idle-ms` | int           | If any non-modifier key is pressed within `require-prior-idle-ms` before a trigger in the combo, the trigger will not be considered for the combo | -1 (disabled) |
| `slow-release`          | bool          | Releases the combo when all keys are released instead of when any key is released                                                                 | false         |

The `triggers` array must not be longer than the `CONFIG_ZMK_COMBO_MAX_TRIGGERS_PER_COMBO` setting, which defaults to 4. If you want a combo that triggers when activating 5 triggers, then you must change the setting to 5.
