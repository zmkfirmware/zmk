---
title: Code Mapper Input Processor
sidebar_label: Code Mapper
---

## Overview

The code mapper input processor is used to map the code of an event to a new one, e.g. changing a vertical Y movement event into a scroll event.

## Usage

When used, a code mapper takes no parameters, as the code mappings are specified in the definition of the specific mapper instance, e.g.:

```dts
&zip_xy_to_scroll_mapper
```

## Pre-Defined Instances

Three pre-defined instance of the code mapper input processor are available:

| Reference                  | Description                                                             |
| -------------------------- | ----------------------------------------------------------------------- |
| `&zip_xy_to_scroll_mapper` | Map X/Y movement events to horizontal wheel/wheel events, respectively. |
| `&zip_xy_swap_mapper`      | Map X to Y, and Y to X for movements.                                   |

Note that swapping X and Y movements can also be accomplished with the [transformer](transformer.md#pre-defined-instances) processors.

## User-Defined Instances

Users can define new instances of the code mapper input processor if they want to target different codes.

### Example

Below example maps the left mouse button code to the middle mouse button.

```dts
#include <zephyr/dt-bindings/input/input-event-codes.h>

/ {
    input_processors {
        zip_click_to_middle_click_mapper: zip_click_to_middle_click_mapper {
            compatible = "zmk,input-processor-code-mapper";
            #input-processor-cells = <0>;
            type = <INPUT_EV_KEY>;
            map = <INPUT_BTN_0 INPUT_BTN_2>;
        };
    };
}
```

### Compatible

The code mapper input processor uses a `compatible` property of `"zmk,input-processor-code-mapper"`.

### Standard Properties

- `#input-processor-cells` - required to be constant value of `<0>`.

### User Properties

- `type` - The [type](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/include/zephyr/dt-bindings/input/input-event-codes.h#L25) of events to scale. Usually, this is `INPUT_EV_REL` for relative events and `INPUT_EV_KEY` for key/button events.
- `map` - The specific codes of the given type to map, e.g. [relative event codes](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/include/zephyr/dt-bindings/input/input-event-codes.h#L245). This list must be an even number of entries which is processed as a list of pairs of codes. The first code in the pair is the source code, and the second is the code to map it to.
