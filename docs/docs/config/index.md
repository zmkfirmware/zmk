---
title: Configuration Overview
sidebar_label: Overview
---

ZMK has many configuration settings that can be changed to change the behavior of your keyboard. They are set in either Kconfig or Devicetree files.

This page describes the Kconfig and Devicetree file formats and how to change settings in them. See the other pages in the configuration section for a list of settings you can change.

:::note
All configuration is currently set at compile time. After changing any settings, you must build new firmware and flash it for the changes to apply.
:::

## Config File Locations

ZMK will search multiple folders for the config files described below. There are three primary locations it will search:

### User Config Folder

When building with a `zmk-config` folder, ZMK will search the `zmk-config/config` folder for the following config files, where `<name>` is the name of the shield if using a shield, or the name of the board otherwise:

- `<name>.conf` (Kconfig)
- `<name>.keymap` (Devicetree)

These files hold your personal settings for the keyboard. All files are optional. If present, they override any configuration set in the board or shield folders. Otherwise, the default configuration and/or keymap is used.

When using a split keyboard, you can use a single file without the `_left` or `_right` suffix to configure both sides. For example, `corne.conf` and `corne.keymap` will apply to both `corne_left` and `corne_right`. If a shared config file exists, any left or right files will be ignored.

### Board Folder

ZMK will search for config files in either of:

