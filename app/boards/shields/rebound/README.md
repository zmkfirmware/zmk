# Supported versions

This shield, Rebound_v4 provides support for the Montsinger Rebound v4.x variants only.
Previous iterations of the Rebound, variants v3.x and earlier are not supported as they use a different matrix to that which is used for the Rebound v4.x variants.

## Building ZMK for the Rebound_v4

Some general notes/commands for building the Montsinger Rebound_v4 from the assembly documentation.

## Layouts using 1u and/or 2u thumb keys

If you built your Rebound_v4 with 2u keys on the bottom row, key-presses of the inside/second column 1u key on the bottom row will be redundant (the switch position labelled as "optional" in the keymap diagram below).

If your Rebound_v4 is built with hotswap support using PCB sockets, you may wish to assign key-press behaviours to all four 1u keys to allow hotswapping between 1u and 2u layouts on both left and right portions of the PCB without having to rebuild and reflash the firmware.

As an example template of each layout, the default keymap shown below envisages a uniform 1u layout on the left portion of the pcb, and a 2u layout on the right portion with the redundant key labelled as "optional" and assigned with key-press "&none".

(Therefore, if you instead choose to implement a 2u layout for the left portion of the PCB whilst using the below default keymap, the key currently mapped as "&kp DEL" on the bottom row will simply become redundant until a 1u layout is implemented)

```
    keymap {
        compatible = "zmk,keymap";

        default_layer {
// -------------------------------------------------------------------------------------
// |   Q   |   W   |   E   |    R   |    T   |    1   |                        |   1   |    Y   |    U   |   I   |   O   |   P   |
// | GUI/A | ALT/S | CTL/D | SHFT/F |    G   |    2   |        | TAB  |        |   2   |    H   | SHFT/J | CTL/K | ALT/L | GUI/' |
// |   Z   |   X   |   C   |    V   |    B   |    3   |        | RET  |        |   3   |    N   |    M   |   ,   |   .   |   /   |
// | CTRL  |  ALT  |  GUI  |  SHFT  |   DEL  |  BSPC  |        | CAPS |        |  SPC  |optional|   GUI  |  ALT  |  CTRL |   ;   |
//
		bindings = <
   &kp Q       &kp W      &kp E       &kp R       &kp T   &mo 1               &mo 1     &kp Y     &kp U       &kp I       &kp O      &kp P
   &hm LGUI A  &hm LALT S &hm LCTRL D &hm LSHFT F &kp G   &mo 2     &kp TAB   &mo 2     &kp H     &hm RSHFT J &hm RCTRL K &hm RALT L &hm RGUI SQT
   &kp Z       &kp X      &kp C       &kp V       &kp B   &mo 3     &kp RET   &mo 3     &kp N     &kp M       &kp COMMA   &kp DOT    &kp FSLH  
   &kp LCTRL   &kp LALT   &kp LGUI    &kp LSHFT   &kp DEL &kp BSPC  &kp CAPS  &kp SPACE &none     &kp RGUI    &kp RALT    &kp RCTRL  &kp SEMI                   
                        >;

			sensor-bindings = <&inc_dec_kp DEL BSPC>;
                };
```

## Encoder Notes

If you built your Rebound_v4 with encoders, you will need to amend your Rebound_v4 config file by uncommenting the following two lines:

```
# Uncomment these two lines to add support for encoders
# CONFIG_EC11=y
# CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
```

To the following:
```
# Uncomment these two lines to add support for encoders
CONFIG_EC11=y
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
```

If you built your Rebound_v4 without encoders, you can either leave those two lines in your Rebound_v4 config file as comments, or replace them with:

```
CONFIG_EC11=n
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=n
```
