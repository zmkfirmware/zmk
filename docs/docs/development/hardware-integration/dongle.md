---
title: Keyboard Dongle
sidebar_label: Keyboard Dongle
---

A bluetooth dongle can be added to any keyboard running ZMK. The result is a [split keyboard](../../features/split-keyboards.md) with the dongle as "central". There are a number of advantages to adding a dongle, but also some disadvantages:

Benefits:

- The "central" dongle results in [improved battery life](/power-profiler) for the keyboard/former "central", as it is now a peripheral.
- It is easier to connect to the host device.

Disadvantages:

- An extra [board](../../faq.md#what-is-a-board) is needed (any BLE-capable board will work).
- The keyboard becomes unusable without the dongle.

Depending on how the dongle is used, there are some additional [latency considerations](../../features/split-keyboards.md#latency-considerations) to keep in mind.
The addition of the dongle adds an extra "hop" for the former central, increasing its latency to that of a peripheral. The other parts are unchanged latency wise. There is also a commonly occurring case where the peripherals benefit.
Assuming the dongle is connected to USB and the former central would have been connected via bluetooth to the host if the dongle wasn't present:

- The former central will have its latency increased by about 1ms from the extra USB "hop"
- The other parts will have their average latency _decreased_ by 6.5ms from the replacement of a BLE "hop" with a "USB" hop.

In other words, for this commonly occurring case the average latency of the keyboard actually decreases.

## Adding a Dongle

To add a dongle, you will create a simplified form of a [new shield](./new-shield.mdx).
If you wish for your dongle to have any keys, then the below approach will not work.

As there are a very large number of possible devices that could be used as a dongle, you will be defining your dongle as a personal shield intended for your exclusive use.

Prior to adding a dongle to your keyboard, please test its functionality without a dongle. The below guide will assume that your keyboard is named `my_keyboard`, replace accordingly.

### Dongle Folder

First, make sure that your `zmk-config` matches the folder structure found in the [unified ZMK config template](https://github.com/zmkfirmware/unified-zmk-config-template) (extra files and folders are fine, but none should be missing).

Next, navigate to the `zmk-config/boards/shields` directory. Create a subdirectory called `my_keyboard`, if one doesn't already exist. Unless otherwise specified, the below files should all be placed in said folder.

### Kconfig Files

#### Kconfig.shield

Make a file called `Kconfig.shield`, if one does not exist already. Add the following lines to it:

```kconfig title="Kconfig.shield"
# No whitespace after the comma or in the keyboard name!
config SHIELD_MY_KEYBOARD_DONGLE
    def_bool $(shields_list_contains,my_keyboard_dongle)
```

Note that there are _two_ names you need to adjust in the above: `SHIELD_MY_KEYBOARD_DONGLE` and `my_keyboard_dongle`.

#### Kconfig.defconfig

Make a file called `Kconfig.defconfig`, if one does not exist already. Add the following lines to it:

```kconfig title="Kconfig.defconfig"
if SHIELD_MY_KEYBOARD_DONGLE

# Max 16 characters in keyboard name
config ZMK_KEYBOARD_NAME
    default "My Board"

config ZMK_SPLIT_ROLE_CENTRAL
    default y

config ZMK_SPLIT
    default y

endif
```

Note that `SHIELD_MY_KEYBOARD_DONGLE` should be adjusted to match the previous section.

### Dongle Overlay File

You will now need to find and copy your keyboard's matrix transform and physical layout. Navigate to the directory defining your keyboard (in-tree keyboards found [here](https://github.com/zmkfirmware/zmk/tree/main/app/boards)) and look through the [devicetree files](../../config/index.md) for nodes with `compatible = "zmk,matrix-transform";` or `compatible = "zmk,physical-layout";`. These should look something like this:

```dts
// Matrix Transform
    default_transform: keymap_transform_0 {
        compatible = "zmk,matrix-transform";
        columns = <14>;
        rows = <5>;

        map = <
            // Lots of RC(r,c) macros
        >;
    };

// Physical Layout
    default_layout: default_layout {
        compatible = "zmk,physical-layout";
        display-name = "Default Layout";
        transform = <&default_transform>;
        kscan = <&kscan0>;
        keys = <
            // Really long list of keys. Keys property may not be present.
        >;
    };
```

You may find some keyboards import their physical layouts from [this location](https://github.com/zmkfirmware/zmk/tree/main/app/dts/layouts).

Now that you've found these nodes, make a file called `my_keyboard_dongle.overlay` (renamed as appropriate) and add the following lines to it:

```dts title="my_keyboard_dongle.overlay"
/ {
    chosen {
        zmk,kscan = &mock_kscan;
        zmk,physical-layout = &default_layout;
    };

    mock_kscan: mock_kscan_0 {
        compatible = "zmk,kscan-mock";
        columns = <0>;
        rows = <0>;
        events = <0>;
    };

    // Copy of matrix transform node here
    // Copy of physical layout node here
      // If kscan property of physical layout is present, assign &mock_kscan to it
};
```

Make sure that the matrix transform is selected by the physical layout. If the physical layout imported, you may need to override it:

```dts title="my_keyboard_dongle.overlay"
&default_layout {
    transform = <&default_transform>;
};
```

If no physical layout is found, you should skip it and instead select the matrix transform as chosen directly:

```dts title="my_keyboard_dongle.overlay"
/ {
    chosen {
        zmk,kscan = &mock_kscan;
        zmk,matrix-transform = &default_transform;
    };
};
```

:::note
If multiple physical layouts and matrix transformations are present, you should only need to import the one you wish to use. However, if you are getting errors, then the shield might be defining things in an unusual manner. In this case, copy in all matrix transformations and physical layouts, overriding as appropriate.
:::

## Enabling the Dongle

Next, navigate to the `config` folder found in your `zmk-config`'s root.

If your keyboard is a unibody, add the following to your `my_keyboard.conf` file (create the file if it does not exist):

```kconfig title="config/my_keyboard.conf"
# Enable split functionality, this will put the unibody keyboard in peripheral mode
CONFIG_ZMK_SPLIT=y
```

If your keyboard is a split keyboard, add the following to your `my_keyboard_left.conf` file, assuming `_left` is the current central (create the file if it does not exist):

```kconfig title="config/my_keyboard_left.conf"
# Disable central role to use dongle
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=n
```

If your keyboard is a split keyboard, you will also need to increase the number of peripherals the dongle can connect to.
Make a file called `my_keyboard_dongle.conf` and add the following lines to it:

```kconfig title="config/my_keyboard_dongle.conf"
CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=n # Change n to the number of peripherals your dongle will have
CONFIG_BT_MAX_CONN=m # Change m to be equal or greater than the number of peripherals + the number of bt hosts the dongle should connect to
```

You can always remove the above lines if you wish to go back to not using a dongle.

:::note
Under certain conditions, you may also need to change `CONFIG_BT_MAX_CONN` when using a unibody: See the [configuration documentation](../../config/system.md#bluetooth).
:::

## Building the Firmware

Add the appropriate lines to your `build.yml` file to build the firmware for your dongle, in addition to the other parts of your keyboard.

```yaml
include:
  # -----------------------------------------
  # Your other keyboard parts here
  # -----------------------------------------
  # Change the board appropriately, you can use any board
  - board: nice_nano_v2
    shield: my_keyboard_dongle
  - board: nice_nano_v2
    shield: settings_reset
```

:::warning
Before flashing your new firmware, flash `settings_reset` [firmware](../../troubleshooting/connection-issues.mdx#acquiring-a-reset-uf2) on all devices.
:::
