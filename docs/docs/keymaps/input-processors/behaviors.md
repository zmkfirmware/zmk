---
title: Behaviors Input Processor
sidebar_label: Behaviors
---

## Overview

The behaviors input processor is used invoke standard behaviors when certain input events occur; most frequently this is used to trigger behaviors when certain mouse buttons are triggered by physical pointing devices.

:::note

This input processor is primarily intended for `INPUT_EV_KEY` type of events that have a binary on/off state, not vector types for relative or absolute movements.

:::

:::note[Source-specific behaviors on split keyboards]
Invoking a [source-specific behavior](../../features/split-keyboards.md#source-locality-behaviors) such as one of the [reset behaviors](../behaviors/reset.md) using this input processor will always trigger it on the central side of the keyboard, regardless of the side includes the input device that originally generated the input event.
:::

## Usage

When used, this input processor takes no parameters, as the event code to behavior mapping is specified in the definition of the specific processor instance, e.g.:

```dts
&zip_button_behaviors
```

## Pre-Defined Instances

One pre-defined instance of the out-of-band behaviors input processor is available:

| Reference               | Description                                        |
| ----------------------- | -------------------------------------------------- |
| `&zip_button_behaviors` | Maps left/right/middle clicks to a given behavior. |

Should you wish to update the existing instance to trigger different behaviors for each mouse button, you can override the `bindings` property, e.g.:

```dts
&zip_button_behaviors {
    bindings = <&kp A &kp B &kp C>;
};
```

By default, the `bindings` property maps all the buttons to [`&none`](../behaviors/misc.md#none), so you will want to override the `bindings` property like above if you use this processor by assigning it to an [input listener](usage.md).

## User-Defined Instances

Users can define new instances of the out-of-band behaviors input processor if they want to target different codes or assign different behaviors.

### Example

Below example maps the left mouse button code to the middle mouse button.

```dts
#include <zephyr/dt-bindings/input/input-event-codes.h>

/ {
    input_processors {
        zip_right_click_trigger_paste: zip_right_click_trigger_paste {
            compatible = "zmk,input-processor-behaviors";
            #input-processor-cells = <0>;
            codes    = <INPUT_BTN_1>;
            bindings = <&kp LC(V)  >;
        };
    };
}
```

### Compatible

The behaviors input processor uses a `compatible` property of `"zmk,input-processor-behaviors"`.

### Standard Properties

- `#input-processor-cells` - required to be constant value of `<0>`.

### User Properties

- `type` - The [type](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/include/zephyr/dt-bindings/input/input-event-codes.h#L25) of events to scale. Usually, this is `INPUT_EV_KEY` for key/button events. The default value if omitted is `INPUT_EV_KEY`.
- `codes` - The specific codes of the given type to capture, e.g. [button event codes](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/include/zephyr/dt-bindings/input/input-event-codes.h#L180). This list must be the same length as the `bindings` property.
- `bindings` - The bindings to trigger when an event with the corresponding code is processed.
