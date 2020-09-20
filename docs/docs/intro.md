---
id: intro
title: Introduction to ZMK
sidebar_label: Introduction
---

ZMK Firmware is an open source (MIT) keyboard
firmware built on the [Zephyr™ Project](https://zephyrproject.org/) Real Time Operating System (RTOS).

The goal is to provide a powerful, featureful keyboard firmware that is free
of licensing issues that prevent upstream BLE support as a first-class
feature.

## Features

At this point, ZMK is still missing many features. Currently, the working bits
include:

- Wireless connectivity via BLE HID Over GATT (HOG)
- USB connectivity
- Low active power usage
- Split keyboard support
- [Keymaps and layers](behavior/layers)
- [Hold-tap](behavior/hold-tap) (which includes [mod-tap](behavior/mod-tap), [layer-tap](behavior/layers))
- [Basic HID over USB](behavior/key-press)
- [Basic consumer (media) keycodes](behavior/key-press#consumer-key-press)
- [Encoders](feature/encoders)
- Basic [OLED display support](feature/displays)
- [RGB Underglow](feature/underglow)

## Missing Features

- One Shot Keys
- Combo keys
- Macros
- Complete split support (encoders and RGB are not supported on the 'peripheral' side)
- Battery reporting
- Low power sleep states
- Low power mode (to toggle LEDs and screen off)
- Shell over BLE

## Code Of Conduct

Please note that this project is released with a
[Contributor Code of Conduct](https://www.contributor-covenant.org/version/2/0/code_of_conduct/).
By participating in this project you agree to abide by its terms.
