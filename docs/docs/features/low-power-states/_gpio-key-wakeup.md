Here is an example for a keyboard with a GPIO pin reused from a matrix kscan:

```dts
/ {
    keys {
        compatible = "gpio-keys";
        soft_off_gpio_key: soft_off_gpio_key {
            gpios = <&gpio0 2 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>;
        };
    };
};
```

The GPIO settings should match those in the kscan, so the above would be set to `(GPIO_ACTIVE_LOW | GPIO_PULL_UP)` for a direct kscan.
