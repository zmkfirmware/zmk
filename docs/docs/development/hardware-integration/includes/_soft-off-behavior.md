### Soft off behavior instance

Behind the scenes, a hardware dedicated GPIO pin utilizes the [soft off behavior](../../../keymaps/behaviors/soft-off.md) to trigger entering the soft-off state. To use said behavior outside of a keymap, add an instance of the behavior to your `.overlay`/`.dts` file:

```dts
/ {
    behaviors {
        hw_soft_off: hw_soft_off {
            compatible = "zmk,behavior-soft-off";
            #binding-cells = <0>;
            split-peripheral-off-on-press; // Turn peripheral off immediately for reliability
            hold-time-ms = <2000>; // Only turn off if the key is held for 2 seconds or longer.
        };
    };
};
```
