---
title: Pointing Device Configuration
sidebar_label: Pointing
---

These are settings related to the pointing device/mouse support in ZMK.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Kconfig

Definition file: [zmk/app/pointing/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/pointing/Kconfig)

### General

| Config                                  | Type | Description                                                                  | Default |
| --------------------------------------- | ---- | ---------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_POINTING`                   | bool | Enable the general pointing/mouse functionality                              | n       |
| `CONFIG_ZMK_POINTING_SMOOTH_SCROLLING`  | bool | Enable smooth scrolling HID functionality (via HID Resolution Multipliers)   | n       |
| `CONFIG_ZMK_POINTING_SCROLL_RESOLUTION` | int  | Scroll resolution in counts per inch, declared to the host (0 declares none) | 0       |

:::note

`CONFIG_ZMK_POINTING_SMOOTH_SCROLLING` relies on the host writing a HID Resolution
Multiplier feature report. macOS never does: the multiplier usage does not appear
anywhere in IOHIDFamily except its usage tables, so the option has no effect there.

:::

### Scroll resolution

`CONFIG_ZMK_POINTING_SCROLL_RESOLUTION` tells the host how many counts the device
emits per inch of scroll travel. It is declared in the report descriptor as a
physical range, and it is worth setting on any device that scrolls from a sensor
rather than from a notched wheel.

Hosts use the declared resolution to tell a fine grained scroll stream from a
notched wheel, and when nothing is declared they assume the latter. macOS assumes
9 counts per inch, and that assumption has two effects: its high resolution
scroll path stays off, since that is gated on the declared resolution exceeding
twice the default, and its scroll acceleration curve is fed a velocity scaled by
`resolution / report_rate`, so a trackball emitting hundreds of counts per inch is
treated as moving far faster than it is. Scrolling comes out in coarse steps and
accelerates away, however fine the stream from the device is.

Set it to what the device really emits per inch, measured after the input
processors. A 1600 CPI sensor feeding
[`&zip_xy_to_scroll_mapper`](../keymaps/input-processors/code-mapper.md#pre-defined-instances)
directly emits 1600 counts per inch; with
[`&zip_xy_scaler 1 10`](../keymaps/input-processors/scaler.md#pre-defined-instances)
ahead of it, 160. Larger values scroll more slowly, which makes this a speed control
that does not cost any granularity, unlike dividing the stream down with a scaler.

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

Input splits are used for [pointing devices on split peripherals](../hardware-integration/pointing.mdx#listener-and-input-split-device).

### Devicetree

Applies to: `compatible = "zmk,input-split"`

Definition file: [zmk/app/dts/bindings/zmk,input-split.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Cinput-split.yaml)

| Property           | Type          | Description                                                         |
| ------------------ | ------------- | ------------------------------------------------------------------- |
| `device`           | handle        | Input device handle                                                 |
| `input-processors` | phandle-array | List of input processors (with parameters) to apply to input events |
