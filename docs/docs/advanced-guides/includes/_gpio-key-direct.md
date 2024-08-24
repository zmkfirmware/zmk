Here is an example for a keyboard with a dedicated on/off push button that is a direct wire between the GPIO pin and ground:

```dts
/ {
    keys {
        compatible = "gpio-keys";
        soft_off_gpio_key: soft_off_gpio_key {
            gpios = <&gpio0 2 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
        };
    };
};
```

In the above example the soft on/off would be triggered by pulling the specified pin low, typically by pressing a switch that has the other leg connected to ground.
