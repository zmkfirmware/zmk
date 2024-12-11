---
title: Backlight
sidebar_label: Backlight
---

Backlight is a feature used to control an array of LEDs, usually placed through or under switches.

:::info
Unlike [RGB Underglow](underglow.md), backlight can only control single color LEDs. Additionally, because backlight LEDs all receive the same power, it's not possible to dim individual LEDs.
:::

## Enabling Backlight

To enable backlight on your board or shield, add the following line to your `.conf` file of your user config directory as such:

```ini
CONFIG_ZMK_BACKLIGHT=y
```

If your board or shield does not have backlight configured, refer to [Adding Backlight to a board or a shield](#adding-backlight-to-a-board-or-a-shield).

## Configuring Backlight

There are various Kconfig options used to configure the backlight feature.
See [backlight configuration](../config/backlight.md) for details.

## Adding Backlight to a Board or a Shield

See [backlight hardware integration page](../development/hardware-integration/lighting/backlight.mdx) for information on adding backlight support to a ZMK keyboard.
