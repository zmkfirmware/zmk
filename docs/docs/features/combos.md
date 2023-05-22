---
title: Combos
---

## Summary

Combo keys are a way to combine multiple keypresses to output a different key. For example, you can hit the Q and W keys on your keyboard to output escape.

## Configuration

Combos configured in your `.keymap` file, but are separate from the `keymap` node found there, since they are processed before the normal keymap. They are specified like this:

```
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
- (advanced) you can specify `slow-release` if you want the combo binding to be released when all key-positions are released. The default is to release the combo as soon as any of the keys in the combo is released. You can also specify `slow-release-positions` to select which key-positions _must_ be held to maintain the combo.
- (advanced) you can add `partial-hold-position` to the `bindings` array, optionally along with additional behaviors, to control what happens when a combo is partially released.

:::info

Key positions are numbered like the keys in your keymap, starting at 0. So, if the first key in your keymap is `Q`, this key is in position `0`. The next key (possibly `W`) will have position 1, etcetera.

:::

## Advanced usage

- Partially overlapping combos like `0 1` and `0 2` are supported.
- Fully overlapping combos like `0 1` and `0 1 2` are supported.
- You are not limited to `&kp` bindings. You can use all ZMK behaviors there, like `&mo`, `&bt`, `&mt`, `&lt` etc.

:::note Source-specific behaviors on split keyboards
Invoking a source-specific behavior such as one of the [reset behaviors](behaviors/reset.md) using a combo will always trigger it on the central side of the keyboard, regardless of the side that the keys corresponding to `key-positions` are on.
:::

See [combo configuration](/docs/config/combos) for advanced configuration options.

### Slow Release

If you want the combo binding to be released when all positions are released, instead of when any position is released, enable `slow-release`. This is useful for combos that are used to toggle a layer, for example.

However, you may want to continue to hold the combo when one position is held but not the other. For example, if the keys corresponding to the combo positions 0 and 1 are `&mo NAV` and `&kp A`, and the combo behavior is `&kp LEFT`, you may want to continue holding `LEFT` while you hold `A` and release `NAV`, but not if you hold `NAV` and release `A`. To solve this, you can specify `slow-release-positions` to select which keys must be held to maintain `slow-release`. In this example, you would specify `slow-release-positions = <1>`. In other words, the combo will be held as long _all_ keys in `slow-release-positions` are held, and released when _any_ key in `slow-release-positions` is released.

### Partial Holds

After pressing a combo, you may want to specify the behavior that is activated when the combo is partially released. For example, if the keys corresponding to the combo positions 0, 1, and 2 are `&tog NAV`, `&kp A`, and `&kp LSFT` and the combo behavior is `&kp LEFT`, you may want to activate `&mo NAV` when you release `A` or `LSFT` but continue to hold `NAV`, or activate `LSFT` when you release `NAV` or `A` but continue to hold `LSFT`.

To do this, you can add `partial-hold-position` to the `bindings` array, optionally along with associated behaviors. When no explicit behavior is specified, it will default to the behavior belonging to the key position.

In this example, you would specify:

```
combo_nav {
    timeout-ms = <50>;
    key-positions = <0 1 2>;
    bindings
        = <&kp LEFT>
        , <&partial-hold-position 0 &mo NAV>
        , <&partial-hold-position 2> // defaults to &kp LSFT
        ;
};
```

### Partial Holds with Slow Release Positions

Partial holds compliment `slow-release-positions`, by letting you control what happens when a combo is partially released. The best motivating example is a combo that is used to "accelerate" an existing thumb momentary layer with a key on that layer, allowing you to mash the keys together at the same time. The following layout is an example of this:

```
#define NAV 1
/ {
    combos {
        compatible = "zmk,combos";
        combo_nav {
            timeout-ms = <50>;
            key-positions = <0 1>;
            bindings = <&kp LEFT>, <&partial-hold-position 0>;
            slow-release;
            slow-release-positions = <1>;
        };
    };

    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &mo NAV &kp A &kp B
            >;
        };

        nav_layer {
            bindings = <
                &mo NAV &kp LEFT &kp RIGHT
            >;
        };
    };
};
```

In this example, you can press `LEFT` by pressing `NAV`, pressing `LEFT`, then releasing `NAV`. However, this requires a slight pause to ensure `NAV` was pressed before `LEFT`, so that `A` isn't pressed instead. What if you wanted to be able to mash the keys together at the same time, and achieve a consistent result? This is what the combo does — it allows you to press `LEFT` up to 50ms before `NAV` and still activate `LEFT`.

However, the introduction of the combo means that you can no longer release `LEFT` while holding `NAV` then press `RIGHT`. This is because the combo will be released when `LEFT` is released, and `B` will be pressed instead. To solve this, you can use `partial-hold-position` to specify that `NAV` should be pressed when `LEFT` is released. Finally, you can use `slow-release-positions` to specify that the combo should be held as long as `LEFT` is held, allowing you to use key-repeat while holding `LEFT` but releasing `NAV`.
