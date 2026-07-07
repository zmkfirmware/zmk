# Derivative Rev A

A 48-key wireless keyboard based on the HNKB40, powered by the nRF52840 SoC.

## Differences from HNKB40

- **48 keys** (6+12+11+11+8 layout) — 6 extra keys on a row above HNKB40
- **51 LEDs** total: 3 indicators (0-2) + 48 per-key (3-50)
- **No underglow** LEDs

## Hardware

- **MCU:** nRF52840
- **Key Matrix:** 48 keys using 6 x 74HC595A shift registers (daisy-chained)
- **Per-Key RGB:** WS2812 LED strip (51 LEDs)
- **Connectivity:** USB and Bluetooth Low Energy (BLE)

## Building

```
west build -p -b derivative-rev-a/nrf52840/zmk
```

## Flashing

The board uses UF2 bootloader. Copy the generated `zmk.uf2` file to the mounted drive when in bootloader mode.
