---
title: Sticky Key Behavior
sidebar_label: Sticky Key
---

## Summary

A sticky key stays pressed until another key is pressed. It is often used for 'sticky shift'. By using a sticky shift, you don't have to hold the shift key to write a capital.

By default, sticky keys stay pressed for a second if you don't press any other key. You can configure this with the `release-after-ms` setting (see below).

Some typists may find that using a sticky shift key interspersed with rapid typing results in two or more capitalized letters instead of one. This happens as the sticky key is active until the next key is released, under which other keys may be pressed and will receive the modifier. You can change this with the `quick-release` setting (see below) which will instead deactivate the sticky key on the next key being pressed, as opposed to released.

### Behavior Binding

- Reference: `&sk`
- Parameter #1: The keycode , e.g. `LSHIFT`

Example:

```
&sk LSHIFT
```

You can use any keycode that works for `&kp` as parameter to `&sk`:

```
&sk LG(LS(LA(LCTRL)))
```

### Configuration

You can configure a different `release-after-ms` or enable `quick-release` in your keymap:

```
&sk {
    release-after-ms = <2000>;
    quick-release;
};

/ {
    keymap {
        ...
    };
};
```

This configuration would apply to all sticky keys. This may not be appropriate if using `quick-release` as you'll lose the ability to chain sticky key modifiers. A better approach for this use case would be to create a new behavior:

```
/ {
    behaviors {
      skq: sticky_key_quick_release {
        compatible = "zmk,behavior-sticky-key";
        label = "STICKY_KEY_QUICK_RELEASE";
        #binding-cells = <1>;
        bindings = <&kp>;
        release-after-ms = <1000>;
        quick-release;
      };
    };

    keymap {
        ...
    };
};
```

### Advanced usage

Sticky keys can be combined; if you tap `&sk LCTRL` and then `&sk LSHIFT` and then `&kp A`, the output will be ctrl+shift+a.

### Comparison to QMK

In QMK, sticky keys are known as 'one shot mods'.
