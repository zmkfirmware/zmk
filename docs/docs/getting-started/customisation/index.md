---
title: Customizing ZMK/zmk-config folders
sidebar_label: Customizing ZMK
---

After verifying you can successfully flash the default firmware, you will probably want to begin customizing your keymap and other keyboard options.
[In the initial setup tutorial](../user-setup.mdx), you created a Github repository called `zmk-config`. This repository is a discrete filesystem which works
with the main `zmk` firmware repository to build your desired firmware.

By default, the `zmk-config` repo has the following structure:

```
zmk-config/
|- .github/
|- boards/
|- config/
|- zephyr/
-- build.yaml
```

To customize ZMK, the [`build.yaml` file](#yaml-build-matrix) and the [`config` directory](#config-directory) are the main points of interest.

After making any changes you want, commit the changes and then push them to GitHub. That will trigger a new
GitHub Actions job to build your firmware which you can download once it completes. Alternatively, using a [local toolchain](development/local-toolchain/setup/index.md) you could [build your firmware locally](development/local-toolchain/build-flash.mdx).

Follow the same [flashing instructions](../user-setup.mdx#installing-the-firmware) as before to flash your updated firmware.

:::note
If you need to, a review of [Learn The Basics Of Git In Under 10 Minutes](https://www.freecodecamp.org/news/learn-the-basics-of-git-in-under-10-minutes-da548267cc91/) will help you get these steps right.
:::

## Config Directory

```
corne.conf
corne.keymap
lily58.conf
lily58.keymap
```

The purpose of the remaining directories is described below.

- `<keyboard>.conf`
- `<keyboard>.keymap`

However, your config folder can also be modified to include a `boards/` directory for keymaps and configurations for multiple boards/shields
outside of the default keyboard setting definitions.

### Configuration Changes

The setup script creates a `config/<keyboard>.conf` file that allows you to add additional configuration options to
control what features and options are built into your firmware. Opening that file with your text editor will allow you to see the
various config settings that can be commented/uncommented to modify how your firmware is built.

Refer to the [Configuration](/docs/config) documentation for more details on this file.

### Keymap

Once you have the basic user config completed, you can find the keymap file in `config/<keyboard>.keymap` and customize from there.
Refer to the [Keymaps](keymaps/index.mdx) documentation to learn more.

## YAML Build Matrix

### Multiple Keyboards

The `build.yaml` file is used by the GitHub Actions job to identify which keyboards should be built, and allows you to set some additional settings.
You can build multiple keyboards with GitHub actions by appending them to `build.yaml` like so:

```yaml title="build.yaml"
include:
  - board: nice_nano_v2
    shield: corne_left
  - board: nice_nano_v2
    shield: corne_right
  - board: bt60_v1
  - board: seeeduino_xiao_ble
    shield: hummingbird
```

The above would cause the GitHub Actions job to build firmware for the left and right halves of a Corne keyboard (using nice!nano V2 controllers) along with a BT60 V1 Soldered keyboard and a Hummingbird keyboard (using a Seeed Studio XIAO nRF52840 controller).

### Multiple Shields, One Board

Multiple shields can be applied to the same board by listing them one after the other. For example:

```yaml title="build.yaml"
include:
  - board: nice_nano_v2
    shield: corne_left nice_view_adapter nice_view
  # Other boards & shields
```

The above would build firmware for a Corne keyboard's left half with a nice!view installed. The `nice_view_adapter` shield is used as a "bridge", altering the `corne_left`'s configuration to enable the nice!view shield (for the default configuration).

### Snippets

Snippets are a combination of [Kconfig] settings and [devicetree] nodes that are generally independent of the choice of board and shield. In other words, they can be reused across a wide range of keyboards. ZMK provides the following snippets:

- `zmk_usb_logging`, enabling [USB logging](../../development/usb-logging.mdx)
- `studio-rpc-usb-uart`, used to enable [ZMK Studio](../../features/studio.md) for live keymap customisation
- `nrf52833-nosd` and `nrf52840-nosd`, providing additional storage space for the corresponding MCUs at the cost of some [downsides](../../config/system.md#snippets).

To build with a particular snippet, add it to the `snippet` field of a `board` entry:

```yaml
include:
  - board: nice_nano_v2
    shield: corne_left
    snippet: zmk-usb-logging
```

### Artifact Name

By default, the name given to a particular firmware file is `<board>-<shields>.uf2`. The `artifact-name` field can be used to change this. Most commonly, this is used if you are building firmware for the same keyboard multiple times, with a snippet enabled or slightly modified settings. For example:

```yaml
include:
  - board: nice_nano_v2
    shield: corne_left
  - board: nice_nano_v2
    shield: corne_left
    snippet: zmk-usb-logging
    artifact-name: nice_nano_v2_corne_left_usb_logging
```

### CMake Arguments

## Other Directories

### .github Directory

### Boards Directory

### Zephyr Directory

This directory contains a single file, `module.yml`. This file causes your `zmk-config` to be a [Zephyr Module](../../features/modules.mdx), enabling the previously named [boards directory](#boards-directory). More advanced functionality exists; read through our page on [Module Creation](#artifact-name) for details.
