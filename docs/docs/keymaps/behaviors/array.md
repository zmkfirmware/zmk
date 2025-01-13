---
title: Array of Behaviors
sidebar_label: Array
---

## Summary

An array of behaviors is meant not to be placed in your keymap directly, but rather to simplify the usage of other behaviors such as [hold-tap](hold-tap.mdx), [combo-trigger](combo-trigger.md), or [mod-morph](mod-morph.md).

Invoking an array of behaviors with a particular integer will trigger the behavior at that location in the array (indexed from 0).

## Mod-Morph

### Configuration

Below is an example of how to implement an array with three elements.

```dts
/ {
    arr: behavior_array {
        compatible = "zmk,behavior-array";
        #binding-cells = <1>;
        bindings = <&mt LEFT_CONTROL A &kp B &lt 1 C>;
    };
};
```

The above behavior array could be triggered like so:

```dts
&arr 0
```

The above would act like `&mt LEFT_CONTROL A`.

```dts
&arr 1
```

The above would act like `&kp B`.

```dts
&arr 2
```

The above would act like `&lt 1 C`.

The `&arr X` call could happen in your keymap, but it is more useful if it is used e.g. as the `fallback-behavior` parameter of a [combo-trigger](combo-trigger.md):

```dts
/ {
    behaviors {
        combo_mt: combo_trigger_or_key_press {
            compatible = "zmk,behavior-combo-trigger";
            #binding-cells = <2>;
            display-name = "Combo or Mod Tap";
            fallback-behavior = <&mt_arr>;
        };
        mt_arr: mod_tap_behavior_array {
            compatible = "zmk,behavior-array";
            #binding-cells = <1>;
            bindings = <&mt LEFT_CONTROL A &mt LEFT_CONTROL B &mt LEFT_CONTROL C>;
        };
    };
};
```
