---
title: Physical Layouts
---

A physical layout is a devicetree entity that aggregates all details about a certain possible keyboard layout.
It contains:

- A [keyboard scan (kscan) driver](../../config/kscan.md)
- A [matrix transform](../../config/layout.md#matrix-transform)
- (Optional) [Physical key positions](#physical-layout-positions)

## Basic Physical Layout

A basic physical layout without the `keys` property looks like this:

```dts
/ {
    default_layout: default_layout {
        compatible = "zmk,physical-layout";
        display-name = "Default Layout";
        transform = <&default_transform>;
        kscan = <&kscan0>;
    };
};
```

It is given a name, a matrix transform, and a kscan. If all of your physical layouts share the same kscan, then the `kscan` property can be omitted - in this case it needs to be set in the [`chosen` node](./new-shield.mdx#chosen-node). See the [configuration section on physical layouts](../../config/index.md) for reference.

## (Optional) Keys Property

:::warning[Alpha Feature]

[ZMK Studio](../../features/studio.md) support is in alpha. Although best efforts are being made, backwards compatibility during active development is not guaranteed.

:::

The `keys` property is required for [ZMK Studio](../../features/studio.md) support. It is used to describe the physical attributes of each key position present in that layout.
To pull in the necessary definition for creating physical layouts with the `keys` property, a new include should be added to the top of the devicetree file:

```
#include <physical_layouts.dtsi>
```

Assigned to the `keys` property is an array of key descriptions listed in the same order as keymap bindings, matrix transforms, etc.
A key description has the shape `<&key_physical_attrs w h x y r rx ry>` with the following properties:

| Property   | Type     | Description                          | Unit                                                    |
| ---------- | -------- | ------------------------------------ | ------------------------------------------------------- |
| Width      | int (>0) | Key(cap) width                       | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Height     | int (>0) | Key(cap) height                      | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| X          | uint     | Key X position (top-left point)      | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Y          | uint     | Key Y position (top-left point)      | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Rotation   | int      | Key rotation (positive => clockwise) | [centi-](https://en.wikipedia.org/wiki/Centi-)degree    |
| Rotation X | int      | Rotation origin X position           | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Rotation Y | int      | Rotation origin Y position           | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |

:::tip
You can specify negative values in devicetree using parentheses around it, e.g. `(-3000)` for a 30 degree counterclockwise rotation.
:::

### Physical Layout with Keys Example

Here is an example of a physical layout for a 2x2 macropad:

```dts
#include <physical_layouts.dtsi>

/ {
    macropad_physical_layout: macropad_physical_layout {
        compatible = "zmk,physical-layout";
        display-name = "Macro Pad";
        transform = <&default_transform>;
        kscan = <&kscan0>;
        keys  //                     w   h    x    y     rot    rx    ry
            = <&key_physical_attrs 100 100    0    0       0     0     0>
            , <&key_physical_attrs 100 100  100    0       0     0     0>
            , <&key_physical_attrs 100 100    0  100       0     0     0>
            , <&key_physical_attrs 100 100  100  100       0     0     0>
            ;
    };
};
```

## Using Predefined Layouts

ZMK defines a number of popular physical layouts in-tree at [`app/dts/layouts`](https://github.com/zmkfirmware/zmk/tree/main/app/dts/layouts).
To use such layouts, import them and assign their `transform` and (optionally) `kscan` properties.

Here is an example of using the predefined physical layouts for a keyboard with the same layout as the "ferris":

```dts
#include <layouts/cuddlykeyboards/ferris.dtsi>

// Assigning suitable kscan and matrix transforms
&cuddlykeyboards_ferris_layout {
    transform = <&default_transform>;
    kscan = <&kscan0>;
};
```

Shared physical layouts found in the same folder are defined such that they can be used together, to define [multiple physical layout](#multiple-physical-layouts) options. See below for more information on multiple physical layouts.

Here is an example of using the predefined physical layouts for a 60% keyboard:

```dts
#include <layouts/common/60percent/all1u.dtsi>
#include <layouts/common/60percent/ansi.dtsi>
#include <layouts/common/60percent/hhkb.dtsi>
#include <layouts/common/60percent/iso.dtsi>


// Assigning suitable kscan and matrix transforms
&layout_60_ansi {
    transform = <&ansi_transform>;
    kscan = <&ansi_kscan>;
};

&layout_60_iso {
    transform = <&iso_transform>;
    kscan = <&iso_kscan>;
};

&layout_60_all1u {
    transform = <&all_1u_transform>;
    kscan = <&all_1u_kscan>;
};

&layout_60_hhkb {
    transform = <&hhkb_transform>;
    kscan = <&hhkb_kscan>;
};
```

## Multiple Physical Layouts

If a keyboard has multiple possible layouts (e.g. you can snap off an outer column), then you should define multiple matrix transformations and multiple physical layouts, one for each possible layout.
If necessary, you can also define multiple kscan instances.

```dts
// Needed if and only if keys property is used
#include <physical_layouts.dtsi>

/ {
    default_layout: default_layout {
        compatible = "zmk,physical-layout";
        display-name = "Default Layout";
        transform = <&default_transform>;
        kscan = <&kscan0>;
        keys = <...>; // List of key positions, optional
    };

    alt_layout: alt_layout {
        compatible = "zmk,physical-layout";
        display-name = "Alternate Layout";
        transform = <&alt_transform>;
        kscan = <&alt_kscan0>;
        keys = <...>; // List of key positions, optional
    };
};
```

### Position Map

A position map is used for precise mapping between layouts. It is used to allow [ZMK Studio](../../features/studio.md) to accurately retain keymaps when switching layouts. A keyboard's position map has a child node for every potential layout of the keyboard, with the corresponding entries in `positions` property will be used to determine the mapping between layouts. A position map looks something like this:

```dts
/ {
    position_map {
        compatible = "zmk,physical-layout-position-map";
        complete; // Not necessary, but generally recommended if you use position maps
        layout1: layout1 {
            physical-layout = <&physical_layout1>;
            positions = <...>; // List of positions to map
        };
        layout2: layout2 {
            physical-layout = <&physical_layout2>;
            positions = <...>; // List of positions to map
        };
    };
};
```

Position maps are optional; if a position map is not present, [ZMK Studio](../../features/studio.md) will automatically determine a (potentially inaccurate) mapping based on the physical key properties. This approach is also used as a fallback for positions not specified in the map if the `complete` property is not set.

See also the [configuration section on position maps](../../config/layout.md#physical-layout-position-map).

:::info

Key positions in the maps are numbered like the keys in your keymap, starting at 0. So the first position in the layout is position `0`, the next key position is `1`, etc.

:::

#### Writing a position map

Start by creating the parent node defining the position map:

```dts
/ {
    keypad_lossless_position_map {
        compatible = "zmk,physical-layout-position-map";
        complete; // Not necessary, but generally recommended if you use position maps

        // Child node 1 here

        // Child node 2 here

        // ...
    };
};
```

Next, select a layout to be the "reference" layout. It is recommended that this be the layout which has the highest number of keys, as that prevents key mappings from being "lost" when switching back and forth between layouts.

Create the child node for the "reference" layout, and fill the `positions` array by iterating through the keys in your layout in the same order as the matrix transform. For a 2x2 macropad the child node would be

```dts
macropad_map: macropad {
    physical-layout = <&macropad_layout>;
    positions // This is equivalent to `positions = <0 1 2 3>;`, reshaped for readability
        = < 0  1 >
        , < 2  3 >;
};
```

Next, you'll need to write child nodes for every other layout, deriving the position map from the reference. Start by deleting any keys which aren't present in your layout, then reassign the numbers so that they match the matrix transform. Finally, (if the reference layout has more keys) add numbers back into the "empty"/"spare"/"nonexistent" positions so that both position maps count up to the same number. The below examples show this process more clearly.

#### Example larger position map

Consider the following macropad/numpad with two physical layouts:

![A 4x5 numpad/macropad](../../assets/hardware-integration/numpad.svg)

Let us first consider each side individually. The "reference" position map of the left side would look like this:

```dts
macropad_map: macropad {
    physical-layout = <&macropad_layout>;
    positions
        = < 0  1  2  3>
        , < 4  5  6  7>
        , < 8  9 10 11>
        , <12 13 14 15>
        , <16 17 18 19>;
};
```

Meanwhile, the "reference" position map of the right side with fewer keys would look like this:

```dts
numpad_map: numpad {
    physical-layout = <&numpad_layout>;
    positions
        = < 0  1  2  3>
        , < 4  5  6  7>
        , < 8  9 10   >
        , <11 12 13 14>
        , <15    16   >;
};
```

As a reminder, the `positions` property is a one-dimensional array like the `keys` property, formatted nicely through the use of whitespace and angle bracket groupings.

If the left side with more keys was used as the reference layout, then the overall position map of the keyboard would look like this:

```dts
/ {
    keypad_lossless_position_map {
        compatible = "zmk,physical-layout-position-map";
        complete;

        macropad_map: macropad {
            physical-layout = <&macropad_layout>;
            positions
                = < 0  1  2  3>
                , < 4  5  6  7>
                , < 8  9 10 11>
                , <12 13 14 15>
                , <16 17 18 19>;
        };

        numpad_map: numpad {
            physical-layout = <&numpad_layout>;
            positions
                = < 0  1  2  3>
                , < 4  5  6  7>
                , < 8  9 10 17>
                , <11 12 13 14>
                , <15 18 16 19>;
        };
    };
};
```

The "missing" positions are filled with the "spare" numbers of the layout with more keys. Meanwhile, if the right side with fewer keys is used as a reference, then the overall position map would look like this:

```dts
/ {
    keypad_lossy_position_map {
        compatible = "zmk,physical-layout-position-map";
        complete;

        macropad_map: macropad {
            physical-layout = <&macropad_layout>;
            positions
                = < 0  1  2  3>
                , < 4  5  6  7>
                , < 8  9 10   >
                , <12 13 14 15>
                , <16    18   >;
        };

        numpad_map: numpad {
            physical-layout = <&numpad_layout>;
            positions
                = < 0  1  2  3>
                , < 4  5  6  7>
                , < 8  9 10   >
                , <11 12 13 14>
                , <15    16   >;
        };
    };
};
```

The above example is "lossy" because (unlike the previous "lossless" example) if a user switches from the macropad layout to the numpad layout _and then_ switches from the numpad layout back to the macropad layout, the assignments to the keys present but not listed in the macropad's map are lost.

#### Example non-`complete` position map

Consider the above device again -- most of the positions have identical `keys` properties. For example, the macropad's `12` key and the numpad's `11` key would have the same physical property, and be mapped to each other automatically. The keys whose mappings are unable to be determined automatically are those with different physical characteristics: the 2u high and 2u wide keys, and their corresponding 1u counterparts.

A non-`complete` position map can be used to assign mappings to only these particular keys:

```dts
/ {
    keypad_lossy_position_map {
        compatible = "zmk,physical-layout-position-map";

        macropad_map: macropad {
            physical-layout = <&macropad_layout>;
            positions = <7 15 16>;
        };

        numpad_map: numpad {
            physical-layout = <&numpad_layout>;
            positions = <7 14 15>;
        };
    };
};
```

This is noticably simpler to write, and can be a useful way of saving flash space for memory-constrained devices. The above is a "lossy" mapping, though. While "lossless" non-`complete` mappings are possible, they can be counter-intuitive enough that it may be easier to write the full position map instead.

For completeness, the equivalent "lossless" non-`complete` position map is shown below:

```dts
/ {
    keypad_lossy_position_map {
        compatible = "zmk,physical-layout-position-map";

        macropad_map: macropad {
            physical-layout = <&macropad_layout>;
            positions = <7 11 15 19 16 17>;
        };

        numpad_map: numpad {
            physical-layout = <&numpad_layout>;
            positions = <7 17 14 19 15 18>;
        };
    };
};
```

#### Additional example: corne

The following is an example of a "lossless" position map which maps the 5-column and 6-column Corne keymap layouts. The 6 column layout is the reference layout.

```dts
    foostan_corne_lossless_position_map {
        compatible = "zmk,physical-layout-position-map";

        complete;

        twelve_map: twelve {
            physical-layout = <&foostan_corne_6col_layout>;
            positions
                = < 0  1  2  3  4  5  6  7  8  9 10 11>
                , <12 13 14 15 16 17 18 19 20 21 22 23>
                , <24 25 26 27 28 29 30 31 32 33 34 35>
                , <         36 37 38 39 40 41         >;
        };

        ten_map: ten {
            physical-layout = <&foostan_corne_5col_layout>;
            positions
                = <36  0  1  2  3  4  5  6  7  8  9 37>
                , <38 10 11 12 13 14 15 16 17 18 19 39>
                , <40 20 21 22 23 24 25 26 27 28 29 41>
                , <         30 31 32 33 34 35         >;
        };
    };
```

Meanwhile, the "lossy" version of the same position map with the 5 column version as reference looks like this:

```dts
    foostan_corne_lossy_position_map {
        compatible = "zmk,physical-layout-position-map";

        complete;

        twelve_map: twelve {
            physical-layout = <&foostan_corne_6col_layout>;
            positions
                = < 1  2  3  4  5  6  7  8  9 10>
                , <13 14 15 16 17 18 19 20 21 22>
                , <25 26 27 28 29 30 31 32 33 34>
                , <      36 37 38 39 40 41      >;
        };

        ten_map: ten {
            physical-layout = <&foostan_corne_5col_layout>;
            positions
                = < 0  1  2  3  4  5  6  7  8  9>
                , <10 11 12 13 14 15 16 17 18 19>
                , <20 21 22 23 24 25 26 27 28 29>
                , <      30 31 32 33 34 35      >;
        };
    };
```
