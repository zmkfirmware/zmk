# Cradio

Cradio is a firmware for a few 34 key keyboards, including Cradio, Hypergolic and Sweep.

## Pin arrangement

Some revisions of the aforementioned PCBs have slightly different pin arrangements compared to what's defined in [`cradio.dtsi`](./cradio.dtsi). If you need to swap a few keys for your particular PCB, you can easily reorder the `input-gpio` definition in your own keymap file (i.e. in `zmk-config/config/cradio.keymap`):

```dts
/* Adjusted Cradio pin arrangement */
/* The position of Q and B keys have been swapped */
&kscan0 {
	input-gpios
	= <&pro_micro_d  6 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_a  0 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_a  1 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_a  2 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_a  3 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d 15 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d 14 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d 16 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d 10 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d  1 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d  2 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d  3 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d  4 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d  5 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d  7 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d  8 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	, <&pro_micro_d  9 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
	;
};
```

This `&kscan0` block must be placed outside of any blocks surrounded by curly braces (`{...}`).
