---
title: Trackball PIM447 Behaviors
sidebar_label: Trackball PIM447
---

The mode of the trackball PIM447 driver can be changed using the
following key behaviors.

- `&pim447_move`: set trackball mode to "move" .
- `&pim447_scroll`: set trackball mode to "scroll" .
- `&pim447_toggle`: toggle trackball mode (from "scroll" to "move", or vice versa).
- `&pim447_move_scroll`: set trackball mode to "move" when pressed, then to "scroll" when released.
- `&pim447_scroll_move`: set trackball mode to "scroll" when pressed, then to "move" when released.
- `&pim447_toggle_toggle`: toggle trackball mode when pressed, then toggle again when released.

Example:

```
#include <dt-bindings/zmk/trackball_pim447.h>

/ {
	keymap {
		compatible = "zmk,keymap";

		a_layer {
			bindings = <&kp a
				    &kp b
				    &kp c
				    &pim447_toggle>;
		};
	};
};
```
