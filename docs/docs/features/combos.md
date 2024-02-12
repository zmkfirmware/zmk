---
title: Combos
---

## Summary

Combo keys are a way to combine multiple keypresses to output a different key. For example, you can hit the Q and W keys on your keyboard to output escape.

### Configuration

Combos configured in your `.keymap` file, but are separate from the `keymap` node found there, since they are processed before the normal keymap. They are specified like this:

```dts
/ {
    combos {
        compatible = "zmk,combos";
        combo_esc {
            timeout-ms = <50>;
            key-positions = <0 1>;
            bindings = <&kp ESC>;
        };
    };
};
```

- The name of the combo doesn't really matter, but convention is to start the node name with `combo_`.
- The `compatible` property should always be `"zmk,combos"` for combos.
- All the keys in `key-positions` must be pressed within `timeout-ms` milliseconds to trigger the combo.
- `key-positions` is an array of key positions. See the info section below about how to figure out the positions on your board.
- `layers = <0 1...>` will allow limiting a combo to specific layers. This is an _optional_ parameter, when omitted it defaults to global scope.
- `bindings` is the behavior that is activated when the behavior is pressed.
- (advanced) you can specify `slow-release` if you want the combo binding to be released when all key-positions are released. The default is to release the combo as soon as any of the keys in the combo is released.
- (advanced) you can specify a `require-prior-idle-ms` value much like for [hold-taps](behaviors/hold-tap.mdx#require-prior-idle-ms). If any non-modifier key is pressed within `require-prior-idle-ms` before a key in the combo, the combo will not trigger.

:::info

Key positions are numbered like the keys in your keymap, starting at 0. So, if the first key in your keymap is `Q`, this key is in position `0`. The next key (possibly `W`) will have position 1, etcetera.

:::

### Advanced usage

- Partially overlapping combos like `0 1` and `0 2` are supported.
- Fully overlapping combos like `0 1` and `0 1 2` are supported.
- You are not limited to `&kp` bindings. You can use all ZMK behaviors there, like `&mo`, `&bt`, `&mt`, `&lt` etc.

:::note[Source-specific behaviors on split keyboards]
Invoking a source-specific behavior such as one of the [reset behaviors](behaviors/reset.md) using a combo will always trigger it on the central side of the keyboard, regardless of the side that the keys corresponding to `key-positions` are on.
:::

See [combo configuration](/docs/config/combos) for advanced configuration options.
