---
title: Temporary Layer Input Processor
sidebar_label: Temporary Layer
---

## Overview

The temporary layer input processor is used to enable a layer when input events are received, and automatically disable it when no further events are received in the given timeout duration. This most frequently is used to temporarily enable a layer with a set of [mouse button emulation behaviors](../behaviors/mouse-emulation.md#mouse-button-press) on it, so you can press various mouse buttons with the normal keyboard keys while using a physical pointer device for X/Y movement.

## Usage

When used, the temporary layer input processor takes two parameters, the layer index to enable and a timeout value in milliseconds:

```dts
&zip_temp_layer 2 2000
```

Above example enables the third layer and automatically disables it again after 2 seconds with no events from this pointing device.

## Pre-Defined Instances

One pre-defined instance of the temporary layer input processor is available:

| Reference         | Description                                                     |
| ----------------- | --------------------------------------------------------------- |
| `&zip_temp_layer` | Enable a certain layer temporarily until no events are received |

## User-Defined Instances

Users can define new instances of the temporary layer input processor to use different settings.

### Example

```dts
#include <zephyr/dt-bindings/input/input-event-codes.h>

/ {
    /omit-if-no-ref/ zip_temp_layer: zip_temp_layer {
        compatible = "zmk,input-processor-temp-layer";
        #input-processor-cells = <2>;
        require-prior-idle-ms = <2000>;
        excluded-positions = <1 2 3>;
    };
};
```

### Compatible

The temp layer input processor uses a `compatible` property of `"zmk,input-processor-temp-layer"`.

### Standard Properties

- `#input-processor-cells` - required to be constant value of `<2>`.

### User Properties

- `require-prior-idle-ms` - Only activate the layer if there have not been any key presses for at least the set number of milliseconds before the pointing device event
- `excluded-positions` - List of (zero-based) key positions to exclude from deactivating the layer once it is active.
