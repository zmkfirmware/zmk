---
title: Persistent Settings
sidebar_label: Settings
---

ZMK uses [Zephyr's settings subsystem](https://docs.zephyrproject.org/3.5.0/services/settings/index.html) to store certain runtime settings in the "storage" partition of the controller's flash memory.
These settings will be saved after certain events and loaded on boot.
For instance, bond information for [paired Bluetooth hosts](../features/bluetooth.md) are stored in this partition so that users do not need to pair to each device again after the controller loses power.

Persisted settings are **not** cleared by flashing regular ZMK firmware: this is by design, since modifications like keymap changes should not cause users to lose their Bluetooth pairings.
They can only be cleared by setting a special Kconfig symbol or flashing a special firmware build as documented below.

Below is a non-comprehensive list of ZMK features that utilize persisted settings.

- [Bluetooth](../features/bluetooth.md): Stores pairing keys and MAC addresses associated with hosts, BT profile selected through the [keymap behavior](../keymaps/behaviors/bluetooth.md)[^1]
- [Split keyboards](../features/split-keyboards.md): Stores pairing keys and MAC addresses for wireless connection between parts
- [Output selection](../keymaps/behaviors/outputs.md): Stores last selected preferred endpoint changed through the keymap behavior[^1]
- [ZMK Studio](../features/studio.md): Stores any runtime keymap modifications and selected physical layouts after they are saved to the keyboard
- [Lighting](../features/lighting.md): Stores current brightness/color/effects for [underglow](../keymaps/behaviors/underglow.md) and [backlight](../keymaps/behaviors/backlight.md) features after being changed through their keymap behaviors[^1]
- [Power management](../keymaps/behaviors/power.md): Stores the state of the external power toggle as changed through the keymap behavior[^1]

[^1]: These are not saved immediately, but after `CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE` milliseconds in order to reduce potential wear on the flash memory.

## Kconfig

See [Configuration Overview](index.md) for instructions on how to change these settings.

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                               | Type | Description                                                                   | Default |
| ------------------------------------ | ---- | ----------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_SETTINGS_RESET_ON_START` | bool | Clears all persistent settings from the keyboard at startup                   | n       |
| `CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE`  | int  | Milliseconds to wait after a setting change before writing it to flash memory | 60000   |

## Clearing Persisted Settings

While regular ZMK builds will not cause any settings to be cleared upon flashing, flashing a build with `CONFIG_ZMK_SETTINGS_RESET_ON_START` enabled as documented above will cause the firmware to run a special procedure when the controller starts that clears the settings partition.

For end users, it is recommended to use a special [shield](../development/hardware-integration/index.mdx#boards--shields) named `settings_reset` to build a new firmware file, then flash that firmware.
See example for building firmware using this shield in the [troubleshooting docs](../troubleshooting/connection-issues.mdx#building-a-reset-firmware).

In both cases, regular, non-reset firmware will need to be flashed afterwards for normal operation.

:::warning

Since pairing information between split keyboards are also cleared with this process, you will need to clear settings on all parts of a split keyboard.
Please follow the full procedure described in [troubleshooting](../troubleshooting/connection-issues.mdx#split-keyboard-parts-unable-to-pair) so that the parts can pair correctly after clearing.

:::

:::tip

[ZMK Studio](../features/studio.md)-specific settings can be easily cleared using the "Restore Stock Settings" button in the header of the Studio client.

:::
