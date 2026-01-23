---
title: Boot Magic Key
sidebar_label: Boot Magic Key
---

A boot magic key performs one or more actions if a specific key is held while powering on the keyboard. This is useful for recovering a keyboard which doesn't have a physical reset button. It also works on the peripheral side of a split keyboard, even when it isn't connected to the central side.

## Magic Keys

To define a boot magic key on a new board or shield, add a `zmk,boot-magic-key` node to your board's `.dts` file or shield's `.overlay` file and select which key will trigger it with the `key-position` property.

You can also enable the feature for any keyboard by adding it to your `.keymap` file.

```c
/ {
    ...
    bootloader_key: bootloader_key {
        compatible = "zmk,boot-magic-key";
        key-position = <0>;
    };
    ...
};
```

:::info

Key positions are numbered like the keys in your keymap, starting at 0. So, if the first key in your keymap is `Q`, this key is in position `0`. The next key (possibly `W`) will have position 1, etcetera.

:::

If `key-position` is omitted, it will trigger for the key in position `0`.

Next, you should add properties to determine what the magic key will do:

### Jump to Bootloader

If a boot magic key has a `jump-to-bootloader` property, it will reboot to the bootloader:

```c
/ {
    ...
    bootloader_key: bootloader_key {
        compatible = "zmk,boot-magic-key";
        ...
        jump-to-bootloader;
    };
    ...
};
```

### Reset Settings

If a boot magic key has a `reset-settings` property, it will reset persistent settings and then reboot:

```c
/ {
    ...
    reset_settings_key: reset_settings_key {
        compatible = "zmk,boot-magic-key";
        ...
        reset-settings;
    };
    ...
};
```

:::info

This clears all BLE bonds. You will need to re-pair the keyboard with any hosts after using this.

:::

:::caution

Currently this action _only_ clears BLE bonds. It will be updated to reset all settings in the future.

:::

## Multiple Actions

If you want a single boot magic key to perform multiple actions, simply add properties for each action to the same `zmk,boot-magic-key` node. The order of the properties does not matter.

For example, to make a key that resets settings and then reboots to the bootloader, add both `reset-settings` and `jump-to-bootloader`:

```c
/ {
    ...
    recovery_key: recovery_key {
        compatible = "zmk,boot-magic-key";
        jump-to-bootloader;
        reset-settings;
    };
    ...
};
```

:::info

You may define multiple `zmk,boot-magic-key` nodes for different keys, but note that if you hold multiple keys at boot, they will be run in an arbitrary order. If one of them reboots the keyboard, the rest of the keys will not run.

:::

## Split Keyboards

For split keyboards, you can define multiple boot magic keys and then only enable the correct key(s) for each side. For example, if key 0 is the top-left key on the left side and key 11 is the top-right key on the right side, you could use:

**shield.dtsi**

```c
/ {
    ...
    bootloader_key_left: bootloader_key_left {
        compatible = "zmk,boot-magic-key";
        key-position = <0>;
        jump-to-bootloader;
        status = "disabled";
    };

    bootloader_key_right: bootloader_key_right {
        compatible = "zmk,boot-magic-key";
        key-position = <11>;
        jump-to-bootloader;
        status = "disabled";
    };
    ...
};
```

**shield_left.overlay**

```c
#include "shield.dtsi"

&bootloader_key_left {
    status = "okay";
};
```

**shield_right.overlay**

```c
#include "shield.dtsi"

&bootloader_key_right {
    status = "okay";
};
```

## Key Positions and Alternate Layouts

Key positions are affected by the [matrix transform](../config/kscan.md#matrix-transform), so if your keyboard has multiple transforms for alternate layouts, you may need to adjust positions according to the user's selected transform. There is no automatic way to do this, but one way to simplify things for users is to add a block of commented out code to the keymap which selects the transform and updates the key positions to match if uncommented.

For example, consider a split keyboard which has 6 columns per side by default but supports a 5-column layout, and assume you want the top-left key on the left side and the top-right key on the right side to be boot magic keys. The top-left key will be position 0 regardless of layout, but the top-right key will be position 11 by default and position 9 in the 5-column layout.

**shield.dtsi**

```c
/ {
    chosen {
        zmk,matrix_transform = &default_transform;
    };

    bootloader_key_left: bootloader_key_left {
        compatible = "zmk,boot-magic-key";
        key-position = <0>;
        jump-to-bootloader;
        status = "disabled";
    };

    bootloader_key_right: bootloader_key_right {
        compatible = "zmk,boot-magic-key";
        key-position = <11>;
        jump-to-bootloader;
        status = "disabled";
    };
    ...
};
```

**shield.keymap**

```c
// Uncomment this block if using the 5-column layout
// / {
//     chosen {
//         zmk,matrix_transform = &five_column_transform;
//     };
//     bootloader_key_right {
//         key-position = <9>;
//     };
// };
```

## Startup Timeout

By default, the keyboard processes boot magic keys for 500 ms. You can change this timeout with `CONFIG_ZMK_BOOT_MAGIC_KEY_TIMEOUT_MS` if it isn't reliably triggering, for example if you have some board-specific initialization code which takes a while.

To change the value for a new board or shield, set this option in your `Kconfig.defconfig` file:

```
config ZMK_BOOT_MAGIC_KEY_TIMEOUT_MS
    default 1000
```

You can also set it from your keyboard's `.conf` file in a user config repo:

```ini
CONFIG_ZMK_BOOT_MAGIC_KEY_TIMEOUT_MS=1000
```
