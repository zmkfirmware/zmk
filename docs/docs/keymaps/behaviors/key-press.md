---
title: Key Press Behaviors
sidebar_label: Key Press
---

## Summary

The most basic of behaviors, is the ability to send certain keycode presses and releases in response to activating
a certain key.

The categories of supported keycodes are:

- [Keyboard](../list-of-keycodes.mdx#keyboard)
- [Modifiers](../list-of-keycodes.mdx#modifiers)
- [Keypad](../list-of-keycodes.mdx#keypad)
- [Editing](../list-of-keycodes.mdx#editing)
- [Media](../list-of-keycodes.mdx#media)
- [Applications](../list-of-keycodes.mdx#applications)
- [Input Assist](../list-of-keycodes.mdx#input-assist)
- [Power & Lock](../list-of-keycodes.mdx#power--lock)

For advanced users, user-defined HID usages are also supported but must be encoded, please see [`dt-bindings/zmk/keys.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/keys.h) for further insight.

## Keycode Defines

To make it easier to encode the HID keycode numeric values, most keymaps include
the [`dt-bindings/zmk/keys.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/keys.h) header
provided by ZMK near the top:

```dts
#include <dt-bindings/zmk/keys.h>
```

Doing so makes a set of defines such as `A`, `N1`, etc. available for use with these behaviors

## Key Press

The "key press" behavior sends standard keycodes on press/release.

### Behavior Binding

- Reference: `&kp`
- Parameter: The keycode usage ID from the usage page, e.g. `N4` or `A`

Example:

```dts
&kp A
```

### Configuration

#### Inverting

By default, ZMK will always send a keycode press event to the host on key press, and a keycode release event on key release.
This is the case even when pressing multiple keys with the same keycode at once, which can cause some unusual interactions.
For example, [toggling a key](./key-toggle.md) on and then pressing and releasing a key with the same keycode will result in the key press cancelling the toggle.

The `invert-if-active` flag allows you to change this interaction. It makes the key press behavior invert its output events.
In other words, if the keycode is pressed on key press, then it releases the keycode. If the keycode is released on key release, then it presses the keycode.

You can set the flag by updating the existing behavior:

```dts
&kp {
    invert-if-active;
};

/ {
    keymap {
        ...
    };
};
```

If you wish to use both versions in the same keymap, you can define a new key press behavior instead:

```
/ {
    behaviors {
        kpi: inverting_key_press {
            compatible = "zmk,behavior-key-press";
            #binding-cells = <1>;
            display-name = "Inverting Key Press";
            invert-if-active;
        };
        ...
    };

    keymap {
        ...
    };
};
```

You would then use the normal behavior with `&kp` and the inverting behavior with `&kpi`.
