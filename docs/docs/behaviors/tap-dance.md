---
title: Tap Dance Behavior
sidebar_label: Tap Dance
---

## Summary

A tap dance key outputs a keycode or behavior corresponding to how many times it is pressed.
Tap dances are completely custom, so for every unique tap dance key, a new tap dance must be defined in your keymap's
`behaviors`.

### Configuration

#### `tapping-term-ms`

Defines how much time in milliseconds after the tap dance is pressed before a keybind is registered.

#### `bindings`

A list of one or more keybinds. This list can include [any keycode in ZMK](../codes/) and keybinds for ZMK behaviors.

#### Example Usage

This example configures a tap dance that outputs the number of keypresses from 1-5:

```
#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>

/ {
	behaviors {
		td: tap_dance {
            compatible = "zmk,behavior-tap-dance";
            label = "TAP_TEST";
            #binding-cells = <0>;
            tapping_term_ms = <1000>;
            bindings = <&kp N1>, <&kp N2>, <&kp N3>, <&kp N4> , <&kp N5>;
        };
	};

	keymap {
		compatible = "zmk,keymap";

		default_layer {
			bindings = <
	            &td
			>;
		};
	};
};

```
