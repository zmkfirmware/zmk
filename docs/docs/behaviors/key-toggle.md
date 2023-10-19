---
title: Key Toggle Behavior
sidebar_label: Key Toggle
---

## Summary

The key toggle behavior toggles the press of a key.
If the key is not currently pressed, key toggle will press it, holding it until the key toggle is pressed again or the key is released in some other way.
If the key _is_ currently pressed, key toggle will release it.

Example uses for key toggle include shift lock, or `ALT-TAB` window switching without holding down the `ALT` modifier.

### Behavior Binding

- Reference: `&kt`
- Parameter: The [keycode](../codes/index.mdx), e.g. `LALT` or `DOWN_ARROW`

Example:

```dts
&kt LALT
```

You can use any keycode that works for `&kp` as parameter to `&kt`, however, [modified keys](../codes/modifiers.mdx#modifier-functions) such as `LA(A)` will be toggled based on the status of the base keycode (in this case `A`).
In other words, modifiers are ignored when determining whether or not the key is currently pressed.
