---
title: Layer Lock Behavior
sidebar_label: Layer Lock
---

## Summary

Layer lock toggle allows to switch to a layer and keep it active until the layer lock key is pressed again. To activate layer log, the binding needs to exist on the corresponding layer. Upon activation
(e.g. momentary layer switch) if the key that corresponds to that binding is pressed, the layer stays locked until that key is pressed again.

### Behavior Binding

- Reference: `&layer_lock`, `&llck`

Example:

```dts
&llck
```

### Comparison to QMK

This behavior is analogous to QMK's Layer Lock.
