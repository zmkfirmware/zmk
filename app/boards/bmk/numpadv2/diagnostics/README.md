# numpadv2 Flash/NVS LED diagnostic

This diagnostic writes an isolated `numpadv2_diag/flash_marker` settings value and reports the
result through the LED Matrix profile 0 palette:

- Red: the settings write failed, or a corrupt marker was loaded.
- Yellow: no marker was loaded and a new marker was written successfully. Wait at least five
  seconds, then remove and restore power.
- Green: the marker was loaded and validated after reboot.
- Yellow after every reboot: the write call succeeds, but the marker is not retained or loaded.

Build the current `0x80000` storage layout:

```sh
west build -s app -p -d app/_bmkbuild/numpadv2_flash_diag_80000 \
  -b numpadv2/nrf52840/zmk -- \
  -DEXTRA_CONF_FILE=boards/bmk/numpadv2/diagnostics/flash_diag.conf
```

Build the former `0xD4000` storage layout:

```sh
west build -s app -p -d app/_bmkbuild/numpadv2_flash_diag_d4000 \
  -b numpadv2/nrf52840/zmk -- \
  -DEXTRA_CONF_FILE=boards/bmk/numpadv2/diagnostics/flash_diag.conf \
  -DDTC_OVERLAY_FILE=boards/bmk/numpadv2/diagnostics/flash_diag_d4000.overlay
```

The diagnostic changes the visible LED Matrix state. Reflash normal firmware and clear settings
after testing before reconnecting or pairing the keyboard for normal use.
