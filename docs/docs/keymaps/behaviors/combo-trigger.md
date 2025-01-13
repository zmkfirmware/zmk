---
title: Combo Trigger Behavior
sidebar_label: Combo Trigger
---

## Summary

The combo trigger behavior is a special type of behavior that is used together with the [`compatible = "zmk,combos";`](../combos.md) node to add combos to your keymap.

## Combo Trigger

### Configuration

Below is an example of how to implement the combo trigger "Combo or Key Press". When assigned to a key, it will send the combo trigger id of the first parameter passed to it on to the `combos` node. If no combo is triggered, the fallback behavior is triggered using the second parameter passed to the combo trigger.

```dts
/ {
    behaviors {
        combo_kp: combo_trigger_or_key_press {
            compatible = "zmk,behavior-combo-trigger";
            #binding-cells = <2>;
            display-name = "Combo or Key Press";
            fallback-behavior = <&kp>;
        };
    };
};
```

Note that this specific combo trigger behavior exists in ZMK by default using the binding `&combo_kp`.

### Behavior Binding

- Reference: `&combo_kp`
- Parameter: None

Example:

```dts
&combo_kp 1 A
```

When activated, the `combos` node receives the trigger `1`. If no combo is triggered, the activation acts like a `&kp A` instead.

### Fallback Behavior

It is assumed that the behavior in `fallback-behavior` accepts a single parameter as an argument. Hence the behavior should always be given without any arguments. If the behavior accepts no arguments, two parameters should still be passed to the combo trigger:

```dts
&combo_no_param_fallback 1 0
```

If the behavior you wish to have as a fallback-behavior accepts two parameters as arguments, it is recommended that you make use of the [array behavior](array.md).
