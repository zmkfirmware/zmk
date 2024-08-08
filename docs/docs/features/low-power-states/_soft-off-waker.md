#### Soft off waker

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

Here are the properties for the node:

- The `compatible` property for the node must be `zmk,gpio-key-wakeup-trigger`.
- The `trigger` property is a phandle to the GPIO key defined earlier.
- The `wakeup-source` property signals to Zephyr this device should not be suspended during the shutdown procedure.
- The `extra-gpios` property contains the list of GPIO pins (including the appropriate flags) to set active before going into power off, to ensure the GPIO pin will trigger properly to wake the keyboard. For example, for a `col2row` matrix kscan, these are the column pins relevant for soft off.
