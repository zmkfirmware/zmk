---
title: Mod-Tap Behavior
sidebar_label: Mod-Tap
---

## Summary

The Mod-Tap sends a different keypress, if it's tapped or held. When you tap the key shortly, the first keycode is sent. If you hold the key for longer than 200ms, the second keycode is sent.

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
    tapping_term_ms = <400>;
};

/ {
    keymap {
        ...
    }
}
```
