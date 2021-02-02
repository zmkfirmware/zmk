---
title: Sticky Key Behavior
sidebar_label: Sticky Key
---

## Summary

A sticky key stays pressed until another key is pressed. It is often used for 'sticky shift'. By using a sticky shift, you don't have to hold the shift key to write a capital.

By default, sticky keys stay pressed for a second if you don't press any other key. You can configure this with the `release-after-ms` setting (see below).

### Behavior Binding

- Reference: `&sk`
- Parameter #1: The keycode , e.g. `LSHFT`

Example:

```
&sk LSHFT
```

You can use any keycode that works for `&kp` as parameter to `&sk`:

```
&sk LG(LS(LA(LCTRL)))
```

### Configuration

You can configure a different `release-after-ms` in your keymap:

```
&sk {
    release-after-ms = <2000>;
};

/ {
    keymap {
        ...
    }
}
```

### Advanced usage

Sticky keys can be combined; if you tap `&sk LCTRL` and then `&sk LSHFT` and then `&kp A`, the output will be ctrl+shift+a.

### Comparison to QMK

In QMK, sticky keys are known as 'one shot mods'.
