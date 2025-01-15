---
title: Layout Configuration
sidebar_label: Layout
---

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Matrix Transform

Defines a mapping from keymap logical positions to physical [kscan](./kscan.md) positions.

You can define multiple matrix transform nodes, one for each layout, and users can select which one they want from the `/chosen` node in their keymaps.

See the [new shield guide](../development/hardware-integration/new-shield.mdx#matrix-transform) for more documentation on how to define a matrix transform.

### Devicetree

Applies to: `compatible = "zmk,matrix-transform"`

Definition file: [zmk/app/dts/bindings/zmk,matrix-transform.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Cmatrix-transform.yaml)

| Property     | Type  | Description                                                           | Default |
| ------------ | ----- | --------------------------------------------------------------------- | ------- |
| `rows`       | int   | Number of rows in the transformed matrix                              |         |
| `columns`    | int   | Number of columns in the transformed matrix                           |         |
| `row-offset` | int   | Adds an offset to all rows before looking them up in the transform    | 0       |
| `col-offset` | int   | Adds an offset to all columns before looking them up in the transform | 0       |
| `map`        | array | A list of position transforms                                         |         |

The `map` array should be defined using the `RC()` macro from [dt-bindings/zmk/matrix_transform.h](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/matrix_transform.h). It should have one item per logical position in the keymap. Each item should list the physical row and column that should trigger the key in that position.

### Example: Skipping Unused Positions

Any keyboard which is not a grid of 1 unit keys will likely have some unused positions in the matrix. A matrix transform can be used to skip the unused positions so users don't have to set them to `&none` in keymaps.

```dts
// numpad.overlay
/ {
    chosen {
        zmk,kscan = &kscan0;
        zmk,matrix-transform = &default_transform;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        // define row-gpios with 5 elements and col-gpios with 4...
    };

    default_transform: matrix_transform {
        compatible = "zmk,matrix-transform";
        rows = <5>;
        columns = <4>;
        // ┌───┬───┬───┬───┐
        // │NUM│ / │ * │ - │
        // ├───┼───┼───┼───┤
        // │ 7 │ 8 │ 9 │ + │
        // ├───┼───┼───┤   │
        // │ 4 │ 5 │ 6 │   │
        // ├───┼───┼───┼───┤
        // │ 1 │ 2 │ 3 │RET│
        // ├───┴───┼───┤   │
        // │ 0     │ . │   │
        // └───────┴───┴───┘
        map = <
            RC(0,0) RC(0,1) RC(0,2) RC(0,3)
            RC(1,0) RC(1,1) RC(1,2) RC(1,3)
            RC(2,0) RC(2,1) RC(2,2)
            RC(3,0) RC(3,1) RC(3,2) RC(3,3)
            RC(4,0)         RC(4,1)
        >;
    };
};
```

```dts
// numpad.keymap
/ {
    keymap {
        compatible = "zmk,keymap";
        default {
            bindings = <
                &kp KP_NUM &kp KP_DIV &kp KP_MULT &kp KP_MINUS
                &kp KP_N7  &kp KP_N8  &kp KP_N9   &kp KP_PLUS
                &kp KP_N4  &kp KP_N5  &kp KP_N6
                &kp KP_N1  &kp KP_N2  &kp KP_N3   &kp KP_ENTER
                &kp KP_N0             &kp KP_DOT
            >;
        };
    }
};
```

### Example: Non-standard Matrix

Consider a keyboard with a [duplex matrix](https://wiki.ai03.com/books/pcb-design/page/matrices-and-duplex-matrix), where the matrix has twice as many rows and half as many columns as the keyboard has keys. A matrix transform can be used to correct for this so that keymaps can match the layout of the keys, not the layout of the matrix.

```dts
/ {
    chosen {
        zmk,kscan = &kscan0;
        zmk,matrix-transform = &default_transform;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        // define row-gpios with 12 elements and col-gpios with 8...
    };

    default_transform: matrix_transform {
        compatible = "zmk,matrix-transform";
        rows = <6>;
        columns = <16>;
        // ESC F1 F2 F3   ...
        // `   1  2  3    ...
        // Tab  Q  W  E   ...
        // Caps  A  S  D  ...
        // Shift  Z  X  C ...
        // Ctrl Alt       ...
        map = <
            RC(0,0) RC(1,0) RC(0,1) RC(1,1)       // ...
            RC(2,0) RC(3,0) RC(2,1) RC(3,1)       // ...
            RC(4,0)   RC(5,0) RC(4,1) RC(5,1)     // ...
            RC(6,0)     RC(7,0) RC(6,1) RC(7,1)   // ...
            RC(8,0)       RC(9,0) RC(8,1) RC(9,1) // ...
            RC(10,0) RC(11,0)                     // ...
        >;
    };
};
```

### Example: Charlieplex

Since a charlieplex driver will never align with a keyboard directly due to the un-addressable positions, a matrix transform should be used to map the pairs to the layout of the keys.
Note that the entire addressable space does not need to be mapped.

```devicetree
/ {
    chosen {
        zmk,kscan = &kscan0;
        zmk,matrix-transform = &default_transform;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-charlieplex";
        wakeup-source;

        interrupt-gpios = <&pro_micro 21 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN) >;
        gpios
          = <&pro_micro 16 GPIO_ACTIVE_HIGH>
          , <&pro_micro 17 GPIO_ACTIVE_HIGH>
          , <&pro_micro 18 GPIO_ACTIVE_HIGH>
          , <&pro_micro 19 GPIO_ACTIVE_HIGH>
          , <&pro_micro 20 GPIO_ACTIVE_HIGH>
          ; // addressable space is 5x5, (minus paired values)
    };

    default_transform: matrix_transform {
        compatible = "zmk,matrix-transform";
        rows = <3>;
        columns = <5>;
        //  Q  W  E  R
        //   A  S  D  F
        //    Z  X  C  V
        map = <
            RC(0,1) RC(0,2) RC(0,3) RC(0,4)
              RC(1,0) RC(1,2) RC(1,3) RC(1,4)
                RC(2,0) RC(2,1) RC(2,3) RC(2,4)
        >;
    };
};
```

## Physical Layout

Defines a keyboard layout by joining together a [matrix transform](#matrix-transform), a [keyboard scan](./kscan.md), and a list of physical key properties.
Multiple physical layouts can be defined for keyboards with multiple physical key layouts.
Read through the [page on physical layouts](../development/hardware-integration/physical-layouts.md) for more information.

### Devicetree

Applies to: `compatible = zmk,physical-layout`

Definition file: [zmk/app/dts/bindings/zmk,physical-layout.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Cphysical-layout.yaml)

| Property       | Type          | Description                                                                                                            | Default |
| -------------- | ------------- | ---------------------------------------------------------------------------------------------------------------------- | ------- |
| `display-name` | string        | The name of this layout, for display purposes                                                                          |         |
| `transform`    | phandle       | The matrix transform to use along with this layout                                                                     |         |
| `kscan`        | phandle       | The kscan to use along with this layout. The `zmk,kscan` chosen will be used as a fallback if this property is omitted |         |
| `keys`         | phandle-array | Array of key physical attributes.                                                                                      |         |

Each element of the `keys` array has the shape `<&key_physical_attrs w h x y r rx ry>`, with the following properties:

| Property   | Type     | Description                          | Unit                                                    |
| ---------- | -------- | ------------------------------------ | ------------------------------------------------------- |
| Width      | int (>0) | Key(cap) width                       | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Height     | int (>0) | Key(cap) height                      | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| X          | uint     | Key X position (top-left point)      | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Y          | uint     | Key Y position (top-left point)      | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Rotation   | int      | Key rotation (positive => clockwise) | [centi-](https://en.wikipedia.org/wiki/Centi-)degree    |
| Rotation X | int      | Rotation origin X position           | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Rotation Y | int      | Rotation origin Y position           | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |

The `key_physical_attrs` node is defined in [`dts/physical_layouts.dtsi`](https://github.com/zmkfirmware/zmk/blob/main/app/dts/physical_layouts.dtsi) and is mandatory.

## Kconfig

| Config                                    | Type | Description                                                   | Default |
| ----------------------------------------- | ---- | ------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_PHYSICAL_LAYOUT_KEY_ROTATION` | bool | Whether to store/support key rotation information internally. | y       |

## Physical Layout Position Map

Defines a mapping between [physical layouts](#physical-layout), allowing key mappings to be preserved in the same locations as previously when using [ZMK Studio](../features/studio.md). Read through the [page on physical layouts](../development/hardware-integration/physical-layouts.md) for more information.

### Devicetree

Applies to: `compatible = zmk,physical-layout-position-map`

Definition file: [zmk/app/dts/bindings/zmk,physical-layout-position-map.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Cphysical-layout-position-map.yaml)

| Property   | Type    | Description                                                                                      | Default |
| ---------- | ------- | ------------------------------------------------------------------------------------------------ | ------- |
| `complete` | boolean | If the mapping complete describes the key mapping, and no position based mapping should be used. |         |

The `zmk,physical-layout-position-map` node should have one child node per physical layout. Each child node should have the following properties:

| Property          | Type    | Description                                                                       | Default |
| ----------------- | ------- | --------------------------------------------------------------------------------- | ------- |
| `physical-layout` | phandle | The physical layout that corresponds to this mapping entry                        |         |
| `positions`       | array   | Array of key positions that match the same array entry in the other sibling nodes |         |
