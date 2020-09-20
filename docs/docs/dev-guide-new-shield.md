---
id: dev-guide-new-shield
title: New Keyboard Shield
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

## Overview

This guide will walk through the steps necessary to add ZMK support for a keyboard the uses a (Pro Micro compatible) addon MCU board to provide the microprocessor.
The high level steps are:

- Create a new shield directory.
- Add the base Kconfig files.
- Add the shield overlay file to define the [KSCAN driver]() for detecting key press/release.
- (Optional) Add the matrix transform for mapping KSCAN row/column values to sane key positions. This is needed for non-rectangular keyboards, or where the underlying row/column pin arrangement does not map one to one with logical locations on the keyboard.
- Add a default keymap, which users can override in their own configs as needed.
- Add support for features such as encoders, OLED displays, or RGB underglow.
- Update build.yml

It may be helpful to review the upstream [shields documentation](https://docs.zephyrproject.org/2.3.0/guides/porting/shields.html#shields) to get a proper understanding of the underlying system before continuing.

## New Shield Directory

Shields for Zephyr applications go into the `boards/shields/` directory; since ZMK's Zephyr application lives in the `app/` subdirectory of the repository, that means the new shield directory should be:

```bash
mkdir app/boards/shields/<keyboard_name>
```

## Base Kconfig Files

There are two required Kconfig files that need to be created for your new keyboard
shield to get it picked up for ZMK, `Kconfig.shield` and `Kconfig.defconfig`.

### Kconfig.shield

The `Kconfig.shield` file defines any additional Kconfig settings that may be relevant when using this keyboard. For most keyboards, there is just one additional configuration value for the shield itself, e.g.:

```
config SHIELD_MY_BOARD
	def_bool $(shields_list_contains,my_board)
```

This will make sure the new configuration `SHIELD_MY_BOARD` is set to true whenever `my_board` is added as a shield in your build.

### Kconfig.defconfig

The `Kconfig.defconfig` file is where overrides for various configuration settings
that make sense to have different defaults when this shield is used. One main item
that usually has a new default value set here is the `ZMK_KEYBOARD_NAME` value,
which controls the display name of the device over USB and BLE.

The updated new default values should always be wrapped inside a conditional on the shield config name defined in the `Kconfig.shield` file. Here's the simplest example file:

```
if SHIELD_MY_BOARD

config ZMK_KEYBOARD_NAME
	default "My Awesome Keyboard"

endif
```

## Shield Overlay

![Labelled Pro Micro pins](assets/pro-micro/pro-micro-pins-labelled.jpg)

ZMK uses the green color coded pin names to generate devicetree node references. For example, to refer to the node `D0` in the devicetree files, use `&pro_micro_d 0` or to refer to `A1`, use `&pro_micro_a 1`.

The `<shield_name>.overlay` is the devicetree description of the keyboard shield that is merged with the primary board devicetree description before the build. For ZMK, this file at a minimum should include the [chosen]() node named `zmk,kscan` that references a KSCAN driver instance. For a simple 3x3 macropad matrix,
this might look something like:

```
/ {
	chosen {
		zmk,kscan = &kscan0;
	};

	kscan0: kscan_0 {
		compatible = "zmk,kscan-gpio-matrix";
		label = "KSCAN";
        diode-direction = "col2row";

        col-gpios
            = <&pro_micro_d 15 GPIO_ACTIVE_HIGH>
            , <&pro_micro_d 14 GPIO_ACTIVE_HIGH>
            , <&pro_micro_d 16 GPIO_ACTIVE_HIGH>
            ;

		row-gpios
			= <&pro_micro_a 1 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
			, <&pro_micro_a 2 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            , <&pro_micro_a 3 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
			;
	};
};
```

## (Optional) Matrix Transform

Internally ZMK translates all row/column events into "key position" events to maintain a consistent model that works no matter what any possible GPIO matrix may look like for a certain keyboard. This is particularly helpful when:

1. To reduce the used pins, an "efficient" number of rows/columns for the GPIO matrix is used, that does _not_ match the physical layout of rows/columns of the actual key switches.
1. For non rectangular keyboards with thumb clusters, non `1u` locations, etc.

A "key position" is the numeric index (zero-based) of a given key, which identifies
the logical key location as perceived by the end user. All _keymap_ mappings actually bind behaviors to _key positions_, not to row/column values.

_Without_ a matrix transform, that intentionally map each key position to the row/column pair that position corresponds to, the default equation to determine that is:

```
($row * NUMBER_OF_COLUMNS) + $column
```

Which effectively amounts to numbering the key positions by traversing each row from top to bottom and assigning numerically incrementing key positions.

Whenever that default key position mapping is insufficient, the `<shield_name>.overlay` file should _also_ include a matrix transform.

Here is an example for the [nice60](https://github.com/Nicell/nice60), which uses an efficient 8x8 GPIO matrix, and uses a transform:

```
#include <dt-bindings/zmk/matrix-transform.h>

/ {
	chosen {
		zmk,kscan = &kscan0;
		zmk,matrix_transform = &default_transform;
	};

	default_transform: keymap_transform_0 {
		compatible = "zmk,matrix-transform";
		columns = <8>;
		rows = <8>;
// | MX1  | MX2  | MX3  | MX4  | MX5  | MX6  | MX7  | MX8  | MX9  | MX10 | MX11 | MX12 | MX13 |    MX14     |
// |   MX15   | MX16 | MX17 | MX18 | MX19 | MX20 | MX21 | MX22 | MX23 | MX34 | MX25 | MX26 | MX27 |  MX28   |
// |    MX29    | MX30 | MX31 | MX32 | MX33 | MX34 | MX35 | MX36 | MX37 | MX38 | MX39 | MX40 |     MX41     |
// |     MX42      | MX43 | MX44 | MX45 | MX46 | MX47 | MX48 | MX49 | MX50 | MX51 | MX52 |       MX53       |
// |  MX54  |  MX55  |  MX56  |                  MX57                   |  MX58  |  MX59  |  MX60  |  MX61  |
		map = <
RC(3,0)  RC(2,0) RC(1,0) RC(0,0) RC(1,1) RC(0,1) RC(0,2) RC(1,3) RC(0,3) RC(1,4) RC(0,4) RC(0,5) RC(1,6)     RC(1,7)
RC(4,0)    RC(4,1) RC(3,1) RC(2,1) RC(2,2) RC(1,2) RC(2,3) RC(3,4) RC(2,4) RC(2,5) RC(1,5) RC(2,6) RC(2,7)   RC(3,7)
RC(5,0)     RC(5,1) RC(5,2) RC(4,2) RC(3,2) RC(4,3) RC(3,3) RC(4,4) RC(4,5) RC(3,5) RC(4,6) RC(3,6)          RC(4,7)
RC(6,0)       RC(6,1) RC(6,2) RC(6,3) RC(5,3) RC(6,4) RC(5,4) RC(6,5) RC(5,5) RC(6,6) RC(5,6)                RC(5,7)
RC(7,0)    RC(7,1)   RC(7,2)                     RC(7,3)                    RC(7,5)    RC(7,6)    RC(6,7)    RC(7,7)
		>;
	};
```

Some important things to note:

- The `#include <dt-bindings/zmk/matrix-transform.h>` is critical. The `RC` macro is used to generate the internal storage in the matrix transform, and is actually replaced by a C preprocessor before the final devicetree is compiled into ZMK.
- `RC(row, column)` is placed sequentially to define what row and column values that position corresponds to.
- If you have a keyboard with options for `2u` keys in certain positions, or break away portions, it is a good idea to set the chosen `zmk,matrix_transform` to the default arrangement, and include _other_ possible matrix transform nodes in the devicetree that users can select in their user config by overriding the chosen node.

## Default Keymap

Each keyboard should provide an OOTB default keymap to be used when building the firmware, which can be overridden and customized by user configs. For "shield keyboards", this should be placed in the `app/boards/shields/<shield_name>/<shield_name>.keymap` file. The keymap is configured as an additional devicetree overlay that includes the following:

- A node with `compatible="zmk,keymap"` where each child node is a layer with a `bindings` array that binds each key position to a given behavior (e.g. key press, momentarily layer, etc).

Here is an example simple keymap for the Kyria, with only one layer:

```
#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>

/ {
	keymap {
		compatible = "zmk,keymap";

		default_layer {
// ---------------------------------------------------------------------------------------------------------------------------------
// |  ESC  |  Q  |  W  |  E   |  R   |  T   |                                          |  Y   |  U    |  I    |  O   |   P   |   \  |
// |  TAB  |  A  |  S  |  D   |  F   |  G   |                                          |  H   |  J    |  K    |  L   |   ;   |   '  |
// | SHIFT |  Z  |  X  |  C   |  V   |  B   | L SHIFT | L SHIFT |  | L SHIFT | L SHIFT |  N   |  M    |  ,    |  .   |   /   | CTRL |
//                     | GUI  | DEL  | RET  |  SPACE  |   ESC   |  |   RET   |  SPACE  | TAB  | BSPC  | R-ALT |
			bindings = <
	&kp ESC  &kp Q &kp W &kp E &kp R &kp T                                            &kp Y &kp U  &kp I    &kp O   &kp P    &kp BSLH
	&kp TAB  &kp A &kp S &kp D &kp F &kp G                                            &kp H &kp J  &kp K    &kp L   &kp SCLN &kp QUOT
	&kp LSFT &kp Z &kp X &kp C &kp V &kp B &kp LSFT &kp LSFT        &kp LSFT &kp LSFT &kp N &kp M  &kp CMMA &kp DOT &kp FSLH &kp RCTL
	              &kp LGUI &kp DEL &kp RET &kp SPC &kp ESC            &kp RET  &kp SPC  &kp TAB &kp BKSP &kp RALT
			>;

			sensor-bindings = <&inc_dec_cp M_VOLU M_VOLD &inc_dec_kp PGUP PGDN>;
		};
	};
};

```

:::note
The two `#include` lines at the top of the keymap are required in order to bring in the default set of behaviors to make them available to bind, and to import a set of defines for the HID keycodes, so keymaps can use parameters like `NUM_2` or `K` instead of the raw keycode numeric values.
:::

### Keymap Behaviors

Further documentation on behaviors and bindings is forthcoming, but a summary of the current behaviors you can bind to key positions is as follows:

- `kp` is the "key press" behavior, and takes a single binding argument of the HID keycode from the 'keyboard/keypad" HID usage table.
- `cp` is the "consumer key press" behavior, and takes a single binding argument of the HID keycode from the "consumer page" HID usage table. This is mostly useful for media keys.
- `mo` is the "momentary layer" behaviour, and takes a single binding argument of the numeric ID of the layer to momentarily enable when that key is held.
- `trans` is the "transparent" behavior, useful to be place in higher layers above `mo` bindings to be sure the key release is handled by the lower layer. No binding arguments are required.
- `mt` is the "mod-tap" behavior, and takes two binding arguments, the modifier to use if held, and the keycode to send if tapped.

## Adding Features

### Encoders

EC11 encoder support can be added to your board or shield by adding the appropriate lines to your board/shield's configuration (.conf), device tree (.dtsi), overlay (.overlay), and keymap (.keymap) files.

<Tabs
defaultValue="conf"
values={[
{label: '.conf', value: 'conf'},
{label: '.dtsi', value: 'dtsi'},
{label: '.overlay', value: 'overlay'},
{label: '.keymap', value: 'keymap'},
]}>
<TabItem value="conf">

In your configuration file you will need to add the following lines so that the encoders can be enabled/disabled:

```
# Uncomment to enable encoder
# CONFIG_EC11=y
# CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
```

These should be commented by default for encoders that are optional/can be swapped with switches, but can be uncommented if encoders are part of the default design.

:::note
If building locally for split boards, you may need to add these lines to the specific half's configuration file as well as the combined configuration file.
:::

</TabItem>
<TabItem value = "dtsi">
In your device tree file you will need to add the following lines to define the encoder sensor:

  
```
left_encoder: encoder_left {
		compatible = "alps,ec11";
		label = "LEFT_ENCODER";
		a-gpios = <PIN_A (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
		b-gpios = <PIN_B (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
		resolution = <4>;
	};
```
Here you will have to replace PIN_A and PIN_B with the appropriate pins that your PCB utilizes for the encoder(s). For keyboards that use the Pro Micro or any of the Pro Micro replacements, Sparkfun's [Pro Micro Hookup Guide](https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/hardware-overview-pro-micro) has a pinout diagram that can be useful to determine the right pins. Reference either the blue numbers labeled "Arduino" (digital pins) or the green numbers labeled "Analog" (analog pins). For pins that are labeled as both digital and analog, refer to your specific board's .dtsi file to determine how you should refer to that pin.

Add additional encoders as necessary by duplicating the above lines, replacing `left` with whatever you would like to call your encoder, and updating the pins. Note that support for peripheral (right) side sensors over BLE is still in progress.

Once you have defined the encoder sensors, you will have to add them to the list of sensors:

```
sensors {
		compatible = "zmk,keymap-sensors";
		sensors = <&left_encoder &right_encoder>;
	};
```

In this example, a left_encoder and right_encoder are both added. Additional encoders can be added with spaces separating each, and the order they are added here determines the order in which you define their behavior in your keymap.

</TabItem>
<TabItem value = "overlay">
Add the following lines to your overlay file(s) to enable the encoder:

```
&left_encoder {
	status = "okay";
};
```

:::note
For split keyboards, make sure to add left hand encoders to the left .overlay file and right hand encoders to the right .overlay file.
:::

</TabItem>
<TabItem value = "keymap">
Add the following line to your keymap file to add default encoder behavior bindings:   

```
sensor-bindings = <&inc_dec_cp M_VOLU M_VOLD>;
```
Add additional bindings as necessary to match the default number of encoders on your board. See the [Encoders](/docs/feature/encoders) and [Keymap](/docs/feature/keymaps) feature documentation for more details.

</TabItem>
</Tabs>

## Testing

Once you've fully created the new keyboard shield definition,
you should be able to test with a build command like:

```
west build --pristine -b proton_c -- -DSHIELD=my_board
```

and then flash with:

```
west flash
```

## Updating `build.yml`

Before publishing your shield to the public via a PR, navigate to `build.yml` found in `.github/workflows` and add your shield to the appropriate list. An example edit to `build.yml` is shown below.

```
jobs:
  build:
    runs-on: ubuntu-latest
    name: Build Test
    strategy:
      matrix:
        board: [proton_c, nice_nano, bluemicro840_v1, nrfmicro_13]
        shield:
          - corne_left
          - corne_right
          - kyria_left
          - kyria_right
          - lily58_left
          - lily58_right
          - iris_left
          - iris_right
          - romac
	  - <MY_BOARD>
	  - <MY_SPLIT_BOARD_left>
	  - <MY_SPLIT_BOARD_right>
        include:
          - board: proton_c
            shield: clueboard_california
```
:::note
Notice that both the left and right halves of a split board need to be added to the list of shields for proper error checking.
:::note
