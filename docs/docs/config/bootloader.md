---
title: Bootloader Integration Configuration
sidebar_label: Bootloader Integration
---

These are general settings that control the various bootloader integration features of ZMK.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/src/boot/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/src/boot/Kconfig)

### Double Tap To Bootloader

Some SoCs, like RP2040 or the various STM32 offerings, require holding a certain "boot button" down to enter the bootloader, on a hardware level. To make it easier to enter the bootloader on those platforms, ZMK integrates an optional feature to allow entering the bootloader by simply double tapping reset within the configured timeout period.

| Config                                     | Type | Description                                                         | Default                     |
| ------------------------------------------ | ---- | ------------------------------------------------------------------- | --------------------------- |
| `CONFIG_ZMK_DBL_TAP_BOOTLOADER`            | bool | Enable the double-tap to enter bootloader functionality             | y if STM32 or RP2040/RP2350 |
| `CONFIG_ZMK_DBL_TAP_BOOTLOADER_TIMEOUT_MS` | int  | Duration (in ms) to wait for a second reset to enter the bootloader | 500                         |

### STM32 nBOOT_SEL Option Byte Setup

Some newer STM32 series SoCs, in particular stm32c0 and stm32g0, enable the `nBOOT_SEL` bit of the option bytes by default. This bit prevents entering the system ROM bootloader by holding the BOOT0 pin/button during a reset/startup.

To ensure the BOOT button on keyboard and controllers using these SoCs works as expected after being flashed with ZMK, we check the `nBOOT_SEL` bit on startup and clear it if it is set. Should you _not_ want that functionality, for some reason, this can be disabled.

| Config                                    | Type | Description                           | Default                 |
| ----------------------------------------- | ---- | ------------------------------------- | ----------------------- |
| `CONFIG_ZMK_BOOT_STM32_ENFORCE_NBOOT_SEL` | bool | Ensure the `nBOOT_SEL` bit is not set | y if STM32CO or STM32G0 |

### Bootmode Magic Value Mapper

Some target SoCs may use the bootmode magic value mapper for [bootloader integration](docs/development/hardware-integration/bootloader/index.mdx). When doing so, the following configurations are used:

| Config                                                           | Type | Description                                                                           | Default |
| ---------------------------------------------------------------- | ---- | ------------------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_BOOTMODE_BOOTLOADER_MAGIC_VALUE`                     | hex  | The magic value to place into retained memory when the bootloader boot mode is set    | none    |
| `CONFIG_ZMK_BOOTMODE_MAGIC_VALUE_BOOTLOADER_TYPE_TINYUF2`        | bool | Used to default the bootloader magic value for the tinyuf2 bootloader                 | false   |
| `CONFIG_ZMK_BOOTMODE_MAGIC_VALUE_BOOTLOADER_TYPE_ADAFRUIT_BOSSA` | bool | Used to default the bootloader magic value for the Adafruit BOSSA (SAMD21) bootloader | false   |
| `CONFIG_ZMK_BOOTMODE_MAGIC_VALUE_BOOTLOADER_TYPE_ADAFRUIT_NRF52` | bool | Used to default the bootloader magic value for the Adafruit nRF52 bootloader          | false   |
