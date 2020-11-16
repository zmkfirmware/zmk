---
title: Introduction to ZMK
sidebar_label: Introduction
slug: /
---

ZMK Firmware is an open source (MIT) keyboard
firmware built on the [Zephyr™ Project](https://zephyrproject.org/) Real Time Operating System (RTOS). ZMK's goal is to provide a modern, wireless, and powerful firmware free of licensing issues.

## Features

ZMK is currently missing some features found in other popular firmware. This table compares the features supported by ZMK, BlueMicro and QMK:

| **Feature**                                                                                                            | ZMK | BlueMicro | QMK |
| ---------------------------------------------------------------------------------------------------------------------- | :-: | :-------: | :-: |
| Low Latency BLE Support                                                                                                | ✅  |    ✅     |     |
| Multi-Device BLE Support                                                                                               | ✅  |           |     |
| [USB Connectivity](behavior/outputs)                                                                                   | ✅  |           | ✅  |
| User Configuration Repositories                                                                                        | ✅  |           |     |
| Split Keyboard Support                                                                                                 | ✅  |    ✅     | ✅  |
| [Keymaps and Layers](behavior/layers)                                                                                  | ✅  |    ✅     | ✅  |
| [Hold-Tap](behavior/hold-tap) (which includes [Mod-Tap](behavior/mod-tap) and [Layer-Tap](behavior/layers/#layer-tap)) | ✅  |    ✅     | ✅  |
| [Keyboard Codes](codes/#keyboard)                                                                                      | ✅  |    ✅     | ✅  |
| [Media](codes/#media-controls) & [Consumer](codes/#consumer-controls) Codes                                            | ✅  |    ✅     | ✅  |
| [Encoders](feature/encoders)[^1]                                                                                       | ✅  |           | ✅  |
| [OLED Display Support](feature/displays)[^2]                                                                           | 🚧  |    🚧     | ✅  |
| [RGB Underglow](feature/underglow)                                                                                     | ✅  |    ✅     | ✅  |
| One Shot Keys                                                                                                          | 🚧  |    ✅     | ✅  |
| Combo Keys                                                                                                             | 🚧  |           | ✅  |
| Macros                                                                                                                 | 🚧  |    ✅     | ✅  |
| Mouse Keys                                                                                                             |     |    ✅     | ✅  |
| Low Active Power Usage                                                                                                 | ✅  |           |     |
| [Low Power Sleep States](https://github.com/zmkfirmware/zmk/pull/211)                                                  | 🚧  |    ✅     |     |
| [Low Power Mode (VCC Shutoff)](https://github.com/zmkfirmware/zmk/pull/242)                                            | 🚧  |           |     |
| [Battery Reporting](https://github.com/zmkfirmware/zmk/issues/47)                                                      | 🚧  |    ✅     |     |
| Shell over BLE                                                                                                         |     |           |     |
| Realtime Keymap Updating                                                                                               | 💡  |           | ✅  |
| AVR/8 Bit                                                                                                              |     |           | ✅  |
| [Wide Range of ARM Chips Supported](https://docs.zephyrproject.org/latest/boards/index.html)                           | ✅  |           |     |

[^2]: Encoders are not currently supported on peripheral side splits.
[^1]: OLEDs are currently proof of concept in ZMK.

## Code Of Conduct

Please note that this project is released with a
[Contributor Code of Conduct](https://www.contributor-covenant.org/version/2/0/code_of_conduct/).
By participating in this project you agree to abide by its terms.
