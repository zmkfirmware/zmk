---
title: Input Processor Overview
sidebar_label: Overview
---

## Input Processors Overview

"Input processors" are small pieces of functionality that process and optionally modify events generated from emulated and physical pointing devices. Processors can do things like scaling movement values to make them larger or smaller for detailed work, swapping the event types to turn movements into scroll events, or temporarily enabling an extra layer while the pointer is in use.

## Usage

For information on using input processors with a given pointing device, see [input processor usage](usage.md).

## Available Processors

Below is a summary of pre-defined input processors and user-definable input processors available in ZMK, with references to documentation pages describing them.

### Pre-Defined Processors

A set of predefined input processors is available by adding the following at the top of your keymap/overlay file:

```dts
#include <input/processors.dtsi>
```

Once included, you can use the following:

| Binding                    | Processor                                                    | Description                                                              |
| -------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------------------ |
| `&zip_xy_scaler`           | [XY Scaler](scaler.md#pre-defined-instances)                 | Scale the X/Y input events using a multiplier and divisor                |
| `&zip_x_scaler`            | [X Scaler](scaler.md#pre-defined-instances)                  | Scale the X input events using a multiplier and divisor                  |
| `&zip_y_scaler`            | [Y Scaler](scaler.md#pre-defined-instances)                  | Scale the Y input events using a multiplier and divisor                  |
| `&zip_scroll_scaler`       | [Scroll Scaler](scaler.md#pre-defined-instances)             | Scale wheel/horizontal wheel input events using a multiplier and divisor |
| `&zip_xy_transform`        | [XY Transform](transformer.md#pre-defined-instances)         | Transform X/Y values, e.g. inverting or swapping                         |
| `&zip_scroll_transform`    | [Scroll Transform](transformer.md#pre-defined-instances)     | Transform wheel/horizontal wheel values, e.g. inverting or swapping      |
| `&zip_xy_to_scroll_mapper` | [XY To Scroll Mapper](code-mapper.md#pre-defined-instances)  | Map X/Y values to scroll wheel/horizontal wheel events                   |
| `&zip_xy_swap_mapper`      | [XY Swap Mapper](code-mapper.md#pre-defined-instances)       | Swap X/Y values                                                          |
| `&zip_temp_layer`          | [Temporary Layer](temp-layer.md#pre-defined-instances)       | Temporarily enable a layer during pointer use                            |
| `&zip_button_behaviors`    | [Mouse Button Behaviors](behaviors.md#pre-defined-instances) | Trigger behaviors when certain mouse buttons are pressed                 |

### User-Defined Processors

Several of the input processors that have predefined instances, e.g. `&zip_xy_scaler` or `&zip_xy_to_scroll_mapper` can also have new instances created with custom properties around which input codes to scale, or which codes to map, etc.

| Compatible                        | Processor                                            | Description                                         |
| --------------------------------- | ---------------------------------------------------- | --------------------------------------------------- |
| `zmk,input-processor-transform`   | [Transform](transformer.md#user-defined-instances)   | Perform various transforms like inverting values    |
| `zmk,input-processor-code-mapper` | [Code Mapper](code-mapper.md#user-defined-instances) | Map one event code to another type                  |
| `zmk,input-processor-behaviors`   | [Behaviors](behaviors.md#user-defined-instances)     | Trigger behaviors for certain matching input events |

## External Processors

Much like behaviors, custom input processors can also be added to [external modules](../../features/modules.mdx) to allow complete control of the processing operation. See [`input_processor.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/drivers/input_processor.h) for the definition of the driver API.
