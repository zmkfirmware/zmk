---
id: intro
title: Introduction to ZMK
sidebar_label: Introduction
---

ZMK Firmware is an open source (MIT) keyboard
firmware built on the [Zephyr™ Project](https://zephyrproject.com/) Real Time Operating System (RTOS).

The goal is to provider a powerful, featureful keyboard firmware that is free
of licensing issues that prevent upstream BLE support as a first-class
feature.

## Features

At this point, ZMK is _missing_ more features than it has. Currently, the mostly working bits
include:

- HID Over GATT (HOG) - This is the official term for BLE HID devices
- Keymaps and layers with basic keycodes
- Mod-Tap
- Layer Tap
- Basic HID over USB
- Basic consumer (media) keycodes.
- Basic OLED display logic
- Basic Split support
- Basic RGB Underglow
- Encoders

## Missing Features

- One Shot
- Complete split support
- Battery reporting
- Low power mode
- Shell over BLE
- Macros

## Code Of Conduct

Please note that this project is released with a
[Contributor Code of Conduct](https://www.contributor-covenant.org/version/2/0/code_of_conduct/).
By participating in this project you agree to abide by its terms.
