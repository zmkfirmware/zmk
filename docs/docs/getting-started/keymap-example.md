```dts
    keymap {
        compatible = "zmk,keymap";

        default_layer { // Layer 0
            display-name = "Base";
// ----------------------------------------------
// |      Z      |        M       |      K      |
// |    &mo 1    |   LEFT SHIFT   |    &mo 2    |
            bindings = <
                &kp Z    &kp M       &kp K
                &mo 1    &kp LSHIFT  &mo 2
            >;
        };
        abc { // Layer 1
            display-name = "ABC";
// ----------------------------------------------
// |      A      |       B       |       C      |
// |    &trans   |    &trans     |     &trans   |
            bindings = <
                &kp A     &kp B     &kp C
                &trans    &trans    &trans
            >;
        };
        xyz { // Layer 2
            display-name = "XYZ";
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
