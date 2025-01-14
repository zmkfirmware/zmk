---
title: Introduction to ZMK
sidebar_label: Introduction
slug: /
---

ZMK Firmware is an open source (MIT) keyboard firmware built on the [Zephyr™ Project](https://zephyrproject.org/) Real Time Operating System (RTOS).
ZMK's goal is to provide a modern and powerful firmware that is wireless-first and free of licensing issues.

## Features

Below table lists major features/capabilities currently supported in ZMK, as well as ones that are currently under development and not planned.

| Legend: | ✅ Supported | 🚧 Under Development | ❌ Not Planned |
| :------ | :----------- | :------------------- | -------------- |

| Hardware                                                                                        | Support |
| ----------------------------------------------------------------------------------------------- | :-----: |
| [Wireless Split Keyboards](features/split-keyboards.md)                                         |   ✅    |
| Wired Split Keyboards                                                                           |   🚧    |
| [Low Active Power Usage](/power-profiler)                                                       |   ✅    |
| [Encoders](features/encoders.md)                                                                |   ✅    |
| [LED-based Lighting](features/lighting.md)                                                      |   ✅    |
| [Displays](features/displays.md)                                                                |   🚧    |
| [Pointing Devices](features/pointing.md)                                                        |   ✅    |
| Multitouch Touchpads (PTP)                                                                      |   🚧    |
| [Low Power Sleep States](features/low-power-states.md)                                          |   ✅    |
| [Low Power Mode (VCC Shutoff) for Peripherals](keymaps/behaviors/power.md)                      |   ✅    |
| Improved Power Handling for Multiple Peripherals                                                |   🚧    |
| [Battery Level Reporting](features/battery.md)                                                  |   ✅    |
| [Support for a Wide Range of ARM Chips](https://docs.zephyrproject.org/3.5.0/boards/index.html) |   ✅    |
| Support for AVR/8-Bit Chips                                                                     |   ❌    |

| Connectivity                                                    | Support |
| --------------------------------------------------------------- | :-----: |
| Low-Latency BLE Support                                         |   ✅    |
| [Multi-Device BLE Connectivity](features/bluetooth.md#profiles) |   ✅    |
| [USB Connectivity](keymaps/behaviors/outputs.md)                |   ✅    |

| Keymap Features                                                                                                                                        | Support |
| ------------------------------------------------------------------------------------------------------------------------------------------------------ | :-----: |
| [User Configuration Repositories](user-setup.mdx)                                                                                                      |   ✅    |
| [Keymaps and Layers](keymaps/index.mdx)                                                                                                                |   ✅    |
| [Wide Range of Keycodes (Including Media and Consumer)](keymaps/list-of-keycodes.mdx)                                                                  |   ✅    |
| [Hold-Taps](keymaps/behaviors/hold-tap.mdx) (including [Mod-Tap](keymaps/behaviors/mod-tap.md) and [Layer-Tap](keymaps/behaviors/layers.md#layer-tap)) |   ✅    |
| [Tap-Dances](keymaps/behaviors/tap-dance.mdx)                                                                                                          |   ✅    |
| [Sticky (One Shot) Keys](keymaps/behaviors/sticky-key.md)                                                                                              |   ✅    |
| [Combos](keymaps/combos.md)                                                                                                                            |   ✅    |
| [Macros](keymaps/behaviors/macros.md)                                                                                                                  |   ✅    |
| [Mouse Keys](keymaps/behaviors/mouse-emulation.md)                                                                                                     |   ✅    |
| [Realtime Keymap Updating](features/studio.md)                                                                                                         |   🚧    |

## Code of Conduct

Please note that this project is released with a [Contributor Code of Conduct](https://www.contributor-covenant.org/version/2/0/code_of_conduct/).
By participating in this project you agree to abide by its terms.
