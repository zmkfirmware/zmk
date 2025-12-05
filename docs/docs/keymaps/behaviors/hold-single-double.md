---
title: Hold / Single / Double Behavior
slug: /docs/behaviors/hold-single-double
---

This behavior provides three actions bound to a single key:

- Hold: when the key is held longer than `tapping-term-ms` (default 200ms), the "hold" binding triggers.
- Single: a single quick tap triggers the "single" binding.
- Double: two quick taps within the tapping term triggers the "double" binding.

Device-tree example:

```dts
&behaviors {
    hsd: behavior_hold_single_double {
        compatible = "zmk,behavior-hold-single-double";
        tapping-term-ms = <200>;
        bindings = <&kp A>, <&kp B>, <&kp C>;
    };
};
```

See also: `Hold Tap` and `Tap Dance` behaviors for related patterns.
