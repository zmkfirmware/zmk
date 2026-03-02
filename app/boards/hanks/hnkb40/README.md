# HNKB40

A 40% wireless keyboard powered by the nRF52840 SoC.

## Hardware

- **MCU:** nRF52840
- **Key Matrix:** 48 keys using 6 x 74HC595A shift registers (daisy-chained)
- **RGB Underglow:** WS2812 LED strip (12 LEDs)
- **Connectivity:** USB and Bluetooth Low Energy (BLE)
- **Power:** Battery with voltage divider for monitoring, DCDC regulator enabled

### Shift Register Configuration

The keyboard uses 6 daisy-chained 74HC595A shift registers to scan 48 key positions with minimal GPIO usage:

| Signal | Pin   | Description                        |
| ------ | ----- | ---------------------------------- |
| SER    | P1.14 | Serial data to shift registers     |
| SCK    | P1.13 | Shift register clock (SRCLK)       |
| RCLK   | VCC   | Latch clock (directly tied to VCC) |
| Sense  | P1.15 | Key sense input (active LOW)       |

### How It Works

The `zmk,kscan-595a` driver uses GPIO bit-banging to scan keys:

1. Fill shift registers with 1s (all columns inactive/HIGH)
2. Shift in a single 0 (active column LOW)
3. Read sense pin - if LOW, key is pressed
4. Shift the 0 to the next column by clocking in a 1
5. Repeat for all 48 columns

Since RCLK is tied to VCC, outputs update immediately on each clock edge.

## Building

```
west build -p -b hnkb40/nrf52840/zmk
```

## Flashing

The board uses UF2 bootloader. Copy the generated `zmk.uf2` file to the mounted drive when in bootloader mode.
