With a simple direct pin setup, the [direct kscan](../../config/kscan.md) driver can be used with a [GPIO key](#gpio-key), to make a small "side matrix":

```dts
/ {
    wakeup_scan: wakeup_scan {
        compatible = "zmk,kscan-gpio-direct";
        input-keys = <&soft_off_gpio_key>;
        wakeup-source;
    };
};
```

The kscan sideband behavior needs to wrap the new driver due to technical reasons:

```dts
/ {
    side_band_behavior_triggers: side_band_behavior_triggers {
        compatible = "zmk,kscan-sideband-behaviors";
        kscan = <&wakeup_scan>;
        auto-enable;
        wakeup-source;
    };
};
```
