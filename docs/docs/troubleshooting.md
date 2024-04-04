---
title: Troubleshooting
sidebar_title: Troubleshooting
---

The following page provides suggestions for common errors that may occur during firmware compilation or other issues with keyboard usage. If the information provided is insufficient to resolve the issue, feel free to seek out help from the [ZMK Discord](https://zmk.dev/community/discord/invite).

Please also see [the troubleshooting section](features/bluetooth.md#troubleshooting) under the Bluetooth feature page for issues related to bluetooth.

### File Transfer Error

Variations of the warnings shown below occur when flashing the `<firmware>.uf2` onto the microcontroller. This is because the microcontroller resets itself before the OS receives confirmation that the file transfer is complete. Errors like this are normal and can generally be ignored. Verification of a functional board can be done by attempting to pair your newly flashed keyboard to your computer via Bluetooth or plugging in a USB cable if `ZMK_USB` is enabled in your Kconfig.defconfig.

| ![Example Error Screen](../docs/assets/troubleshooting/filetransfer/windows.png) |
| :------------------------------------------------------------------------------: |
|               An example of the file transfer error on Windows 10                |

| ![Example Error Screen](../docs/assets/troubleshooting/filetransfer/linux.png) |
| :----------------------------------------------------------------------------: |
|                 An example of the file transfer error on Linux                 |

| ![Example Error Screen](../docs/assets/troubleshooting/filetransfer/mac.png) |
| :--------------------------------------------------------------------------: |
|                An example of the file transfer error on macOS                |

### macOS Ventura error

macOS 13.0 (Ventura) Finder may report an error code 100093 when copying `<firmware>.uf2` files into microcontrollers. This bug is limited to the operating system's Finder. You can work around it by copying on Terminal command line or use a third party file manager. Issue is fixed in macOS version 13.1.

### macOS Sonoma error

macOS 14.x (Sonoma) Finder may report an "Error code -36" when copying `<firmware>.uf2` files into microcontrollers. A similar "fcopyfile failed: Input/output error" will also be reported when copying is performed using Terminal command line. These errors can be ignored because they are reported when the bootloader disconnects automatically after the uf2 file is copied successfully.

### CMake Error

An error along the lines of `CMake Error at (zmk directory)/zephyr/cmake/generic_toolchain.cmake:64 (include): include could not find load file:` during firmware compilation indicates that the Zephyr Environment Variables are not properly defined.
For more information, see [toolchain setup documentation](../docs/development/setup.mdx).

### West Build Errors

West build errors usually indicate syntax problems in the `<keyboard>.keymap` file during the compilation process. The following are some examples and root causes.

:::note
If you are reviewing these errors in the GitHub Actions tab, they can be found in the `West Build` step of the build process.
:::

#### Keymap error

If you get an error stating `Keymap node not found, check a keymap is available and is has compatible = "zmk,keymap" set` this is an indication that the build process cannot find the keymap. Double check that the `<keyboard>.keymap` file is present and has been discovered by the build process. This can be checked by looking for a line in the build log stating `-- Using keymap file: /path/to/keymap/file/<keyboard>.keymap`. Inside the keymap file ensure the keymap node has `compatible = zmk,keymap` and it's not misspelled. For more information see the [Keymap](features/keymaps.mdx) and [Config](config/index.md) documentation.

#### Devicetree errors

A `devicetree error` followed by a reference to the line number on `<keyboard>.keymap` refers to an issue at the exact line position in that file. For example, below error message indicates a missing `;` at line 109 of the `cradio.keymap` file:

```
devicetree error: /__w/zmk-config/zmk-config/config/cradio.keymap:109 (column 4): parse error: expected ';' or ','
```

A `devicetree error` followed by an `empty_file.c` reference with `lacks #binding-cells` string indicates possible problems with improper parameters for specific bindings:

```
devicetree error: <Node /soc/gpio@50000300 in '/tmp/tmp.vJq9sMwkcY/zephyr/misc/empty_file.c'> lacks #binding-cells
```

This error can be triggered by incorrect binding syntax such as `&kp BT_SEL 0` instead of `&bt BT_SEL 0`.

A `devicetree_generated.h` error that follows with an "undeclared here" string indicates a problem with key bindings, like behavior nodes (e.g. `&kp` or `&mt`) with incorrect number of parameters:

```
/__w/zmk-config/zmk-config/build/zephyr/include/generated/devicetree_generated.h:3756:145: error: 'DT_N_S_keymap_S_symbol_layer_P_bindings_IDX_12_PH_P_label' undeclared here (not in a function); did you mean 'DT_N_S_keymap_S_symbol_layer_P_bindings_IDX_16_PH'?
```

In this example, the error string `DT_N_S_keymap_S_symbol_layer_P_bindings_IDX_12_PH_P_label` indicates a problem with the key binding in position `12` in the `symbol_layer` of the keymap.

:::info
Key positions are numbered starting from `0` at the top left key on the keymap, incrementing horizontally, row by row.
:::

:::tip
A common mistake that leads to this error is to use [key press keycodes](behaviors/key-press.md) without the leading `&kp` binding. That is, having entries such as `SPACE` that should have been `&kp SPACE`.
:::

### Split Keyboard Halves Unable to Pair

Split keyboard halves will automatically pair with one another, but there are some cases where this breaks, and the pairing needs to be reset, for example:

- Switching which halves are the central/peripheral.
- Replacing the controller for one of the halves.

These issues can be resolved by flashing a settings reset firmware to both controllers.

:::warning

This procedure will erase all settings, such as Bluetooth profiles, output selection, RGB underglow color, etc.

:::

First, acquire the reset UF2 image file with one of the following options:

#### Option 1: Build Reset UF2 in 'zmk-config'

Find the `build.yaml` file in your `zmk-config` folder and add an additional settings reset build for the board used by your split keyboard. For example assuming that the config repo is setup for nice!nano v2 with Corne, append the `settings_reset` shield to the `build.yaml` file as follows:

```yml
include:
  - board: nice_nano_v2
    shield: corne_left
  - board: nice_nano_v2
    shield: corne_right
  - board: nice_nano_v2
    shield: settings_reset
```

Save the file, commit the changes and push them to GitHub. Download the new firmware zip file build by the latest GitHub Actions job. In it you will find an additional `settings_reset` UF2 image file.

#### Option 2: Download Reset UF2 from ZMK's Workflow

1. [Open the GitHub `Actions` tab and select the `Build` workflow](https://github.com/zmkfirmware/zmk/actions?query=workflow%3ABuild+branch%3Amain+event%3Apush).
1. Find one of the 'results' for which the core-coverage job was successfully run, indicated by a green checkmark in the core-coverage bubble like the image example below.
1. From the next page under "Artifacts", download and unzip the `<board_name>-settings_reset-zmk` zip file for the UF2 image.

| ![Successful core-coverage Job](../docs/assets/troubleshooting/splitpairing/corecoverage.png) |
| :-------------------------------------------------------------------------------------------: |
|  An example of a successful core-coverage job which will produce a settings_reset firmware.   |

#### Reset Split Keyboard Procedure

Perform the following steps to reset both halves of your split keyboard:

1. Put each half of the split keyboard into bootloader mode.
1. Flash one of the halves of the split with the downloaded settings reset UF2 image.
1. Repeat step 2 with the other half of the split keyboard.
1. Flash the actual image for each half of the split keyboard (e.g `my_board_left.uf2` to the left half, `my_board_right.uf2` to the right half).

After completing these steps, pair the halves of the split keyboard together by resetting them at the same time. Most commonly, this is done by grounding the reset pins for each of your keyboard's microcontrollers or pressing the reset buttons at the same time.

Once this is done, you can remove/forget the keyboard on each host device and pair it again.

:::info

The settings reset firmware has Bluetooth disabled to prevent the two sides from automatically re-pairing until you are done resetting them both. You will not be able to pair your keyboard or see it in any Bluetooth device lists until you have flashed the normal firmware again.

:::
