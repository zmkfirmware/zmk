# TIDBIT Compatibility Notes

- The top-left and top-right encoders share the same pins. Install only one, and enable/include EITHER `encoder_1` OR `encoder_1_top_row` in your keymap; not both.
- `encoder_3` cannot be used at the same time as the OLED and/or HT16K33 modules, as it is wired to the same pins.
  - While the HT16K33 hardware is supported by Zephyr, functionality may not have been implemented in ZMK for it.
- `encoder_4` cannot be used at the same time as the TRRS jack, as it is wired to the same pins.
