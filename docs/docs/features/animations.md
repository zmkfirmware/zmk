---
title: RGB Animations
sidebar_label: RGB Animations
---

ZMK supports animation effects on RGB control strips via the animation
subsystem.

## Enabling Animation Support

Set `CONFIG_ZMK_ANIMATION=y` to enable animation support on your keyboard.
See animation configuration for instructions on how to customize animation
support.

If your board does not have animation support configured, see
[Adding Animation Support.](#adding-animation-support)

## Color settings

All colors are given in [HSL](https://www.w3schools.com/colors/colors_hsl.asp)
format. The user should specify colors in this format, although they will
be written to the LED chain driver in RGB format.

## System Design

This subsystem is made up of 3 key components:
- The core animations layer
- Animation drivers
- Color utilites

### Core Animation Layer

The core animation layer is responsible for scheduling animations, kicking
off rendering of new frames, and sending frame data to the LED strip via
Zephyr's LED Strip API.

### Animation Drivers

Each animation effect is implemented as a driver. The animation API exposes
the following functions:

|       Function           |          Purpose                      |
| ------------------------ | ------------------------------------- |
| `animation_start`        | starts rendering the animation effect |
| `animation_stop`         | ends rendering of the effect          |
| `animation_render_frame` | renders a frame of the animation      |

Each driver will also likely utilize `ZMK_LISTENER` to listen for keypress
events. A driver can then request the animation subsystem render frames
using `zmk_animation_request_frames`.


### Color Utilities

Color utilities are used by the animation drivers to perform color conversions.
Since colors are given in HSL format, `zmk_hsl_to_rgb` is provided to
convert user color input from devicetree to RGB data before writing to an
animation output pixel.

## Adding Animation Support

Adding animation support to your board will require the following devicetree
changes:
- Animation core node with `zmk,animation` compatible describing
  "pixels" within animation as well as LED drivers to write output pixels to
- root animation node with `zmk,animation-control` compatible to cycle though
  animations
- animation driver nodes for all animations the user would like to implement

### Animation Core Node

This node describes all output "pixels" on your keyboard. These pixels
correspond to individual LEDs on your LED chain. The animation subsystem
will write these pixels to your LED chain driver in the order they are
declared.

Each pixel has an X and Y coordinate, which helps the animation
subsystem determine how effects like a ripple should be rendered on your
keyboard. X and Y coordinates can range from 0-255.

The node also needs to have references to your LED strip driver nodes within
devicetree. Multiple LED strip nodes can be provided, but each node must have
a `chain-length` property equal to the number of pixels in the LED chain.
When multiple LED strip nodes are provided, the `chain-length` of each will be
used to determine how many pixels to write. Pixels are written sequentially, so
the pixels for the second LED driver should be declared after those written
to the first one.

:::note

The total number of pixels defined in your `pixel` array property should match
the sum of all `chain-length` properties for the LED strip drivers you will be
using.

:::

For example, here is a chain of 6 pixels, using two LED drivers:
```
animation {
	compatible = "zmk,animation";
	drivers = <&led_ctrl1 &led_ctrl2>;
	pixels = <&pixel 0 0>, /* Pixel 0 is at (0,0) */
		<&pixel 21 0>, /* Pixel 1 is (0, 21) */
		<&pixel 42 0>,
		<&pixel 63 0>,
		<&pixel 84 0>,
		<&pixel 105 0>,
		<&pixel 255 255>; /* Max coordinate (255, 255) */
}
```

In this case, if `led_ctrl1` has a `chain-length` of 4 and `led_ctrl2` has
a `chain-length` of 2, then the first 4 pixels will be written to `led_ctrl1`,
and the final two will be written to `led_ctrl2`.


## Animation Root Node

Your animation root node should be an instance of the `zmk,animation-control`
driver. This driver allows you to cycle though other animation effects using
a keybinding.

A simple root animation node looks like this:

```
root_animation: animation_0 {
		compatible = "zmk,animation-control";
		label = "ANIMATION_CTRL_0";
        /* List of all animations to cycle through */
		animations = <&ripple_effect &solid_color>;
};
```
You should set the `zmk,animation` chosen node to this root animation:

```
chosen {
	zmk,animation = &root_animation;
};
```

Finally, you should add a keybinding to use the animation control feature
within your keymap:
```
#include <dt-bindings/zmk/animation.h>

default_layer {
    /* Basic binding, toggles animation on and off */
    bindings = <&animation ANIMATION_TOGGLE(0)>;
}
```

## Animation Drivers

Animation drivers allow you to create custom effects on your keyboard,
and implement different animation zones. Zone implementation is performed
using the `pixels` property, which sets the pixels from the `pixels` property
in the core animation node that the animation effect will apply to.

### Blending Modes

Several animations can utilize blending effects via the
`zmk_apply_blending_mode` API. The following blending modes are supported:

| Mode     | Affect                                                                    |
|----------|---------------------------------------------------------------------------|
| Multiply | Multiplies each pixel's color by the new effect's pixel color             |
| Lighten  | Selects the highest R, G, and B values between old and new for each pixel |
| Darken   | Selects the lowest R, G, and B values between old and new for each pixel  |
| Screen   | Adds (1 - base {R,G,B}) * new {R,G,B} value to base {R,G,B} value         |


### Ripple Effect

Creates a ripple effect on your keyboard each time a key is pressed,
originating at the pixel corresponding to the key. An example of
a ripple effect node is given below.

```
ripple_effect: animation_1 {
	compatible = "zmk,animation-ripple";
	status = "okay";
    /* Will only apply the effect to the first 4 pixels */
	pixels = <0 1 2 3>;
    /* Sets how this effect will be blended with other animations */
	blending-mode = <BLENDING_MODE_NORMAL>;
    /* Duration of animation in ms */
	duration = <1000>;
    /* Will result in a pure white */
	color = <HSL(0, 0, 100)>;
	ripple-width = <50>;
};
```

### Solid Color

Creates a solid color background. This effect can be combined with other
effects such as the ripple animation to produce a static background, or
zone specific lighting

```
solid_color: color {
    compatible = "zmk,animation-solid";
    status = "okay";
    /* All pixels will be illuminated */
    pixels = <0 1 2 3 4 5 6>;
    colors = <HSL(236, 30, 50)>;
};
```

### Compose Animation

Composes multiple animations together, allowing you to combine animation zones
or effects. Each animation will be rendered sequentially. If an animation has
a blending mode set, then it will be blended onto the current state of the
pixels

```
combine_zones: animation_1 {
    compatible = "zmk,animation-compose";
    status = "okay";
    animations = <
        &solid_color
        &ripple_effect
		>;
};
```
