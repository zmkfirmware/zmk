Hillside is a split ergonomic keyboard with 3x6+4+2 choc-spaced keys.
It has the aggressive stagger of the Ferris but a longer thumb arc and a break-off outer pinky column.
More information is at [github/mmccoyd](https://github.com/mmccoyd/hillside/).

The default keymap is described in
  [QMK](https://github.com/qmk/qmk_firmware/tree/master/keyboards/handwired/hillside/keymaps/default).
For ZMK, the adjust layer has extra keys for bluetooth, reset, output, and a lack of RGB controls.

If used, the following must be manually enabled in hillside.conf:
- Encoders
- Underglow

If desired, you could hardwire a display to the I2C header,
  which is arranged for a haptic feedback board.
