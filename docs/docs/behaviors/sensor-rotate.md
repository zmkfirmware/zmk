---
title: Sensor Rotation
sidebar_label: Sensor Rotation
---

## Summary

The Sensor Rotation behavior triggers a different behavior, depending on whether the sensor is rotated clockwise or counter-clockwise.

- If rotated counter-clockwise, the first behavior is triggered.
- If rotated clockwise, the second behavior is triggered.

### Configuration

An example implementation of an encoder that changes RGB brightness is shown below:

```
/ {
    behaviors {
        rgb_encoder: rgb_encoder {
            compatible = "zmk,behavior-sensor-rotate";
            label = "RGB_ENCODER";
            #sensor-binding-cells = <0>;
            bindings = <&rgb_ug RGB_BRD>, <&rgb_ug RGB_BRI>;
        };
    };

    keymap {
        ...
        sensor-bindings = <&rgb_encoder>
    };
};
```
