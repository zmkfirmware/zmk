---
title: Mod-Tap
---

## Summary

The Mod-Tap behavior allows varying the effect of pressing and releasing a key position depending
on whether it is used with other simultaneous key presses at the same time.

If pressed and released independently, the Mod-Tap behavior will send the press and release events
for the configure keycode. If pressed and held while another key is pressed and released, then
the configured modifiers will be applied to that _other_ key press, and no press will be generated
on the release of the Mod-Tap key.

## Mod-Tap

The Mod-Tap behavior either acts as a held modifier, or as a tapped keycode.

### Behavior Binding

- Reference: `&mt`
- Parameter #1: The modifiers to be used when activating as a modifier, e.g. `MOD_LSFT`
- Parameter #2: The keycode to sent when used as a tap, e.g. `A`, `B`.

Example:

```
&mt MOD_LSFT A
```
