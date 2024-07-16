---
title: Building Issues
sidebar_label: Building Issues
description: Troubleshooting issues when compiling ZMK firmware.
---

## CMake Error

An error along the lines of `CMake Error at (zmk directory)/zephyr/cmake/generic_toolchain.cmake:64 (include): include could not find load file:` during firmware compilation indicates that the Zephyr Environment Variables are not properly defined.
For more information, see [Zephyr's CMake Package](https://docs.zephyrproject.org/3.5.0/build/zephyr_cmake_package.html).

## West Build Errors

West build errors usually indicate syntax problems in the `<keyboard>.keymap` file during the compilation process. The following are some examples and root causes.

:::note
If you are reviewing these errors in the GitHub Actions tab, they can be found in the `West Build` step of the build process.
:::

### Keymap Error

If you get an error stating `Keymap node not found, check a keymap is available and is has compatible = "zmk,keymap" set` this is an indication that the build process cannot find the keymap. Double check that the `<keyboard>.keymap` file is present and has been discovered by the build process. This can be checked by looking for a line in the build log stating `-- Using keymap file: /path/to/keymap/file/<keyboard>.keymap`. Inside the keymap file ensure the keymap node has `compatible = zmk,keymap` and it's not misspelled. For more information see the [Keymap](features/keymaps.mdx) and [Config](config/index.md) documentation.

### Devicetree Errors

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
