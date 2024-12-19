---
title: Transformer Input Processor
sidebar_label: Transformer
---

## Overview

The transformer input processor is used to perform various transforms on the value of an input event that has a code matching the codes set on the transformer. Events with other codes will be ignored.

## Available Transforms

The following transforms are available, by including
the [`dt-bindings/zmk/input_transform.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/input_transform.h) header
provided by ZMK near the top of your keymap/overlay:

```dts
#include <dt-bindings/zmk/input_transform.h>
```

- `INPUT_TRANSFORM_XY_SWAP` - When encountering a value with matching type, swap the type of the event to the other axis, e.g. change an event of type `INPUT_REL_X` to type `INPUT_REL_Y`.
- `INPUT_TRANSFORM_X_INVERT` - Invert the values of any events that match the configured `x-codes` of the processor, by multiplying by negative one.
- `INPUT_TRANSFORM_Y_INVERT` - Invert the values of any events that match the configured `y-codes` of the processor, by multiplying by negative one.

## Usage

When used, a transformer takes one parameter, a combination of flags indicating which transforms to apply:

```dts
&zip_xy_transform (INPUT_TRANSFORM_X_INVERT | INPUT_TRANSFORM_Y_INVERT)
```

## Pre-Defined Instances

Three pre-defined instance of the scaler input processor are available:

| Reference               | Description                                                   |
| ----------------------- | ------------------------------------------------------------- |
| `&zip_xy_transform`     | Applies the given transforms to X/Y movement events           |
| `&zip_scroll_transform` | Applies the given transforms to wheel/horizontal wheel events |

## User Defined Instances

Users can define new instances of the transform input processor if they want to target different codes.

### Example

```dts
#include <zephyr/dt-bindings/input/input-event-codes.h>

/ {
    input_processors {
        my_rotation_event_transform: my_rotation_event_transform {
            compatible = "zmk,input-processor-transform";
            #input-processor-cells = <1>;
            type = <INPUT_EV_REL>;
            x-codes = <INPUT_REL_RX>;
            y-codes = <INPUT_REL_RY>;
        };
    };
}
```

### Compatible

The transform input processor uses a `compatible` property of `"zmk,input-processor-transform"`.

### Standard Properties

- `#input-processor-cells` - required to be constant value of `<1>`.

### User Properties

- `type` - The [type](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/include/zephyr/dt-bindings/input/input-event-codes.h#L25) of events to transform. Usually, this is `INPUT_EV_REL` for relative events.
- `x-codes` - The specific X codes within the given type to transform, e.g. [relative event codes](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/include/zephyr/dt-bindings/input/input-event-codes.h#L245)
- `y-codes` - The specific Y codes within the given type to transform, e.g. [relative event codes](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/include/zephyr/dt-bindings/input/input-event-codes.h#L245)
