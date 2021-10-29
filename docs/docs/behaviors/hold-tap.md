---
title: Hold-Tap Behavior
sidebar_label: Hold-Tap
---

## Summary

Hold-tap is the basis for other behaviors such as layer-tap and mod-tap.

Simply put, the hold-tap key will output the 'hold' behavior if it's held for a while, and output the 'tap' behavior when it's tapped quickly.

### Hold-Tap

The graph below shows how the hold-tap decides between a 'tap' and a 'hold'.

![Simple behavior](../assets/hold-tap/case1_2.png)

By default, the hold-tap is configured to also select the 'hold' functionality if another key is tapped while it's active:

![Hold preferred behavior](../assets/hold-tap/case1_2.png)

We call this the 'hold-preferred' flavor of hold-taps. While this flavor may work very well for a ctrl/escape key, it's not very well suited for home-row mods or layer-taps. That's why there are two more flavors to choose from: 'tap-preferred' and 'balanced'.

#### Flavors

- The 'hold-preferred' flavor triggers the hold behavior when the `tapping-term-ms` has expired or another key is pressed.
- The 'balanced' flavor will trigger the hold behavior when the `tapping-term-ms` has expired or another key is pressed and released.
- The 'tap-preferred' flavor triggers the hold behavior when the `tapping-term-ms` has expired. It triggers the tap behavior when another key is pressed.
- The 'tap-positionally-preferred' flavor triggers the hold behavior if and only if a key at one of the `hold-trigger-key-positions` is pressed before `tapping-term-ms` expires. See the below section on `hold-trigger-key-positions` for more details.

When the hold-tap key is released and the hold behavior has not been triggered, the tap behavior will trigger.

![Hold-tap comparison](../assets/hold-tap/comparison.png)

### Basic usage

For basic usage, please see [mod-tap](mod-tap.md) and [layer-tap](layers.md) pages.

### Advanced Configuration

#### `tapping-term-ms`

Defines how long a key must be pressed to trigger Hold behavior.

#### `quick_tap_ms`

If you press a tapped hold-tap again within `quick_tap_ms` milliseconds, it will always trigger the tap behavior. This is useful for things like a backspace, where a quick tap+hold holds backspace pressed. Set this to a negative value to disable. The default is -1 (disabled).

In QMK, unlike ZMK, this functionality is enabled by default, and you turn it off using `TAPPING_FORCE_HOLD`.

#### `retro-tap`

If retro tap is enabled, the tap behavior is triggered when releasing the hold-tap key if no other key was pressed in the meantime.

For example, if you press `&mt LEFT_SHIFT A` and then release it without pressing another key, it will output `a`.

```
&mt {
	retro-tap;
};
```

#### `hold-trigger-key-positions`

- `hold-trigger-key-positions` is used exclusively with the `tap-positionally-preferred` flavor, and is required for the `tap-positionally-preferred` flavor.
- Specifies which key positions are eligible for triggering a hold behavior.
- If and only if a key at one of the `hold-trigger-key-positions` is pressed before `tapping-term-ms` expires, will 'tap-positionally-preferred' produce a hold behavior.
- `hold-trigger-key-positions` is an array of key positions indexes. Key positions are numbered / indexed according to the keys in your keymap, starting at 0. So, if the first key in your keymap is Q, this key is in position 0. The next key (possibly W) will have position 1, et cetera. 
* See the following example:

```
#include <dt-bindings/zmk/keys.h>
#include <behaviors.dtsi>

/ {
	behaviors {
		tpht: tap_positional_hold_tap {
			compatible = "zmk,behavior-hold-tap";
			label = "TAP_POSITIONAL_HOLD_TAP";
			#binding-cells = <2>;
			flavor = "tap-positionally-preferred";
			tapping-term-ms = <200>;
			quick-tap-ms = <200>;
			bindings = <&kp>, <&kp>;
			hold-trigger-key-positions = <1>;    // <---[[the W key]]
		};
	};

	keymap {
		compatible = "zmk,keymap";
		label ="Default keymap";
		default_layer {
			bindings = <
				//  position 0         position 1       position 2
				&tpht LEFT_SHIFT Q        &kp W            &kp E
			>;
		};
	};
};
```

- The sequence `(tpht_down, W_down, W_up, tpht_up)` produces `W`. The `tap-positionally-preferred` flavor produces a **hold** behavior because the first key pressed after the hold-tap key is the `W key`, which is in position 1, which **IS** included in `hold-trigger-key-positions`.
- Meanwhile the sequence `(tpht_down, E_down, E_up, tpht_up)` produces `qe`. The `tap-positionally-preferred` flavor produces a **tap** behavior because the first key pressed after the hold-tap key is the `E key`, which is in position 2, which is **NOT** included in `hold-trigger-key-positions`.
- If the `LEFT_SHIFT / Q key` is held by itself for longer than `tapping-term-ms`, a **tap** behavior is produced.
- The `tap-positionally-preferred` flavor is useful with home-row modifiers. With a home-row-modifier key in the left hand, by only including keys positions from the right hand in `hold-trigger-key-positions`, hold behaviors will only occur with cross-hand key combinations.

#### Home row mods

This example configures a hold-tap that works well for homerow mods:

```
#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>

/ {
	behaviors {
		hm: homerow_mods {
			compatible = "zmk,behavior-hold-tap";
			label = "HOMEROW_MODS";
			#binding-cells = <2>;
			tapping-term-ms = <150>;
			quick_tap_ms = <0>;
			flavor = "tap-preferred";
			bindings = <&kp>, <&kp>;
		};
	};

	keymap {
		compatible = "zmk,keymap";

		default_layer {
			bindings = <
	            &hm LCTRL A &hm LGUI S &hm LALT D &hm LSHIFT F
			>;
		};
	};
};

```

If this config does not work for you, try the flavor "balanced" with a medium `tapping-term-ms` such as 200ms.

#### Comparison to QMK

The hold-preferred flavor works similar to the `HOLD_ON_OTHER_KEY_PRESS` setting in QMK. The 'balanced' flavor is similar to the `PERMISSIVE_HOLD` setting, and the `tap-preferred` flavor is similar to `IGNORE_MOD_TAP_INTERRUPT`.
