# nice!view Adapter

This shield is used as an adapter between the nice!view and existing shields/boards that expose an I2C OLED header.

To use this shield, you should add this shield to your list of shields *before* `nice_view`.

The nice!view will use the SDA/SCL pins of the OLED, and then the adapter expects a final pin to be "bodged" from your microcontroller to the nice!view CS pin. This adapter assumes that the CS pin bodged is the `&pro_micro 1` pin or "D1", which is the top left pin when looking at the front of the board. If you can't use this pin, you'll need to override the `cs-gpios` for the `&nice_view_spi` bus (in your `zmk-config` keymap for example) or you will want to define your own `&nice_view_spi` bus without using this adapter.

```
west build -b nice_nano_v2 -- -DSHIELD="lily58_left nice_view_adapter nice_view"
```
