---
title: Key Tap Behavior
sidebar_label: Key Tap
---

## Summary

The key tap behavior is a combination of key press and release on just pressing the key.

All keycodes including modifiers can be used the same way as with the key press behavior (so standard keycodes are sent for press/release).

It is usefull if you want to make a layout that has the same keycode with and without a modifier close together, e.g. `[` and `{`.

Since `LBRC` is basically just `LSHIFT(LBKT)` it can happen, that you still hold `{` while using `[` which leads to a missed keycode since the base-key is still pressed.

By immediately releasing the key with key tap this is circumvented.

### Behavior Binding

- Reference: `&kt`
- Parameter: The keycode usage ID from the usage page, e.g. `LBTK` or `A`

Example:

```
&kt LBKT
```
