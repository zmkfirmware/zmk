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
- Parameter: The [keycode](../list-of-keycodes.mdx), e.g. `LALT` or `DOWN_ARROW`

Example:

```dts
&kt LALT
```

You can use any keycode that works for `&kp` as parameter to `&kt`, however, [modified keys](../modifiers.mdx#modifier-functions) such as `LA(A)` will be toggled based on the status of the base keycode (in this case `A`).
In other words, modifiers are ignored when determining whether or not the key is currently pressed.

### Toggle On and Toggle Off

For state-independent toggles, there exist two further behaviors:

- `&kt_on`: Toggles a keycode on, even if it is already on.
- `&kt_off`: Toggles a keycode off, even if it is already off.

Use these just as you would use `&kt`.
