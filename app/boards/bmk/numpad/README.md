# BMK Numpad

An nRF52840 number pad with an IS31FL3737 LED matrix and an ST75256 LCD.

## Building numpad ZMK firmware

The LCD requires the ST75256 driver. If your Zephyr checkout already contains `drivers/display/display_st75256.c`, go straight to the build command. On a fresh workspace, run these commands from the repository root to apply the bundled display driver first.

```sh
git -C zephyr apply --check "$PWD/app/boards/bmk/numpad/st75256-zephyr.patch"
git -C zephyr apply "$PWD/app/boards/bmk/numpad/st75256-zephyr.patch"
```

```sh
west build -s app -b numpad//zmk -d build/numpad-normal
```

## Calculator

[Board-local calculator usage, porting and tests](calculator/README.md).

## LCD backlight diagnostics

[Read-only probe, firmware variants, measurement procedure and acceptance checks](diagnostics/README.md).

## ZMK Studio (normal firmware)

The normal firmware enables Studio by default. The board includes a USB CDC ACM transport alongside the USB HID keyboard, so the build does not need a separate `studio-rpc-usb-uart` snippet.

Connect with a USB data cable. If Bluetooth is the active output, press **Fn + decimal point** to select USB. In Studio, select the keyboard's serial port and press **Fn + 0** to unlock it. The unlock key works in Base, Media, and calculator modes. The device locks on disconnect or after 600 seconds of inactivity. Keep the Fn and unlock bindings accessible when remapping. The calculator layer IDs and their dedicated bindings are needed for **Fn + Num Lock**, **Fn + Delete**, and **Fn + Enter**.

The backlight diagnostic builds set `CONFIG_ZMK_STUDIO=n`. The isolated build also disables settings, automatic display blanking, and device power management. These builds have no Studio USB port and **Fn + 0** has no action. Flashing firmware preserves saved settings, so a keymap previously saved in Studio can override the factory unlock binding.
