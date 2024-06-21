---
title: Turbo Behavior
sidebar_label: Turbo Key
---

## Summary

The turbo behavior will repeatedly trigger a behavior after a specified amount of time.

### Configuration

An example of how to implement a turbo key to output `A` every 5 seconds:

```
/ {
    behaviors {
        turbo_A: turbo_A {
            compatible = "zmk,behavior-turbo-key";
            label = "TURBO_A";
            #binding-cells = <0>;
            bindings = <&kp A>;
            wait-ms = <5000>;
        };
    };
};
```

### Behavior Binding

- Reference: `&turbo_A`
- Parameter: None

Example:

```
&turbo_A
```

### Advanced Configuration

#### `wait-ms`

Defines how often the behavior will trigger. Defaults to 200ms.

#### `tap-ms`

Defines how long the behavior will be held for each press. Defaults to 5ms.

#### `toggle-term-ms`

Releasing the turbo key within `toggle-term-ms` will toggle the repeating, removing the need to hold down the key.