- [`zmk/app/boards/arm/<board>`](https://github.com/zmkfirmware/zmk/tree/main/app/boards/arm)
- `zmk-config/config/boards/arm/<board>`

...where `<board>` is the name of the board. These files describe the hardware of the board.

ZMK will search the board folder for the following config files:

- `<board>_defconfig` (Kconfig)
- `<board>.conf` (Kconfig)
- `<board>.dts` (Devicetree)
- `<board>.keymap` (Devicetree, keyboards with onboard controllers only)

Shared config files (excluding any `_left` or `_right` suffix) are not currently supported in board folders.

For more documentation on creating and configuring a new board, see [Zephyr's board porting guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html#write-kconfig-files).

### Shield Folder

When building with a shield, ZMK will search for config files in either of:

- [`zmk/app/boards/shields/<shield>`](https://github.com/zmkfirmware/zmk/tree/main/app/boards/shields)
- `zmk-config/config/boards/shields/<shield>`

...where `<shield>` is the name of the shield. These files describe the hardware of the shield that the board is plugged into.

ZMK will search the shield folder for the following config files:

- `<shield>.conf` (Kconfig)
- `<shield>.overlay` (Devicetree)
- `<shield>.keymap` (Devicetree)

Shared config files (excluding any `_left` or `_right` suffix) are not currently supported in shield folders.

For more documentation on creating and configuring a new shield, see [Zephyr's shield documentation](https://docs.zephyrproject.org/latest/hardware/porting/shields.html) and [ZMK's new keyboard shield](../development/new-shield.md) guide.

## Kconfig Files

Kconfig is used to configure global settings such as the keyboard name and enabling certain hardware devices. These typically have a `.conf` file extension and are text files containing `CONFIG_XYZ=value` assignments, with one setting per line.

Kconfig files look like this:

```ini
CONFIG_ZMK_SLEEP=y
CONFIG_EC11=y
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
```

The list of available settings is determined by various files in ZMK whose names start with `Kconfig`. Files ending with `_defconfig` use the same syntax, but are intended for setting configuration specific to the hardware which users typically won't need to change. Note that options are _not_ prefixed with `CONFIG_` in these files.

See [Zephyr's Kconfig documentation](https://docs.zephyrproject.org/latest/build/kconfig/index.html) for more details on Kconfig files.

### KConfig Value Types

#### bool

Either `y` for yes or `n` for no.

Example: `CONFIG_FOO=y`

#### int

An integer.

Example: `CONFIG_FOO=42`

#### string

Text surrounded by double quotes.

Example: `CONFIG_FOO="foo"`

## Devicetree Files

Various Devicetree files are combined to build a tree that describes the hardware for a keyboard. They are also used to define keymaps.

Devicetree files use various file extensions. These indicate the purpose of the file, but they have no effect on how the file is processed. Common file extensions for Devicetree files include:

- `.dts`: The base hardware definition.
- `.overlay`: Adds to and/or overrides definitions in a `.dts` file.
- `.keymap`: Holds a keymap and user-specific hardware configuration.
- `.dtsi`: A file which is only intended to be `#include`d from another file.

Devicetree files look like this:

```dts
/ {
    chosen {
        zmk,kscan = &kscan0;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        label = "KSCAN";
    };
};
```

Devicetree properties apply to specific nodes in the tree instead of globally. The properties that can be set for each node are determined by `.yaml` files in ZMK in the various `dts/bindings` folders.

See [Zephyr's Devicetree guide](https://docs.zephyrproject.org/latest/build/dts/index.html) for more details on Devicetree files.

### Changing Devicetree Properties

Since Devicetree properties are set for specific nodes in the tree, you will first need to find the node you want to configure. You will typically need to
search through the `.dts` file for your board, `.overlay` file for your shield, or a `.dtsi` file included by those files using an `#include` statement.

A Devicetree node looks like this:

```dts
kscan0: kscan {
    compatible = "zmk,kscan-gpio-matrix";
    // more properties and/or nodes...
};
```

The part before the colon, `kscan0`, is a label. This is optional, and it provides a shortcut to allow changing the properties of the node. The part after the colon, `kscan`, is the node's name. The values inside the curly braces are the node's properties.

The `compatible` property indicates what type of node it is. Search this documentation for the text inside the quotes to see which properties the node
supports. You can also search ZMK for a file whose name is the value of the `compatible` property with a `.yaml` file extension.

To set a property, see below for examples for common property types, or see [Zephyr's Devicetree documentation](https://docs.zephyrproject.org/latest/build/dts/intro.html#writing-property-values) for more details on the syntax for properties.

To change a property for an existing node, first find the node you want to change and find its label. Next, outside of any other node, write an ampersand (`&`)
followed by the node's label, an opening curly brace (`{`), one or more new property values, a closing curly brace (`}`), and a semicolon (`;`).

For example, to adjust the debouncing of the `zmk,kscan-gpio-matrix` node shown above, you could add this to your keymap:

```dts
&kscan0 {
    debounce-press-ms = <0>;
};
```

If the node you want to edit doesn't have a label, you can also write a new tree and it will be merged with the existing tree, overriding any properties. Adding this to your keymap would be equivalent to the previous example.

```dts
/ {
    kscan {
        debounce-press-ms = <0>;
    };
};
```

### Devicetree Property Types

These are some of the property types you will see most often when working with ZMK. [Zephyr's Devicetree bindings documentation](https://docs.zephyrproject.org/latest/build/dts/bindings.html) provides more detailed information and a full list of types.

#### bool

True or false. To set the property to true, list it with no value. To set it to false, do not list it.

Example: `property;`

If a property has already been set to true and you need to override it to false, use the following command to delete the existing property:

```dts
/delete-property/ the-property-name;
```

#### int

A single integer surrounded by angle brackets. Also supports mathematical expressions.

Example: `property = <42>;`

#### string

Text surrounded by double quotes.

Example: `property = "foo";`

#### array

A list of integers surrounded by angle brackets and separated with spaces. Mathematical expressions can be used but must be surrounded by parenthesis.

Example: `property = <1 2 3 4>;`

Values can also be split into multiple blocks, e.g. `property = <1 2>, <3 4>;`

#### phandle

A single node reference surrounded by angle brackets.

Example: `property = <&label>`

#### phandles

A list of node references surrounded by angle brackets.

Example: `property = <&label1 &label2 &label3>`

#### phandle array

A list of node references and possibly numbers to associate with the node. Mathematical expressions can be used but must be surrounded by parenthesis.

Example: `property = <&none &mo 1>;`

Values can also be split into multiple blocks, e.g. `property = <&none>, <&mo 1>;`

See the documentation for "phandle-array" in [Zephyr's Devicetree bindings documentation](https://docs.zephyrproject.org/latest/build/dts/bindings.html)
for more details on how parameters are associated with nodes.

#### GPIO array

This is just a phandle array. The documentation lists this as a different type to make it clear which properties expect an array of GPIOs.

Each item in the array should be a label for a GPIO node (the names of which differ between hardware platforms) followed by an index and configuration flags. See [Zephyr's GPIO documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html) for a full list of flags.

Example:

```dts
some-gpios =
    <&gpio0 0 GPIO_ACTIVE_HIGH>,
    <&gpio0 1 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
    ;
```

#### path

A path to a node, either as a node reference or as a string.

Examples:

```dts
property = &label;
property = "/path/to/some/node";
```
