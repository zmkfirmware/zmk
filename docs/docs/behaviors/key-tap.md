---
title: Key Tap Behavior
sidebar_label: Key Tap
---

## Summary

The key tap behavior is similar to [key press](key-press.md), but key tap will press _and then immediately release_ the key (even if the physical key remains held down).

All [keycodes](../codes/index.mdx) (including modifiers) can be used the same way as with the [key press](key-press.md) behavior.

:::tip

Key tap is useful if you experience issues when rolling keys that share a base keycode.

For example, a common programming bigram is `+=`. Behind the scenes, `+` is produced with `SHIFT` and `=`. So typing the keys `+` and `=` too quickly can sometimes result in one of them being ignored since the base-key is still pressed. However, by immediately releasing the key with key tap this issue is circumvented.

See [zmkfirmware/zmk #1076](https://github.com/zmkfirmware/zmk/issues/1076) for a thread tracking this.

:::

### Behavior Binding

- Reference: `&kt`
- Parameter: The keycode usage ID from the usage page, e.g. `PLUS` or `A`

Example:

```
&kt LBKT
```
