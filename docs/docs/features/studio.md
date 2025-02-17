---
title: ZMK Studio
---

ZMK Studio provides runtime update functionality to ZMK powered devices, allowing users to change their keymap layers without flashing new firmware to their keyboards.

:::info

To use ZMK Studio, a keyboard needs to be [configured appropriately](#adding-zmk-studio-support-to-a-keyboard). ZMK has updated some, but not all, of its in-tree keyboards for use with ZMK Studio, the list of which can be found [here](/blog/2024/11/11/zmk-studio-mvp-ga). If your keyboard is supported by an external module/config, check with the maintainer to see if support has been added.

:::

## Capabilities

ZMK Studio currently has the following capabilities:

| Legend: | ✅ Supported | 🚧 Under Development | 💡 Planned | 🕯️ Low Priority | ❌ Not Planned |
| :------ | :----------- | :------------------- | :--------- | :-------------- | -------------- |

| Status | Feature/Capability                                                                                                                        |
| :----: | ----------------------------------------------------------------------------------------------------------------------------------------- |
|   ✅   | Making keymap changes while keyboard is in use                                                                                            |
|   ✅   | Making changes while connected via USB                                                                                                    |
|   ✅   | Native app for: Windows, Linux, MacOS                                                                                                     |
|   ✅   | Making changes while connected via BLE (Linux web-app & native apps only)                                                                 |
|   ✅   | Assigning [predefined behaviors](../keymaps/behaviors/index.mdx) to keys                                                                  |
|   ✅   | Assigning [user-defined behaviors](../keymaps/behaviors/index.mdx#user-defined-behaviors) to keys                                         |
|   💡   | [Configuring basic behavior properties](../config/behaviors.md)                                                                           |
|   💡   | Configuring advanced behavior properties, e.g. [tap dance](../keymaps/behaviors/tap-dance.mdx), [macros](../keymaps/behaviors/macros.md), |
|   ❌   | Defining new behaviors not specified in devicetree                                                                                        |
|   💡   | Configuring [combos](../keymaps/combos.md)                                                                                                |
|   💡   | Configuring [conditional layers](../keymaps/conditional-layers.md)                                                                        |
|   🕯️   | Assigning behaviors to encoders                                                                                                           |
|   ✅   | Selecting alternative pre-defined physical layouts for the keyboard                                                                       |
|   ❌   | Defining new physical layouts for the keyboard                                                                                            |
|   ✅   | Renaming layers & enabling [extra layers](#including-extra-layers)                                                                        |
|   ❌   | Adding more layers than specified by devicetree                                                                                           |
|   💡   | Host locale selection                                                                                                                     |
|   💡   | Importing and exporting keymaps                                                                                                           |

Items listed as "planned", "under development", "low priority", or "not planned" can be configured using [devicetree](../config/index.md#devicetree-files) instead.

## Keymap Changes

To unlock your keyboard to allow ZMK Studio to make changes, you'll need to add a [`&studio_unlock`](../keymaps/behaviors/studio-unlock.md) binding to the keymap.

:::note

Once using ZMK Studio to manage your keymap, any future changes made to the `.keymap` file for your keyboard will not be applied unless you perform a "Restore Stock Settings" action from the ZMK Studio UI.

Generally, if you intend to use ZMK Studio, then you should not make any further changes to the `.keymap` file (with the exception of adding new empty layers to then use within ZMK Studio).

:::

## Accessing ZMK Studio

You can use ZMK Studio with Chrome/Edge at https://zmk.studio/.

To use the native app for Linux, macOS, or Windows, visit the [download page](https://zmk.studio/download).

:::warning

To use ZMK Studio over USB, you need permission to access the USB serial port. This most commonly occurs on Linux, with various distributions having different methods of resolving the error. For example, you may need to be added to a `uucp` or a `dialout` group. Refer to your operating system's documentation for more information.

:::

## Building

Building for ZMK Studio involves two main additional items.

- Build with the `studio-rpc-usb-uart` snippet to enable the endpoint used for ZMK Studio communication over USB.
- Enable the `ZMK_STUDIO` Kconfig setting.

### GitHub Actions

Add a `studio-rpc-usb-uart` to the `snippet` property of your build configuration, in the `build.yaml` file at the root of your user config. Enabling the `ZMK_STUDIO` Kconfig setting can also be done from the `build.yaml` file using the `cmake-args` property.

For a split keyboard, you should do this _only_ for your central/left side, e.g.:

```yaml title="build.yaml"
---
include:
  - board: nice_nano_v2
    shield: corne_left
    snippet: studio-rpc-usb-uart
    cmake-args: -DCONFIG_ZMK_STUDIO=y
  - board: nice_nano_v2
    shield: corne_right
```

### Local Build

When building locally, use the `-S` parameter to include the `studio-rpc-usb-uart` snippet. Instead of adding it to your config file, you can also append the `ZMK_STUDIO` Kconfig as an additional CMake argument, e.g.:

```bash
west build -d build/cl_studio -b nice_nano_v2 \
  -S studio-rpc-usb-uart -- -DSHIELD=corne_left -DCONFIG_ZMK_STUDIO=y
```

## Including Unreferenced Behaviors

Normally, ZMK will only build and include behaviors that are referenced by your keymap and unused behavior will be skipped. However, ZMK Studio builds using the `studio-rpc-usb-uart` snippet will automatically define the `ZMK_BEHAVIORS_KEEP_ALL` value, which changes the approach and builds all possible behaviors into the firmware. This lets those behaviors be used in ZMK Studio.

A few controls are available to override this behavior for fine-grained control of what behaviors are built. Behaviors can be kept or omitted by defining certain values in the top of your keymap file, _before_ the standard `behaviors.dtsi` file is included.

By convention, the defines used to keep/omit a given behavior are typically upper-cased versions of the behavior label, e.g. for the `&kt` behavior, the suffix to use would be `_KT`.

### Omit Specific Behaviors

You can omit a specific behaviors by defining a variable like `ZMK_BEHAVIORS_OMIT_KT` at the top of your keymap:

```dts
#define ZMK_BEHAVIORS_OMIT_KT

#include <behaviors.dtsi>
```

### Keep Only Selective Behaviors

To override the default "keep all" functionality, you can undefine the `ZMK_BEHAVIORS_KEEP_ALL` flag, and then keep only specific behaviors with a flag like `ZMK_BEHAVIORS_KEEP_KT`, e.g.:

```dts
#undef ZMK_BEHAVIORS_KEEP_ALL

#define ZMK_BEHAVIORS_KEEP_SK
#define ZMK_BEHAVIORS_KEEP_MT
#define ZMK_BEHAVIORS_KEEP_KT

#include <behaviors.dtsi>
```

## Including Extra Layers

By default, a build with ZMK Studio enabled will only allow as many layers as are defined in your standard keymap. To make additional layers available for use through ZMK Studio, you simply add new empty layers to your keymap with a status of `reserved`, e.g.:

```dts
/ {
    keymap {
        compatible = "zmk,keymap";

        base {
            display-name = "Base";
            bindings = // etc.
        };

        fn_layer {
            display-name = "Fn";
            bindings = // etc.
        };

        extra1 {
            status = "reserved";
        };

        extra2 {
            status = "reserved";
        };
    }
}
```

The reserved layers will be ignored during regular ZMK builds but will become available for ZMK Studio enabled builds.

## Adding ZMK Studio Support to a Keyboard

To allow ZMK Studio to be used with a keyboard, the keyboard will need to have a physical layout with the `keys` property defined. The keyboard should also **not** have a `chosen` `zmk,matrix-transform`. Relevant information can be found in:

- The [dedicated page on physical layouts](../development/hardware-integration/physical-layouts.md), informing you how to define one
- The [new shield guide](../development/hardware-integration/new-shield.mdx), informing you how to select a physical layout once defined
- The corresponding [configuration page](../config/layout.md#physical-layout), for reference

To use the `studio-rpc-usb-uart` snippet, the keyboard also needs to be configured to allow CDC-ACM console snippets (this is also used for [USB logging](../development/usb-logging.mdx)). If your keyboard is a composite keyboard, consisting of an in-tree board and a shield, then you can skip this step as the board will already be configured properly. Relevant information on that can be found [in the Zephyr documentation](https://docs.zephyrproject.org/3.5.0/snippets/cdc-acm-console/README.html).

Firmware with ZMK Studio enabled require significantly more RAM. Some MCUs, such as the STM32F072 series, will require fine tuning of various settings in order to reduce the RAM consumption enough for a Studio enabled build to fit.

Finally, once you have successfully built and tested firmware with ZMK Studio enabled, add the `studio` flag to your keyboard's [metadata](../development/hardware-integration/hardware-metadata-files#features).
