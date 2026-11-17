---
title: Miscellaneous Behaviors
sidebar_label: Miscellaneous
---

## Summary

There are a few miscellaneous behaviors that are helpful when working with layers in keymaps,
in particular, with handling what happens in higher layers, and if events are passed to
the next layer or not

## Transparent

The transparent behavior simply ignores key position presses/releases, so they will be
passed down to the next active layer in the stack.

### Behavior Binding

- Reference: `&trans`
- Parameters: None

Example:

```dts
&trans
```

## None

The none behavior simply swallows and stops key position presses/releases, so they will **not**
be passed down to the next active layer in the stack.

### Behavior Binding

- Reference: `&none`
- Parameters: None

Example:

```dts
&none
```
