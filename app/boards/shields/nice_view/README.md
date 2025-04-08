# nice!view

The nice!view is a low-power, high refresh rate display meant to replace I2C OLEDs traditionally used.

This shield requires that an `&nice_view_spi` labeled SPI bus is provided with _at least_ MOSI, SCK, and CS pins defined.

## Custom widget

The nice!view shield includes a custom vertical widget.

Profile indicators show the status of the first five BLE profiles using numbers from 1 to 5.
The number corresponding to the currently selected profile is drawn in a filled disk
and the circle outline around each profile number correspond to the following states:

- solid outline: connected
- dashed outline: not connected
- no outline: not bound

## Disable custom widget

To use the built-in ZMK widget instead of the custom nice!view one, add the following item to your `.conf` file:

```
CONFIG_ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN=y
CONFIG_ZMK_LV_FONT_DEFAULT_SMALL_MONTSERRAT_26=y
CONFIG_LV_FONT_DEFAULT_MONTSERRAT_26=y
```
