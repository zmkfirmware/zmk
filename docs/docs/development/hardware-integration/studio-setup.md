---
title: ZMK Studio Setup
---

:::warning[Alpha Feature]

ZMK Studio support is in alpha. Although best efforts are being made, backwards compatibility during active development is not guaranteed.

:::

This guide will walk you through enabling ZMK Studio support for a keyboard.

The main additional pieces needed for ZMK Studio support involve additional metadata needed in order
to properly to display the physical layouts available for the particular keyboard.

# Physical Layout Positions

Physical layouts are described as part of the [new shield guide](./new-shield.mdx#physical-layout) with the exception of the `keys` property that is required for ZMK Studio support. This is used to describe the physical attributes of each key position present in that layout and its items are listed in the same order as keymap bindings, matrix transforms, etc. The properties available are:

| Property   | Type     | Description                          | Unit                                                    |
| ---------- | -------- | ------------------------------------ | ------------------------------------------------------- |
| Width      | int (>0) | Key(cap) width                       | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Height     | int (>0) | Key(cap) height                      | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| X          | uint     | Key X position (top-left point)      | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Y          | uint     | Key Y position (top-left point)      | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Rotation   | int      | Key rotation (positive => clockwise) | [centi-](https://en.wikipedia.org/wiki/Centi-)degree    |
| Rotation X | int      | Rotation origin X position           | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |
| Rotation Y | int      | Rotation origin Y position           | [centi-](https://en.wikipedia.org/wiki/Centi-)"keyunit" |

:::note
You can specify negative values in devicetree using parentheses around it, e.g. `(-3000)` for a 30 degree counterclockwise rotation.
:::

## Header Include

To pull in the necessary definition for creating physical layouts, a new include should be added to the top of the devicetree file:

```
#include <physical_layouts.dtsi>
```

## Example

Here is an example physical layout for a 2x2 macropad:

```dts
    macropad_physical_layout: macropad_physical_layout {
        compatible = "zmk,physical-layout";
        display-name = "Macro Pad";

        keys  //                     w   h    x    y     rot    rx    ry
            = <&key_physical_attrs 100 100    0    0       0     0     0>
            , <&key_physical_attrs 100 100  100    0       0     0     0>
            , <&key_physical_attrs 100 100    0  100       0     0     0>
            , <&key_physical_attrs 100 100  100  100       0     0     0>
            ;
    };
```

# Position Map

When switching between layouts with ZMK Studio, the keymap of the previously selected layout is used to populate the keymap in the new layout. To determine which keymap entry maps to which entry in the new layout, keys between the two layouts that share the exact same physical attributes are matched.

However, keys between layouts might not be in exactly the same positions, in which case a position map can be used. The position map includes a sequence for every relevant layout, and the corresponding entries in `positions` property will be used to determine the mapping between layouts. By default, the physical attribute matching behavior will be used as a fallback for positions not specified in the map, but the `complete` property can be added to the map to specify that no matching fallback should occur.

:::info

Key positions in the maps are numbered like the keys in your keymap, starting at 0. So the first position in the layout is position `0`, the next key position is `1`, etc.

:::

## Examples

### Basic Map

For example, the following position map correctly maps the 5-column and 6-column Corne keymap layouts.

```dts
    foostan_corne_position_map {
        compatible = "zmk,physical-layout-position-map";

        complete;

        twelve {
            physical-layout = <&foostan_corne_6col_layout>;
            positions
                = < 1  2  3  4  5  6  7  8  9 10>
                , <13 14 15 16 17 18 19 20 21 22>
                , <25 26 27 28 29 30 31 32 33 34>
                , <      36 37 38 39 40 41      >;
        };

        ten {
            physical-layout = <&foostan_corne_5col_layout>;
            positions
                = < 0  1  2  3  4  5  6  7  8  9>
                , <10 11 12 13 14 15 16 17 18 19>
                , <20 21 22 23 24 25 26 27 28 29>
                , <      30 31 32 33 34 35      >;
        };
    };
```

The first entries in the two mappings have values `1` and `0` respectively, which means that position `1` in the 6-column layout will map to position `0` in the 5-column layout, the second entries show that position `2` in the 6-column layout corresponds to position `1` in the 5-column layout, etc.

### Full Preserving Map

The above basic example has one major downside. Because the keys on the outer columns of the 6-column layout aren't mapped into any locations in the 5-column layout, when a user switches to the 5-column layout and then back to the 6-column layout, the bindings for those outer columns will have been lost/dropped at the first step.

In order to preserve those bindings that are in "missing" keys in other layouts, we can include those locations in the map, but map them "off the end" of the smaller layout key positions.

Here is a fixed up Corne mapping:

```dts
    foostan_corne_position_map {
        compatible = "zmk,physical-layout-position-map";

        complete;

        twelve {
            physical-layout = <&foostan_corne_6col_layout>;
            positions
                = < 0  1  2  3  4  5  6  7  8  9 10 11>
                , <12 13 14 15 16 17 18 19 20 21 22 23>
                , <24 25 26 27 28 29 30 31 32 33 34 35>
                , <         36 37 38 39 40 41         >;
        };

        ten {
            physical-layout = <&foostan_corne_5col_layout>;
            positions
                = <36  0  1  2  3  4  5  6  7  8  9 37>
                , <38 10 11 12 13 14 15 16 17 18 19 39>
                , <40 20 21 22 23 24 25 26 27 28 29 41>
                , <         30 31 32 33 34 35         >;
        };
    };
```

Notice how the outer column positions in the 6-column layout are mapped to positions 36, 37, etc. in the 5-column layout. The 5-column layout only uses key positions up to 35, so those bindings in the outer columns will get migrated into the "extra space" that is ignored by the smaller layout, preserved to get mapped back in place when the user switches back.
