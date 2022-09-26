# nice!view Adapter

This shield is used as an adapter between the nice!view and existing shields/boards that expose an I2C OLED header.

To use this shield, you should add this shield to your list of shields *before* `nice_view`.

The nice!view will use the SDA/SCL pins of the OLED, and then the adapter expects a final pin to be "bodged" from your microcontroller to the nice!view CS pin.

```
west build -b nice_nano_v2 -- -DSHIELD="lily58_left nice_view_adapter nice_view"
```
