/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/bt.h>
#include <dt-bindings/zmk/outputs.h>
#include <dt-bindings/zmk/rgb.h>

#define TIMEOUT 300

&middle_left_encoder {
    status = "okay";
};

&top_right_encoder {
    status = "okay";
};

/ {
    combos {
        compatible = "zmk,combos";
        combo_btclr {
            timeout-ms = <TIMEOUT>;
            key-positions = <0 4>;
            bindings = <&bt BT_CLR>;
        };
        combo_reset {
            timeout-ms = <TIMEOUT>;
            key-positions = <0 2>;
            bindings = <&sys_reset>;
        };
        combo_bootloader {
            timeout-ms = <TIMEOUT>;
            key-positions = <0 1>;
            bindings = <&bootloader>;
        };
        combo_bt_nxt {
            timeout-ms = <TIMEOUT>;
            key-positions = <0 3>;
            bindings = <&bt BT_NXT>;
        };
    };

    keymap: keymap {
        compatible = "zmk,keymap";

        default_layer {
            display-name = "default";
            bindings = <
                            &kp F1       &kp F2          &kp F3            &kp F4
                            &kp KP_NUM   &kp KP_DIVIDE   &kp KP_MULTIPLY   &kp KP_MINUS
                            &kp KP_N7    &kp KP_N8       &kp KP_N9         &kp KP_PLUS
                &kp C_MUTE  &kp KP_N4    &kp KP_N5       &kp KP_N6         &trans
                &mo 1       &kp KP_N1    &kp KP_N2       &kp KP_N3         &kp KP_ENTER
                &kp BSPC    &kp KP_N0    &trans          &kp KP_DOT        &trans

                            &bt BT_CLR   &rgb_ug RGB_TOG &rgb_ug RGB_EFF
            >;
            sensor-bindings = <&inc_dec_kp C_VOL_UP C_VOL_DN &inc_dec_kp PG_UP PG_DN>;

        };

        fn_layer {
            display-name = "fn";
            bindings = <
                            &out OUT_TOG     &bt BT_PRV       &bt BT_NXT       &trans
                            &trans           &trans           &trans           &trans
                            &rgb_ug RGB_HUD  &rgb_ug RGB_SPI  &rgb_ug RGB_HUI  &trans
                &bt BT_CLR  &rgb_ug RGB_EFR  &rgb_ug RGB_TOG  &rgb_ug RGB_EFF  &trans
                &trans      &rgb_ug RGB_BRD  &rgb_ug RGB_SPD  &rgb_ug RGB_BRI  &trans
                &kp DEL     &rgb_ug RGB_SAD  &trans           &rgb_ug RGB_SAI  &trans

                            &trans           &trans           &trans
            >;
            sensor-bindings = <&inc_dec_kp PG_UP PG_DN &inc_dec_kp C_VOL_UP C_VOL_DN>;

        };
    };
};
