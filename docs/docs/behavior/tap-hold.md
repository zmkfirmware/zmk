---
title: Tap or Hold
---

This behavior is unlike many other behaviors. It implements tap or hold logic for a single key allowing any
other behaviors as arguments.

Usage:
```
#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>

/ {
	behaviors {
		rst: behavior_tap_hold {
			compatible = "zmk,behavior-hold-tap";
			label = "Reset Tap - Bootloader Hold";
			#binding-cells = <0>;
			hold_ms = <3000>;
			bindings = <&bootloader>, <&reset>;
		};

		mup: behavior_music_up {
			compatible = "zmk,behavior-hold-tap";
			label = "Reset Tap - Bootloader Hold";
			#binding-cells = <0>;
			hold_ms = <3000>;
			bindings = <&cp M_NEXT>, <&cp M_VOLU>;
		};
		
		mdwn: behavior_music_down {
			compatible = "zmk,behavior-hold-tap";
			label = "Reset Tap - Bootloader Hold";
			#binding-cells = <0>;
			hold_ms = <3000>;
			bindings = <&cp M_NEXT>, <&cp M_VOLU>;
		};
	};
		
	keymap {
		compatible = "zmk,keymap";

		default_layer {
			bindings = <
	            &rst &mup &mdwn
			>;
		};
	};
};

```