---
title: Scaler Input Processor
sidebar_label: Scaler
---

## Overview

The scaler input processor is used to scale the value of an input event that has a code matching the codes set on the scaler. Events with other codes will be ignored. Values are scaled by multiplying by the multiplier parameter, and then dividing by the divisor parameter.

## Usage

When used, a scaler takes two parameters that are positive integers, a multiplier and a divisor, e.g.:

```dts
&zip_xy_scaler 2 1
```

which will double all the X/Y movement, or:

```dts
&zip_xy_scaler 1 3
```

which will make movements more granular by reducing the speed to one third.

:::warning

A maximum value of `16` should be used for the multiplier and divisor parameters to avoid overflows.

:::

## Pre-Defined Instances

Three pre-defined instance of the scaler input processor are available:

| Reference            | Description                                   |
| -------------------- | --------------------------------------------- |
| `&zip_xy_scaler`     | Scale X- and Y-axis values by the same amount |
| `&zip_x_scaler`      | Scale X-axis values                           |
| `&zip_y_scaler`      | Scale Y-axis values                           |
| `&zip_scroll_scaler` | Scale wheel and horizontal wheel values       |

## User-Defined Instances

Users can define new instances of the scaler input processor if they want to target different codes.

### Example

```dts
#include <zephyr/dt-bindings/input/input-event-codes.h>

/ {
    input_processors {
        zip_wheel_scaler: zip_wheel_scaler {
            compatible = "zmk,input-processor-scaler";
            #input-processor-cells = <2>;
            type = <INPUT_EV_REL>;
            codes = <INPUT_REL_WHEEL>;
            track-remainders;
        };
    };
}
```

### Compatible

The scaler input processor uses a `compatible` property of `"zmk,input-processor-scaler"`.

### Standard Properties

- `#input-processor-cells` - required to be constant value of `<2>`.
- `track-remainders` - boolean flag that indicates callers should allow the processor to track remainders between events.

### User Properties

- `type` - The [type](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/include/zephyr/dt-bindings/input/input-event-codes.h#L25) of events to scale. Usually, this is `INPUT_EV_REL` for relative events.
- `codes` - The specific codes within the given type to scale, e.g. [relative event codes](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/include/zephyr/dt-bindings/input/input-event-codes.h#L245)
