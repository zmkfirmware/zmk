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
- Parameter #1: The keycode to be sent when activating as a modifier, e.g. `LSHFT`
- Parameter #2: The keycode to sent when used as a tap, e.g. `A`, `B`.

Example:

```
&mt LSHFT A
```

### Configuration

You can configure a different tapping term in your keymap:

```
&mt {
    tapping-term-ms = <400>;
};

/ {
    keymap {
        ...
    }
}
```

### Additional information

The mod-tap is a [hold-tap](./hold-tap.md) under the hood with the "balanced" flavor and tapping-term-ms 200.
