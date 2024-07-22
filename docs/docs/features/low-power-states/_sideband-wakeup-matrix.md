The kscan sideband behavior needs to wrap the kscan due to technical reasons:

```
/ {
    side_band_behavior_triggers: side_band_behavior_triggers {
        compatible = "zmk,kscan-sideband-behaviors";
        wakeup-source;
        kscan = <&kscan>;
    };
};
```
