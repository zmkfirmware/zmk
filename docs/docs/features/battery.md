---
title: Battery Level
sidebar_label: Battery Level
---

If your keyboard has a battery sensor, ZMK will report its battery level to the connected bluetooth host and show it on the keyboard's display, if it has one.

For [split keyboards](split-keyboards.md), only the battery level of the central (usually left) side is reported over bluetooth by default. ZMK can be [configured to report the battery levels for peripherals](../config/battery.md#peripheral-battery-monitoring), but not many host systems will display this information without additional configuration or the use of third party utilities.

:::note

Windows may not properly ask the keyboard to notify it of changes in battery level, so the level shown may be out of date.

:::

## Adding a Battery Sensor to a Board

If your keyboard is using one of the [boards supported in ZMK](../hardware.mdx) it will already be configured to sense and report battery levels.
If you are using a custom board, see [battery sensing hardware integration page](../development/hardware-integration/battery.md) to add support.
