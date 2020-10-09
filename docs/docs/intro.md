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

At this point, ZMK is still missing many features compared to more developed firmwares such as QMK. Here is a table comparing supported ZMK and QMK features:
 
|                                                                                             |    ZMK    |     QMK     |
|----------------------------------------------------------------------------------------------------------|:---------:|:-----------:|
| Wireless Connectivity via BLE                                                                            |     ✅     |             |
| USB Connectivity                                                                                         |     ✅     |      ✅      |
| Low Active Power Usage                                                                                   |     ✅     |             |
| Split Keyboard Support                                                                                   |     ✅     |      ✅      |
| [Keymaps and Layers](behavior/layers)                                                                    |     ✅     |      ✅      |
| [Hold-tap](behavior/hold-tap) (which includes [mod-tap](behavior/mod-tap), [layer-tap](behavior/layers)) |     ✅     |      ✅      |
| [Basic HID over USB](behavior/key-press)                                                                 |     ✅     |      ✅      |
| [Basic consumer (media) keycodes](behavior/key-press#consumer-key-press)                                 |     ✅     |      ✅      |
| [Encoders](feature/encoders)                                                                             |     ✅     |      ✅      |
| Basic [OLED display support](feature/displays)                                                           |     ✅     |      ✅      |
| [RGB Underglow](feature/underglow)                                                                       |     ✅     |      ✅      |
| One Shot Keys                                                                                            |           |      ✅      |
| Combo Keys                                                                                               |           |      ✅      |
| Macros                                                                                                   |           |      ✅      |
| Mouse Keys                                                                                               |           |      ✅      |
| Battery Reporting                                                                                        |           |             |
| Low Power Sleep States                                                                                   |           |             |
| Low Power Mode (VCC Shutoff)                                                                             |           |             |
| Shell over BLE                                                                                           |           |             |
| AVR/8 Bit                                                                                                |           |      ✅      |

## Code Of Conduct

Please note that this project is released with a
[Contributor Code of Conduct](https://www.contributor-covenant.org/version/2/0/code_of_conduct/).
By participating in this project you agree to abide by its terms.
