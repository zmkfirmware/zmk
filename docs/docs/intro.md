---
id: intro
title: Introduction to ZMK
sidebar_label: Introduction
---

ZMK Firmware is an open source (MIT) keyboard
firmware built on the [Zephyr Project](https://zephyrproject.com/) RTOS.

The goal is to provider a powerful, featureful keyboard firmware that is free
of licensing issues that prevent upstream BLE support as a first-class
feature.

## Features

At this point, ZMK is _missing_ more features than it has. Currently, the mostly working bits
include:

- HID Over GATT (HOG) - This is the official term for BLE HID devices.
- Keymaps and layers with basic keycodes.
- Some initial work on one "action", Mod-Tap.
- Basic HID over USB - This somehow _conflicts_ with BLE at least on the stm32wb55rg dev kit, so investigation needed.

## Missing Features

- Consumer Key Support (play/pause, etc)
- One Shot
- Layer Tap
- Split support
- Encoders
- Battery reporting
- Low power mode.
- Shell over BLE
