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

```
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
used to reference the macro in your keymap
:::

The macro can then be bound in your keymap by referencing it by the label `&zed_em_kay`, e.g.:

```
    raise_layer {
        bindings = <&zed_em_kay>;
    };
```

### Bindings

Like [hold-taps](/docs/behaviors/hold-tap), macros are created by composing other behaviors, and any of those behaviors can
be added to the `bindings` list, e.g.:

```
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

```
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

```
bindings
    = <&macro_press &mo 1 &kp LSHFT>
    , <&macro_pause_for_release>
    , <&macro_release &mo 1 &kp LSHFT>
    ;
```

### Wait Time

The wait time setting controls how long of a delay is introduced between behaviors in the `bindings` list. The initial wait time for a macro, 100ms by default, can
be set by assigning a value to the `wait-ms` property of the macro, e.g. `wait-ms = <20>;`. If you want to update the wait time at any
point in the macro bindings list, use `&macro_wait_time`, e.g. `&macro_wait_time 30`. A full example:

```
wait-ms = <10>;
bindings
    = <&kp F &kp A &kp S &kp T>
    , <&macro_wait_time 500>
    , <&kp S &kp L &kp O &kp W>
    ;
```

### Tap Time

The tap time setting controls how long a tapped behavior is held in the `bindings` list. The initial tap time for a macro, 100ms by default, can
be set by assigning a value to the `tap-ms` property of the macro, e.g. `tap-ms = <20>;`. If you want to update the tap time at any
point in a macro bindings list, use `&macro_tap_time`, e.g. `&macro_tap_time 30`. A full example:

```
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

## Common Patterns

Below are some examples of how the macro behavior can be used for various useful functionality.

### Layer Activation + More

Macros make it easy to combine a [layer behavior](/docs/behaviors/layers), e.g. `&mo` with another behavior at the same time.
Common examples are enabling one or more modifiers when the layer is active, or changing the RBG underglow color.

To achieve this, a combination of a 0ms wait time and splitting the press and release between a `&macro_pause_for_release` is used:

#### Layer + Modifier

```
wait-ms = <0>;
bindings
    = <&macro_press &mo 1 &kp LSHFT>
    , <&macro_pause_for_release>
    , <&macro_release &mo 1 &kp LSHFT>;
```

#### Layer + Underglow Color

To trigger a different underglow when the macro is pressed, and when it is released, we use the macro "press" activation mode whenever triggering the `&rgb_ug` behavior:

```
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

```
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

```
wait-ms = <40>;
tap-ms = <40>;
bindings
    = <&macro_press   &kp LALT>
    , <&macro_tap     &kp KP_N0 &kp KP_N1 &kp KP_N6 &kp KP_N3>
    , <&macro_release &kp LALT>
    ;
```

## Convenience C Macro

To avoid repetition or possible typos when declaring a macro, a convenience _C_ macro, named `ZMK_MACRO(name, props)` can be used to simplify things:

```
    ZMK_MACRO(my_macro,
        wait-ms = <30>;
        tap-ms = <40>;
        bindings = <&kp Z &kp M &kp K>;
    )
```

This can be used instead of a complete macro definition. During the firmware build process, the example above would produce the complete macro definition below:

```
    my_macro: my_macro {
        compatible = "zmk,behavior-macro";
        label = "ZM_my_macro";
        #binding-cells = <0>;
        wait-ms = <30>;
        tap-ms = <40>;
        bindings = <&kp Z &kp M &kp K>;
    };
```

Using the C macro is entirely optional, and is provided only as a convenience.
