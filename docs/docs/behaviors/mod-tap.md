---
title: Mod-Tap Behavior
sidebar_label: Mod-Tap
---

## Summary

The Mod-Tap behavior sends a different keypress, depending on whether it's held or tapped.

- If you hold the key for longer than 200ms, the first keycode ("mod") is sent.
- If you tap the key (release before 200ms), the second keycode ("tap") is sent.

If you press another key within the 200ms, the 'mod' behavior is also activated.

## Mod-Tap

The Mod-Tap behavior either acts as a held modifier, or as a tapped keycode.

### Behavior Binding

- Reference: `&mt`
- Parameter #1: The keycode to be sent when activating as a modifier, e.g. `LSHIFT`
- Parameter #2: The keycode to sent when used as a tap, e.g. `A`, `B`.

Example:

```dts
&mt LSHIFT A
```

### Configuration

You can configure a different tapping term in your keymap:

```dts
&mt {
    tapping-term-ms = <400>;
};

/ {
    keymap {
        ...
    };
};
```

:::info
Under the hood, the mod-tap is simply a [hold-tap](hold-tap.mdx) of the ["hold-preferred" flavor](hold-tap.mdx#flavors) with a [`tapping-term-ms`](hold-tap.mdx#tapping-term-ms) of 200 that takes in two [keypresses](key-press.md) as its "hold" and "tap" parameters. This means that the mod-tap can be used to invoke **any** [keycode](../codes/index.mdx), and is not limited to only activating [modifier keys](../codes/modifiers.mdx) when it is held.

For users who want to momentarily access a specific [layer](../features/keymaps.mdx#layers) while a key is held and send a keycode when the same key is tapped, see [Layer-Tap](layers.md#layer-tap).

Similarly, for users looking to create a keybind like the mod-tap that invokes behaviors _other_ than [keypresses](key-press.md), like [sticky keys](sticky-key.md) or [key toggles](key-toggle.md), see [Hold-Tap](hold-tap.mdx).

:::
