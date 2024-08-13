```dts
    keymap {
        compatible = "zmk,keymap";

        default_layer { // Layer 0
// ----------------------------------------------
// |      Z      |        M       |      K      |
// |    &mo 1    |   LEFT SHIFT   |    &mo 2    |
            bindings = <
                &kp Z    &kp M       &kp K
                &mo 1    &kp LSHIFT  &mo 2
            >;
        };
        abc { // Layer 1
// ----------------------------------------------
// |      A      |       B       |       C      |
// |    &trans   |    &trans     |     &trans   |
            bindings = <
                &kp A     &kp B     &kp C
                &trans    &trans    &trans
            >;
        };
        xyz { // Layer 2
// ----------------------------------------------
// |        X       |      Y       |      Z     |
// |   LEFT CTRL    |  LEFT ALT    |   &trans   |
            bindings = <
                &kp X        &kp Y       &kp Z
                &kp LCTRL    &kp LALT    &trans
            >;
        };
    };
```
