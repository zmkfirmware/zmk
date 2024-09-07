### KScan sideband behavior

The kscan sideband behavior driver will be used to trigger the [soft off behavior](../../../keymaps/behaviors/soft-off.md) "out of band" from the normal keymap processing. To do so, it will decorate/wrap an underlying kscan driver.

For the matrix-integrated approach you will supplement the existing kscan matrix by adding the additional pin as another entry in
the `row-gpios`/`col-gpios` for whichever pins are used to read the matrix state. This approach requires a matrix transform to be present. As an example, consider the following existing kscan matrix:

```dts
/ {
    kscan: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        diode-direction = "col2row";
        wakeup-source;

        col-gpios
            = <&gpio0 12 GPIO_ACTIVE_HIGH>
            , <&gpio1 9  GPIO_ACTIVE_HIGH>
            ;
        row-gpios
            = <&gpio0 19 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            , <&gpio0 4  (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            ;
    };
};
```

To supplement it with a soft off input, you would add another row value (without changing the matrix transform):

```dts
/ {
    kscan: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        diode-direction = "col2row";
        wakeup-source;

        col-gpios
            = <&gpio0 12 GPIO_ACTIVE_HIGH>
            , <&gpio1 9  GPIO_ACTIVE_HIGH>
            ;
        row-gpios
            = <&gpio0 19 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            , <&gpio0 4  (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            , <&gpio0 2  (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            ;
    };
};
```

With that in place, you would decorate the kscan driver:

```dts
/ {
    side_band_behavior_triggers: side_band_behavior_triggers {
        compatible = "zmk,kscan-sideband-behaviors";
        wakeup-source;
        kscan = <&kscan>;
        soft_off {
            column = <0>;
            row = <2>;
            bindings = <&hw_soft_off>;
        };
    };
};
```

Critically, the `column` and `row` values would correspond to the location of the added entry. The properties of the `kscan-sideband-behaviors` node can be found in the [appropriate configuration section](../../../config/kscan.md#kscan-sideband-behavior-driver).
