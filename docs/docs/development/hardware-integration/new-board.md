---
title: New Board
---

This guide will walk through the necessary steps to write a [board](./index.mdx#boards--shields) suitable for use with ZMK. Boards used with ZMK fall into two categories:

- Boards with interconnects, such as `nice_nano` or `seeed_xiao`. Such boards will (active development of ZMK aside) always be used with a shield to create a keyboard.
- Boards that are themselves keyboards, such as `bt75` or `ferris`.

Some keyboards are boards but also have interconnects for modular add-ons. These are considered as keyboard-boards for the purpose of this guide. Details on adding an interconnect for modular add-ons are considered out of scope.

## Boards Included in ZMK

Boards with interconnects can be considered for inclusion to the tree of ZMK. If this is your aim, it is vital that you read through our [clean room policy](../contributing/clean-room.md).

Boards with interconnects that are included with ZMK are generally:

- Reliably commercially available as individual units
- In regular use across multiple shields (i.e. not niche)

Popular open source designs which are not being sold can also be considered for inclusion.

There also exist many boards in the tree of upstream Zephyr. We generally accept ZMK-variants of these boards into our tree as well.

## New ZMK Module Repository

Regardless of whether you aim to include the board in ZMK or not, you should first write the definition for it in a module. A new ZMK module repository can be created from a template.

:::note
This guide assumes you already have a configured GitHub account. If you don't yet have one, go ahead and [sign up](https://github.com/join) before continuing.
:::

Follow these steps to create your new repository:

- Visit https://github.com/zmkfirmware/unified-zmk-config-template
- Click the green "Use this template" button
- In the drop down that opens, click "Use this template".
- In the following screen, provide the following information:
  - A repository name, e.g. `my-board-module`.
  - A brief description, e.g. `ZMK Support For MyBoard`.
  - Select Public or Private, depending on your preference.
- Click the green "Create repository" button

The repository is a combination of the directories and files required of a ZMK config, and those required of a shield module.
This enables the use of GitHub Actions to test that the shield is defined correctly.
See also the page on [module creation](../module-creation.md) for a reference on exactly which file structure and files are required for a ZMK keyboard module.

We recommend that you take this moment to name your module according to our [convention](../module-creation.md), i.e. your `zephyr/module.yml` file should begin with

```yaml title="zephyr/module.yml"
name: zmk-keyboard-<keyboard_name>
```

if it is a keyboard, or

```yaml title="zephyr/module.yml"
name: zmk-component-<board_name>
```

if it is a board with an interconnect.

## Write a Zephyr Board Definition

Zephyr has a guide on writing a board [here](https://docs.zephyrproject.org/4.1.0/hardware/porting/board_porting.html). Follow this guide to create a Zephyr-compatible board. Use the `boards` folder of your ZMK module as a base.

Once your board definition has been written, we recommend flashing some [Zephyr samples](https://docs.zephyrproject.org/4.1.0/samples/basic/basic.html) to it, to verify that it is working.

Flashing Zephyr samples can also be very helpful when troubleshooting/testing the functionality of extra features such as LED ICs, Bluetooth, or various sensor ICs.

## Write a ZMK Variant of Zephyr Board

To make a board fully compatible with ZMK, you will be creating a ZMK variant of the Zephyr board you made in the previous step. Perform the below actions for each board you've defined, in the case of multi-board split keyboards.

### Add Variant to board.yml

Edit your existing `board.yml` file to add the variant:

```yaml title="board.yml"
board:
  ...
  socs:
  - name: <soc-1>
    variants:
    - name: zmk
```

### Add Kconfig for ZMK Variant

Add a file to your board's folder called `<your-board>_zmk_defconfig`. This file will be used to set Kconfig flags specific to the ZMK variant.

:::warning
Make sure you know what each Kconfig flag does before you enable it. Some flags may be incompatible with certain hardware, or have other adverse effects.
:::

These flags are typically a subset of the following:

```yaml title="<your-board>_zmk_defconfig"
# SPDX-License-Identifier: <your license>

# Enable MPU
CONFIG_ARM_MPU=y

CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=125000000

# Enable reset by default
CONFIG_RESET=y

# Enable clock control by default
CONFIG_CLOCK_CONTROL=y

# Code partition needed to target the correct flash range
CONFIG_USE_DT_CODE_PARTITION=y

# Output UF2 by default, native bootloader supports it.
CONFIG_BUILD_OUTPUT_UF2=y

# USB HID
CONFIG_ZMK_USB=y

# BLE HID
CONFIG_ZMK_BLE=y

# Bootloader Support
CONFIG_RETAINED_MEM=y
CONFIG_RETENTION=y
CONFIG_RETENTION_BOOT_MODE=y

# Settings Support
CONFIG_MPU_ALLOW_FLASH_WRITE=y
CONFIG_NVS=y
CONFIG_SETTINGS_NVS=y
CONFIG_FLASH=y
CONFIG_FLASH_PAGE_LAYOUT=y
CONFIG_FLASH_MAP=y


# Enable HW stack protection
CONFIG_HW_STACK_PROTECTION=y

# Enable GPIO
CONFIG_GPIO=y

# Defaults for matrix scanning to avoid interrupt issues
CONFIG_ZMK_KSCAN_MATRIX_POLLING=y
```

Note that none of our in-tree boards have all of the above flags set. We recommend referencing the Kconfig flags from an existing in-tree board with the same SoC as the one you are using for your initial definition, making sure you understand what each flag does.

### Add Devicetree for ZMK Variant

```dts title="<your-board>_zmk.dts
/*
 * Copyright (c) <year> <you>
 *
 * SPDX-License-Identifier: <your license>
 */

// Include the base board definition
#include <../boards/<vendor or you>/<board folder name>/<your board>.dts>
// Include predefined boot mode settings, e.g.
#include <arm/raspberrypi/rp2040-boot-mode-retention.dtsi>

// Disable UART nodes if they are not actively in use (wired split) as they increase power draw
&uart0 { status = "disabled"; };

// Reduce the code partition section of the memory and add a storage partition to store ZMK Studio changes
&code_partition {
    reg = <0x100 (DT_SIZE_M(2) - 0x100 - DT_SIZE_K(512))>;
};
&flash0 {
    reg = <0x10000000 DT_SIZE_M(2)>;
    partitions {
        storage_partition: partition@180000 {
            reg = <0x180000 DT_SIZE_K(512)>;
            read-only;
        };
    };
};
```

See [here](./bootloader/index.mdx) for bootloader instructions for other SoCs. Depending on your SoC and design you may need to make further changes. Please refer to our in-tree boards with the same SoC as yours as examples.

## Next Steps

If your board is a keyboard, continue from [the `Kconfig.defconfig` step](https://zmk.dev/docs/development/hardware-integration/new-shield?keyboard-type=unibody#kconfigdefconfig) of the new shield guide. Use your ZMK variant's devicetree instead of the overlay file which would be used for a shield.

If your board is a board with an interconnect, your next step should be to write a [tester shield](../../troubleshooting/hardware-issues.mdx#identifying-issues). Such a shield should be the bare minimum shield to verify that your board works with ZMK.
