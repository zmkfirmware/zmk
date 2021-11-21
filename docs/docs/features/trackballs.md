---
title: Trackballs
sidebar_label: Trackballs
---

Only the [Pimoroni PIM447](https://shop.pimoroni.com/products/trackball-breakout)
trackball is supported in ZMK so far.

## Enable Trackball Support

To enable support for PIM447 trackball — and to make it acts like a
mouse — add the two following lines to the shield `.config` file:

```ini
CONFIG_ZMK_MOUSE=y
CONFIG_ZMK_TRACKBALL_PIM447=y
```

Assuming the PIM447 trackball is connected to the `pro_micro_i2c` pins
and its bus address hasn't been changed, then the following lines must
be added to the shield `.overlay` file:

```devicetree
&pro_micro_i2c {
	status = "okay";

	trackball_pim447@a {
		compatible = "pimoroni,trackball_pim447";
		reg = <0xa>;
		label = "TRACKBALL_PIM447";
	};
};
```

## Customize Trackball Driver Properties

The PIM447 trackball driver can be configured according to a lot of
different properties that are listed in the shield `.overlay` sample
below:

```devicetree
#include <dt-bindings/zmk/trackball_pim447.h> // for PIM447_MOVE and PIM447_SCROLL constants.

&pro_micro_i2c {
	status = "okay";

	trackball_pim447@a {
		compatible = "pimoroni,trackball_pim447";
		reg = <0xa>;
		label = "TRACKBALL_PIM447";

		move-factor = <2>;      // Increase pointer velocity (default: 1)
		invert-move-x;          // Invert pointer X axis (left is right, and vice versa)
		invert-move-y;          // Invert pointer Y axis (up is down, and vice versa)
		button = <1>;           // Send right-click when pressing the ball (default: 0, ie. left-click)
		swap-axes;              // Swap X and Y axes (horizontal is vertical, and vice versa)
		scroll-divisor = <1>;   // Increase wheel velocity (default: 2)
		invert-scroll-x;        // Invert wheel X axis (left is right, and vice versa)
		invert-scroll-y;        // Invert wheel Y axis (up is down, and vice versa)
		mode = <PIM447_SCROLL>; // Act as mouse wheels at startup (default: PIM447_MOVE for a pointer)
	};
};
```

## Change Trackball Driver Mode

As seen at the end of the previous section, the PIM447 trackball
driver can act as a mouse pointer or as mouse wheels according to the
`mode` specified in the shield `.overlay` file.  It can also be
changed dynamically using [dedicated key
behaviors](/docs/behaviors/trackball-pim447).
