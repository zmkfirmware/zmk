---
title: Combos
---

## Summary

Combo keys are a way to combine multiple keypresses to output a different key. For example, you can hit the Q and W keys on your keyboard to output escape.

### Configuration

Combos configured in your `.keymap` file, but are separate from the `keymap` node found there. Alongside other options, they define a list of `triggers` that need to be activated by [combo-triggers](behaviors/combo-trigger.md) within `timeout-ms` seconds of each other to activate the output found in `bindings`. They are specified like this:

```dts
/ {
    combos {
        compatible = "zmk,combos";
        combo_esc {
            timeout-ms = <50>;
            triggers = <0 1>;
            bindings = <&kp ESC>;
        };
    };
};
```

- The name of the combo doesn't really matter, but convention is to start the node name with `combo_`.
- The `compatible` property should always be `"zmk,combos"` for combos.
- All the [combo-triggers](behaviors/combo-trigger.md) in `triggers` must be pressed within `timeout-ms` milliseconds to trigger the combo.
- `key-positions` is an array of key positions. See the info section below about how to figure out the positions on your board.
- `bindings` is the behavior that is activated when the behavior is pressed.
- (advanced) you can specify `slow-release` if you want the combo binding to be released when all key-positions are released. The default is to release the combo as soon as any of the keys in the combo is released.
- (advanced) you can specify a `require-prior-idle-ms` value much like for [hold-taps](behaviors/hold-tap.mdx#require-prior-idle-ms). If any non-modifier key is pressed within `require-prior-idle-ms` before a key in the combo, the combo will not trigger.

:::info

By default, triggers can be any non-negative number below 20. To increase (or decrease for memory savings) this limit, you will need to adjust the corresponding [`Kconfig flag`](../config/combos.md).

:::

### Advanced Usage

- Partially overlapping combos like `0 1` and `0 2` are supported.
- Fully overlapping combos like `0 1` and `0 1 2` are supported.
- You are not limited to `&kp` bindings. You can use all ZMK behaviors there, like `&mo`, `&bt`, `&mt`, `&lt` etc.

:::note[Source-specific behaviors on split keyboards]
Invoking a [source-specific behavior](../features/split-keyboards.md#source-locality-behaviors) such as one of the [reset behaviors](behaviors/reset.md) using a combo will always trigger it on the central side of the keyboard, regardless of the side that the keys corresponding to `key-positions` are on.
:::

See [combo configuration](../config/combos.md) for advanced configuration options.
