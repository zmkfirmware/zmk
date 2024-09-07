### Soft off waker

We need to add another device which will be enabled only when the keyboard is going into soft off state, and will configure the previously declared GPIO key with the correct interrupt configuration to wake the device from soft off once it is pressed.

```dts
/ {
    wakeup_scan: wakeup_scan {
        compatible = "zmk,gpio-key-wakeup-trigger";
        trigger = <&soft_off_gpio_key>;
        wakeup-source;
        extra-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>
    };
};
```

The properties for the `gpio-key-wakeup-trigger` node can be found in the [appropriate configuration section](../../../config/power.md#gpio-key-wakeup-trigger).
