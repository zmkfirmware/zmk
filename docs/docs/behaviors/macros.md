---
title: Macro Behavior
sidebar_label: Macros
---

## Summary

The macro behavior allows configuring a list of other behaviors to invoke
when the macro is pressed and/or released.

## Macro Definition

Each macro you want to use in your keymap gets defined first, then bound in your keymap.

A macro definition looks like:

```dts
/ {
    macros {
        zed_em_kay: zed_em_kay {
            label = "ZM_zed_em_kay";
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings
                = <&macro_press &kp LSHFT>
                , <&macro_tap &kp Z &kp M &kp K>
                , <&macro_release &kp LSHFT>
                ;
        };
    };
};
```

:::note
The text before the colon (`:`) in the declaration of the macro node is the "node label", and is the text
used to reference the macro in your keymap.
:::

The macro can then be bound in your keymap by referencing it by the label `&zed_em_kay`, e.g.:

```dts
    raise_layer {
        bindings = <&zed_em_kay>;
    };
```

:::note
For use cases involving sending a single keycode with modifiers, for instance ctrl+tab, the [key press behavior](key-press.md)
with [modifier functions](../codes/modifiers.mdx#modifier-functions) can be used instead of a macro.
:::

### Bindings

Like [hold-taps](/docs/behaviors/hold-tap), macros are created by composing other behaviors, and any of those behaviors can
be added to the `bindings` list, e.g.:

```dts
bindings
    = <&to 1>
    , <&bl BL_ON>
    , <&kp Z &kp M &kp K &kp EXCLAMATION>
    ;
```

## Macro Controls

There are a set of special macro controls that can be included in the `bindings` list to modify the
way the macro is processed.

### Binding Activation Mode

Bindings in a macro are activated differently, depending on the current "activation mode" of the macro.

Available modes:

- Tap - The default mode; when in this mode, the macro will press, then release, each behavior in the `bindings` list. This mode is useful for
  basic keycode output to hosts, i.e. when activating a `&kp` behavior.
- Press - In this mode, the macro will only trigger a press on each behavior in the `bindings` list. This is useful for holding down modifiers for some duration of a macro, e.g. `&kp LALT`.
- Release - In this mode, the macro will only trigger a release on each behavior in the `bindings` list. This is useful for releasing modifiers previously pressed earlier in the macro processing, e.g. `&kp LALT`.

To modify the activation mode, macro controls can be added at any point in the `bindings` list.

- `&macro_tap`
- `&macro_press`
- `&macro_release`

A concrete example, used to hold a modifier, tap multiple keys, then release the modifier, would look like:

```dts
bindings
    = <&macro_press &kp LSHFT>
    , <&macro_tap &kp Z &kp M &kp K>
    , <&macro_release &kp LSHFT>
    ;
```

### Processing Continuation on Release

The macro can be paused so that only part of the `bindings` list is processed when the macro is pressed, and the remainder is processed once
the macro itself is released.

To pause the macro until release, use `&macro_pause_for_release`. For example, this macro will press a modifier and activate a layer when the macro is pressed. Once the macro is released, it will release the modifier and deactivate the layer by releasing the `&mo`:

```dts
bindings
    = <&macro_press &mo 1 &kp LSHFT>
    , <&macro_pause_for_release>
    , <&macro_release &mo 1 &kp LSHFT>
    ;
```

### Wait Time

The wait time setting controls how long of a delay is introduced between behaviors in the `bindings` list. The initial wait time for a macro,
which is equal to the value of [`CONFIG_ZMK_MACRO_DEFAULT_WAIT_MS`](../config/behaviors.md#macro) by default, can
be set by assigning a value to the `wait-ms` property of the macro, e.g. `wait-ms = <20>;`. If you want to update the wait time at any
point in the macro bindings list, use `&macro_wait_time`, e.g. `&macro_wait_time 30`. A full example:

```dts
wait-ms = <10>;
bindings
    = <&kp F &kp A &kp S &kp T>
    , <&macro_wait_time 500>
    , <&kp S &kp L &kp O &kp W>
    ;
```

### Tap Time

The tap time setting controls how long a tapped behavior is held in the `bindings` list. The initial tap time for a macro,
which is equal to the value of [`CONFIG_ZMK_MACRO_DEFAULT_TAP_MS`](../config/behaviors.md#macro) by default, can
be set by assigning a value to the `tap-ms` property of the macro, e.g. `tap-ms = <20>;`. If you want to update the tap time at any
point in a macro bindings list, use `&macro_tap_time`, e.g. `&macro_tap_time 30`. A full example:

```dts
bindings
    = <&macro_tap_time 10>
    , <&kp S &kp H &kp O &kp R &kp T>
    , <&macro_tap_time 500>
    , <&kp L &kp O &kp N &kp G>
    ;
```

### Behavior Queue Limit

Macros use an internal queue to invoke each behavior in the bindings list when triggered, which has a size of 64 by default. Bindings in "press" and "release" modes correspond to one event in the queue, whereas "tap" mode bindings correspond to two (one for press and one for release). As a result, the effective number of actions processed might be less than 64 and this can cause problems for long macros.

To prevent issues with longer macros, you can change the size of this queue via the `CONFIG_ZMK_BEHAVIORS_QUEUE_SIZE` setting in your configuration, [typically through your `.conf` file](../config/index.md). For example, `CONFIG_ZMK_BEHAVIORS_QUEUE_SIZE=512` would allow your macro to type about 256 characters.

Another limit worth noting is that the maximum number of bindings you can pass to a `bindings` field in the [Devicetree](../config/index.md#devicetree-files) is 256, which also constrains how many behaviors can be invoked by a macro.

## Parameterized Macros

Macros can also be "parameterized", allowing them to be bound in your keymap with unique values passed into them, e.g.:

```dts
    raise_layer {
        bindings = <&my_one_param_macro A>
    };
```

### Defining Parameterized Macros

Parameterized macros must be defined using specific values for the `compatible` and `#binding-cells` properties, depending on how many parameters they require (up to a maximum of two):

```dts
/ {
    macros {
        // 0 params macro
        my_macro: my_macro {
            // ...
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>; // Must be 0
            bindings = /* ... */;
        };

        // 1 param macro
        my_one_param_macro: my_one_param_macro {
            // ...
            compatible = "zmk,behavior-macro-one-param";
            #binding-cells = <1>; // Must be 1
            bindings = /* ... */;
        };

        // 2 params macro
        my_two_param_macro: my_two_param_macro {
            // ...
            compatible = "zmk,behavior-macro-two-param";
            #binding-cells = <2>; // Must be 2
            bindings = /* ... */;
        };
    };
};
```

### Parameters, Bindings and Controls

There are special macro controls which must be used in order to forward received parameters to the macro's `bindings`. These controls are "one shot" and will determine how received parameters are used on the very next (non-macro control) behavior in the macro's `bindings` list.

For example, to pass the first parameter received into a `&kp` binding, you would use:

```dts
bindings = <&macro_param_1to1>, <&kp MACRO_PLACEHOLDER>;
```

Because `kp` takes one parameter, you can't simply make the second entry `<&kp>` in the `bindings` list. Whatever value you do pass in will be replaced when the macro is triggered, so you can put _any_ value there, e.g. `0`, `A` keycode, etc. To make it very obvious that the parameter there is not actually going to be used, you can use `MACRO_PLACEHOLDER` which is simply an alias for `0`.

The available parameter controls are:

- `&macro_param_1to1` - pass the first parameter of the macro into the first parameter of the next behavior in the `bindings` list.
- `&macro_param_1to2` - pass the first parameter of the macro into the second parameter of the next behavior in the `bindings` list.

* `&macro_param_2to1` - pass the second parameter of the macro into the first parameter of the next behavior in the `bindings` list.
* `&macro_param_2to2` - pass the second parameter of the macro into the second parameter of the next behavior in the `bindings` list.

## Common Patterns

Below are some examples of how the macro behavior can be used for various useful functionality.

### Layer Activation + More

Macros make it easy to combine a [layer behavior](/docs/behaviors/layers), e.g. `&mo` with another behavior at the same time.
Common examples are enabling one or more modifiers when the layer is active, or changing the RBG underglow color.

To achieve this, a combination of a 0ms wait time and splitting the press and release between a `&macro_pause_for_release` is used:

#### Layer + Modifier

```dts
/**
 * Temporarily switches to a layer (`&mo`) while a modifier is held.
 * Analogous to QMK's `LM()`, using a parameterized macro.
 *
 * Params:
 *  1. Layer to switch to
 *  2. Modifier to press while layer is active
 *
 * Example:
 *  `&lm NUM_LAYER LSHIFT`
 */
lm: lm {
    label = "LAYER_MOD";
    compatible = "zmk,behavior-macro-two-param";
    wait-ms = <0>;
    tap-ms = <0>;
    #binding-cells = <2>;
    bindings
        = <&macro_param_1to1>
        , <&macro_press &mo MACRO_PLACEHOLDER>
        , <&macro_param_2to1>
        , <&macro_press &kp MACRO_PLACEHOLDER>
        , <&macro_pause_for_release>
        , <&macro_param_2to1>
        , <&macro_release &kp MACRO_PLACEHOLDER>
        , <&macro_param_1to1>
        , <&macro_release &mo MACRO_PLACEHOLDER>
        ;
};
```

#### Layer + Underglow Color

To trigger a different underglow when the macro is pressed, and when it is released, we use the macro "press" activation mode whenever triggering the `&rgb_ug` behavior:

```dts
wait-ms = <0>;
tap-ms = <0>;
bindings
    = <&macro_press &mo 1>
    , <&macro_tap &rgb_ug RGB_COLOR_HSB(128,100,100)>
    , <&macro_pause_for_release>
    , <&macro_release &mo 1>
    , <&macro_tap &rgb_ug RGB_COLOR_HSB(300,100,50)>;
```

### Keycode Sequences

The other common use case for macros is to sending sequences of keycodes to the connected host. Here, a wait and tap time of at least 30ms is recommended to
avoid having HID notifications grouped at the BLE protocol level and then processed out of order:

```dts
wait-ms = <40>;
tap-ms = <40>;
bindings
    = <&kp Z &kp M &kp K>
    , <&kp SPACE>
    , <&kp R &kp O &kp C &kp K &kp S>
    ;
```

### Unicode Sequences

Many operating systems allow a special sequence to input unicode characters, e.g. [Windows alt codes](https://support.microsoft.com/en-us/office/insert-ascii-or-unicode-latin-based-symbols-and-characters-d13f58d3-7bcb-44a7-a4d5-972ee12e50e0). You can use macros to automate inputting the sequences, e.g. below macro inserts `£` on Windows:

```dts
wait-ms = <40>;
tap-ms = <40>;
bindings
    = <&macro_press   &kp LALT>
    , <&macro_tap     &kp KP_N0 &kp KP_N1 &kp KP_N6 &kp KP_N3>
    , <&macro_release &kp LALT>
    ;
```

## Convenience C Macro

To avoid repetition or possible typos when declaring a **zero parameter macro**, a convenience _C_ macro, named `ZMK_MACRO(name, props)` can be used to simplify things:

```dts
    ZMK_MACRO(my_zero_param_macro,
        wait-ms = <30>;
        tap-ms = <40>;
        bindings = <&kp Z &kp M &kp K>;
    )
```

:::note
`ZMK_MACRO()` **only supports declaring non-parameterized (zero parameter) macros**; parameterized declarations are not currently supported.
:::

This can be used instead of a complete macro definition. During the firmware build process, the example above would produce the complete macro definition below:

```dts
    my_zero_param_macro: my_zero_param_macro {
        compatible = "zmk,behavior-macro";
        label = "ZM_my_macro";
        #binding-cells = <0>;
        wait-ms = <30>;
        tap-ms = <40>;
        bindings = <&kp Z &kp M &kp K>;
    };
```

Using the C macro is entirely optional, and is provided only as a convenience.
