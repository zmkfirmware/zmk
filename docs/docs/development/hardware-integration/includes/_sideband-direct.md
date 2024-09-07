### KScan sideband behavior

The kscan sideband behavior driver will be used to trigger the [soft off behavior](../../../keymaps/behaviors/soft-off.md) "out of band" from the normal keymap processing. To do so, it will decorate/wrap an underlying kscan driver.

With a simple direct pin setup, the [direct kscan](../../../config/kscan.md) driver can be used with a [GPIO key](#gpio-key), to make a small "side matrix":

```dts
/ {
    wakeup_scan: wakeup_scan {
        compatible = "zmk,kscan-gpio-direct";
        input-keys = <&soft_off_gpio_key>;
        wakeup-source;
    };
};
```

With that in place, the kscan sideband behavior will wrap the new driver:

```dts
/ {
    side_band_behavior_triggers: side_band_behavior_triggers {
        compatible = "zmk,kscan-sideband-behaviors";

        kscan = <&wakeup_scan>;
        auto-enable;
        wakeup-source;

        soft_off {
            column = <0>;
            row = <0>;
            bindings = <&hw_soft_off>;
        };
    };
};
```

As the kscan used only has a single key, both column and row are set to 0. The properties of the `kscan-sideband-behaviors` node can be found in the [appropriate configuration section](../../../config/kscan.md#kscan-sideband-behavior-driver).
