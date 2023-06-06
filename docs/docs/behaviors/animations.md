---
title: Animation Control Behavior
sidebar_label: Animation Control
---

## Summary

This page contains [Animations](../features/animations.md) behaviors supported by ZMK.

## Animation Control Defines

RGB actions defines are provided through the `dt-bindings/zmk/animation.h` header,
which is added at the top of the keymap file:

```
#include <dt-bindings/zmk/animation.h>
```

This will allow you to reference the actions defined in this header such as
`ANIMATION_TOGGLE`.

Here is a table describing the action for each define:

| Define                               | Action                                                        |
| ------------------------------------ | ------------------------------------------------------------- |
| `ANIMATION_TOGGLE`                   | Toggles active animation                                      |
| `ANIMATION_NEXT`                     | Selects next animation within root animation node array       |
| `ANIMATION_PREVIOUS`                 | Selects previous animation within root animation node array   |
| `ANIMATION_SELECT(x)`                | Selects animation within root animation node at index `x`     |
| `ANIMATION_BRIGHTEN`                 | Increases brightness (applied after other blending)           |
| `ANIMATION_DIM`                      | Decreases brightness (applied after other blending)           |

## Examples

1. Toggle active animation on/off

   ```
   &animation ANIMATION_TOGGLE(0)
   ```

2. Switch to next animation

   ```
   &animation ANIMATION_NEXT(0)
   ```

3. Select animation at index 3

   ```
   &animation ANIMATION_SELECT(3)
   ```

