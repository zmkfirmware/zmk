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

At this point, ZMK is still missing many features compared to popular firmware such as QMK. Here is a table comparing supported ZMK and QMK features:
 
|   **Feature**                                                                                          |   &nbsp  ZMK    &nbsp |    &nbsp  QMK   &nbsp   |
|----------------------------------------------------------------------------------------------------------|:---------:|:-----------:|
| Multi-Device BLE Support                                                                                 |     ✅     |              |
| USB Connectivity                                                                                         |     ✅     |      ✅      |
| Low Active Power Usage                                                                                   |     ✅     |              |
| Split Keyboard Support                                                                                   |     ✅     |      ✅      |
| [Keymaps and Layers](behavior/layers)                                                                    |     ✅     |      ✅      |
| [Hold-tap](behavior/hold-tap) (which includes [mod-tap](behavior/mod-tap), [layer-tap](behavior/layers/#layer-tap)) |     ✅     |      ✅      |
| [Basic Keycodes](behavior/key-press)                                                                     |     ✅     |      ✅      |
| [Basic consumer (media) keycodes](behavior/key-press#consumer-key-press)                                 |     ✅     |      ✅      |
| [Encoders](feature/encoders)[^1]                                                                            |     ✅     |      ✅      |
| Proof of Concept [OLED display support](feature/displays)                                                |     ✅     |      ✅      |
| [RGB Underglow](feature/underglow)                                                                       |     ✅     |      ✅      |
| One Shot Keys                                                                                            |  In Dev         |      ✅      |
| Combo Keys                                                                                               |  In Dev         |      ✅      |
| Macros                                                                                                   |  In Dev         |      ✅      |
| Mouse Keys                                                                                               |                 |      ✅      |
| Battery Reporting                                                                                        | In Dev          |             |
| Low Power Sleep States                                                                                   | In Dev          |             |
| Low Power Mode (VCC Shutoff)                                                                             | In Dev          |             |
| Shell over BLE                                                                                           |                 |             |
| Realtime Keymap Updating                                                                                 |     Planned    |      ✅       |
| AVR/8 Bit                                                                                                |                |      ✅      |
| Wide Range of ARM Chips Supported                                                                        |    ✅         |              |
[^1]:Note: Encoders are not currently supported on peripheral side splits.


## Code Of Conduct

Please note that this project is released with a
[Contributor Code of Conduct](https://www.contributor-covenant.org/version/2/0/code_of_conduct/).
By participating in this project you agree to abide by its terms.
