---
title: Sticky Layer Behavior
sidebar_label: Sticky Layer
---

## Summary

A sticky layer stays pressed until another key is pressed. By using a sticky layer, you don't have to hold the layer key to access a layer. Instead, tap the sticky layer key to activate the layer until the next keypress.

By default, sticky layers stay pressed for a second if you don't press any other key. You can configure this with the `release-after-ms` setting (see below).

### Behavior Binding

- Reference: `&sl`
- Parameter #1: The layer to activate , e.g. `1`

Example:

```
&sl 1
```

### Configuration

You can configure a different `release-after-ms` in your keymap:

```
&sl {
    release-after-ms = <2000>;
};

/ {
    keymap {
        ...
    }
}
```

### Advanced usage

Sticky layers behave slightly different from sticky keys. They are configured to `quick-release`. This means that the layer is released immediately when another key is pressed. "Normal" sticky keys are not `quick-release`; they are released when the next key is released. This makes it possible to combine sticky layers and sticky keys as such: tap `&sl 1`, tap `&sk LSHFT` on layer 1, tap `&kp A` on base layer to output shift+A.

### Comparison to QMK

In QMK, sticky layers are known as 'one shot layers'.
