---
title: Antecedent-Morph Behavior
sidebar_label: Antecedent-Morph
---

## Summary

The Antecedent-Morph behavior (adaptive keys) sends different behaviors, depending on which key was most recently
released before the antecedent-morph behavior was pressed, if this occurs within a configurable time period.

## Antecedent-Morph

The configuration of the behavior consists of an array of `antecedents`, key codes with implicit modifiers, as well as
of a delay `max-delay-ms` in milli-seconds. If none of the `antecedents` was released during the `max-delay-ms` before
the antecedent-morph behavior is pressed, the behavior invokes the `defaults` binding. If, however, the `n`-th of the
key codes (with implicit modifiers) listed in the array `antecedents` was released within `max-delay-ms`, the behavior
invokes the `n`-th of the bindings of the `bindings` property.

### Configuration

If the key A is assigned the behavior `&ad_a` defined as follows, for example,

```dts
/ {
    behaviors {
        ad_a: adaptive_a {
            compatible = "zmk,behavior-antecedent-morph";
            label = "ADAPTIVE_A";
            #binding-cells = <0>;
			defaults = <&kp A>;
            bindings = <&kp U>, <&kp O>;
			antecedents = <Q Z>;
			max-delay-ms = <250>;
        };
    };
};
```

then by default, pressing this key issues the key press `&kp A`. But if it is preceded within 250 milli-seconds by Q or
Z, then `&kp U` or `&kp O`, respectively, are issued instead.

### Behavior Binding

- Reference: `&ad_a`
- Parameter: None

Example:

```dts
&ad_a
```

### Behaviors

The antecedent-morph behavior can issue not only key press events, but rather arbitrary behaviors.

### Implicit Modifiers

Some special key codes can be obtained only with the help of modifiers. For example, on the U.S. International Keyboard,
the symbol Ü is obtained by holding right Alt and tapping Y. In order to issue this symbol, one can therefore use `&kp
RA(Y)`. Here, right Alt is called an *implicit modifier*. Antecedents are always considered with implicit modifiers. For
example,

```dts
/ {
    behaviors {
        ad_a: adaptive_a {
            compatible = "zmk,behavior-antecedent-morph";
            label = "ADAPTIVE_A";
            #binding-cells = <0>;
			defaults = <&kp A>;
            bindings = <&kp U>, <&kp O>;
			antecedents = <RA(Y) Z>;
			max-delay-ms = <250>;
        };
    };
};
```

replaces the A by a U if preceded by Ü, but not if preceded by Y.

### Explicit Modifiers

The entire function of the antecedent-morph behavior is independent of the *explicit modifiers*. These are the modifiers
that the user is holding down while tapping the antecedent or while pressing the antecedent-morph behavior. In the above
example, A is replaced by O if preceded by Z, regardless of which modifier keys were held down while tapping the Z,
i.e. in particular after a lower case z or after an upper case Z.

Finally, if the user is still holding down a Shift key when pressing the A that triggers the above antecedent-morph
behavior, then this results in an upper case O rather than a lower case o.

### Dead Antecedents

If some binding somewhere issues an illegal key code in the range beyond 0x00ff, this illegal key code is recognized and
tested as an antecedent, but then immediately discarded from the event queue. It therefore functions similarly to a dead
key. Note that these illegal key codes are specified with the *usage page* 0x07 as in the following example. If some key
is bound to `&kp DEAD_ANTE`, then it does not print anything, but still turns a subsequent A into a U.

```dts
#define DEAD_ANTE 0x070100

/ {
    behaviors {
        ad_a: adaptive_a {
            compatible = "zmk,behavior-antecedent-morph";
            label = "ADAPTIVE_A";
            #binding-cells = <0>;
			defaults = <&kp A>;
            bindings = <&kp U>, <&kp O>;
			antecedents = <DEAD_ANTE Z>;
			max-delay-ms = <250>;
        };
    };
};
```
