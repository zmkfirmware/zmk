---
title: Input Processor Usage
sidebar_label: Usage
---

Input processors are used by assigning them to a given [input listener](../../features/pointing.md#input-listeners). A base set of processors is assigned to a listener, and then overrides can be set that are only active when certain [layers](../index.mdx#layers) are active. The examples in the following assume you are adding processors to the `&trackpad` device which is set up with a `&trackpad_listener`.

### Base Processors

Base processors are assigned in the `input-processors` property, and when events are generated, the events are process in the sequence in the order the processors are listed. For example, if you wanted your trackpad to always scale the values to increase the movements, you would assign the [scaler](scaler.md#pre-defined-instances) input processor to the property:

```dts
#include <input/processors.dtsi>

&trackpad_listener {
    input-processors = <&zip_xy_scaler 3 2>;
}
```

### Layer Specific Overrides

Additional overrides can be added that only apply when the associated layer is active. For example, to make the trackpad work as a scroll device when your layer `1` is active, nest a child node on the listener and set the `layers` and `input-processors` properties:

```dts
#include <input/processors.dtsi>

&trackpad_listener {
    input-processors = <&zip_xy_scaler 3 2>;

    scroller {
        layers = <1>;
        input-processors = <&zip_xy_to_scroll_mapper>;
    };
}
```

:::note

Overrides are processed in the order they are declared, from top to bottom, followed by the base processors in the parent node. Their application order is _not_ in any way tied to the layers specified in the `layers` property.

:::

By default, the first-defined override node that matches the layer specification will apply, in which case and any other overrides or the base processors will be skipped. If you add the `process-next;` property to a child node, the other processors will continue to be checked and applied even if that node's layer filter matches.

```dts
#include <input/processors.dtsi>

&trackpad_listener {
    input-processors = <&zip_xy_scaler 3 2>;

    scroller {
        layers = <1>;
        input-processors = <&zip_xy_to_scroll_mapper>;
        process-next;
    };
}
```

For more details, see the [Input Listener configuration](../../config/pointing.md#input-listener) section.
