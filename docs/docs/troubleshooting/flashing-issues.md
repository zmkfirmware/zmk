---
title: Flashing Issues
sidebar_label: Flashing Issues
description: Troubleshooting issues when flashing ZMK firmware to devices.
---

## File Transfer Error

Variations of the warnings shown below occur when flashing the `<firmware>.uf2` onto the microcontroller. This is because the microcontroller resets itself before the OS receives confirmation that the file transfer is complete. Errors like this are normal and can generally be ignored. Verification of a functional board can be done by attempting to pair your newly flashed keyboard to your computer via Bluetooth or plugging in a USB cable if `ZMK_USB` is enabled in your Kconfig.defconfig.

| ![Example Error Screen](../../docs/assets/troubleshooting/filetransfer/windows.png) |
| :---------------------------------------------------------------------------------: |
|                 An example of the file transfer error on Windows 10                 |

| ![Example Error Screen](../../docs/assets/troubleshooting/filetransfer/linux.png) |
| :-------------------------------------------------------------------------------: |
|                  An example of the file transfer error on Linux                   |

| ![Example Error Screen](../../docs/assets/troubleshooting/filetransfer/mac.png) |
| :-----------------------------------------------------------------------------: |
|                 An example of the file transfer error on macOS                  |

## macOS Ventura Error

macOS 13.0 (Ventura) Finder may report an error code 100093 when copying `<firmware>.uf2` files into microcontrollers. This bug is limited to the operating system's Finder. You can work around it by copying on Terminal command line or use a third party file manager. Issue is fixed in macOS version 13.1.

## macOS Sonoma Error

macOS 14.x (Sonoma) Finder may report an "Error code -36" when copying `<firmware>.uf2` files into microcontrollers. A similar "fcopyfile failed: Input/output error" will also be reported when copying is performed using Terminal command line. These errors can be ignored because they are reported when the bootloader disconnects automatically after the uf2 file is copied successfully.
