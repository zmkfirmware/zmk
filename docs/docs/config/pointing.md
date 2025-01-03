---
title: Pointing Device Configuration
sidebar_label: Pointing
---

These are settings related to the pointing device/mouse support in ZMK.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/pointing/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/pointing/Kconfig)

### General

| Config                                 | Type | Description                                                                | Default |
| -------------------------------------- | ---- | -------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_POINTING`                  | bool | Enable the general pointing/mouse functionality                            | n       |
| `CONFIG_ZMK_POINTING_SMOOTH_SCROLLING` | bool | Enable smooth scrolling HID functionality (via HID Resolution Multipliers) | n       |

### Advanced Settings

The following settings are from Zephyr and should be defaulted to sane values, but can be adjusted if you encounter problems.

| Config                           | Type | Description                                                | Default                         |
| -------------------------------- | ---- | ---------------------------------------------------------- | ------------------------------- |
| `CONFIG_INPUT_THREAD_STACK_SIZE` | int  | Stack size for the dedicated input event processing thread | 512 (1024 on split peripherals) |

## Input Listener

The following documents settings related to [input listeners](../features/pointing.md#input-listeners).

### Devicetree

Applies to: `compatible = "zmk,input-listener"`

Definition file: [zmk/app/dts/bindings/zmk,input-listener.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Cinput-listener.yaml)

| Property           | Type          | Description                                                         |
| ------------------ | ------------- | ------------------------------------------------------------------- |
| `device`           | phandle       | Input device handle                                                 |
| `input-processors` | phandle-array | List of input processors (with parameters) to apply to input events |

#### Child Properties

Additional properties can be set on child nodes, which allows changing the settings when certain layers are enabled:

| Property           | Type          | Description                                                                                |
| ------------------ | ------------- | ------------------------------------------------------------------------------------------ |
| `layers`           | array         | List of layer indexes. This config will apply if any layer in the list is active.          |
| `input-processors` | phandle-array | List of input processors (with parameters) to apply to input events                        |
| `process-next`     | bool          | Whether to continue applying other input processors after this override if it takes effect |

## Input Split

Input splits are used for [pointing devices on split peripherals](../development/hardware-integration/pointing.mdx#listener-and-input-split-device).

### Devicetree

Applies to: `compatible = "zmk,input-split"`

Definition file: [zmk/app/dts/bindings/zmk,input-split.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Cinput-split.yaml)

| Property           | Type          | Description                                                         |
| ------------------ | ------------- | ------------------------------------------------------------------- |
| `device`           | handle        | Input device handle                                                 |
| `input-processors` | phandle-array | List of input processors (with parameters) to apply to input events |
