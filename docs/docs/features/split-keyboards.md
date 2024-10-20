---
title: Split Keyboards
sidebar_label: Split Keyboards
---

ZMK supports setups where a keyboard is split into two or more physical parts (also called "sides" or "halves" when split in two), each with their own controller running ZMK. The parts communicate with each other to work as a single keyboard device.

:::note[Split communication protocols]
Currently ZMK only supports split keyboards that communicate with each other wirelessly over BLE.
As such, only controllers that support BLE can be used with ZMK split keyboards.

Supporting split communication over wired protocols is planned, allowing for ZMK split keyboards using non-wireless controllers.
:::

## Central and Peripheral Roles

In split keyboards running ZMK, one part is assigned the "central" role which receives key position and sensor events from the other parts that are called "peripherals."
The central runs the necessary keymap logic to convert received events into HID events such as keycodes and then communicates with the connected host devices, e.g. over USB or bluetooth.

The internal keyboard state (like active layers) is handled exclusively by the central.
Peripherals _cannot_ communicate with host devices on their own, since they can only communicate with the central.
They will not present as keyboard devices when connected over USB and will not advertise as pairable BLE keyboards.

By convention, for a keyboard split into two "halves" the left half is set as the central and the right as a peripheral.

:::info[Battery life impact]
For BLE-based split keyboards, the central uses significantly more power than the peripherals because its radio needs to periodically wake up to check for incoming transmissions.
You can refer to the [power profiler](/power-profiler) to see battery life estimates for different roles.
:::

### Configuration

The [new shield guide](../development/hardware-integration/new-shield.mdx) details how to define a split keyboard shield with two parts, enabling the split feature and setting up the necessary roles for each part.

Also see the reference section on [split keyboards configuration](../config/system.md#split-keyboards) where the relevant symbols include `CONFIG_ZMK_SPLIT` that enables the feature, `CONFIG_ZMK_SPLIT_ROLE_CENTRAL` which sets the central role and `CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS` that sets the number of peripherals.

### Latency Considerations

Since peripherals communicate through centrals, the key and sensor events originating from them will naturally have a larger latency, especially with a wireless split communication protocol.
For the currently used BLE-based transport, split communication increases the average latency by 3.75ms with a worst case increase of 7.5ms.

## Building and Flashing Firmware

ZMK split keyboards require building and flashing different firmware files for each split part.
For instance when [using the GitHub workflow](../user-setup.mdx) to build two part split keyboards, two firmware files that typically contain `<keyboard>_left` and `<keyboard>_right` in the file names will be produced.
These files need to be flashed to the respective controllers of the two halves.

:::tip[Updating your keymap]
Since the keymap processing is primarily done on the central side, for keymap changes it will typically be enough to flash the controller of the central half.
However if you make changes to [config files](../config/index.md) that should apply to all parts, you need to flash to all parts.
Any changes in ZMK related to split keyboard features might also necessitate doing this.
:::

## Pairing for Wireless Split Keyboards

Split keyboards with BLE-based split communications (i.e. all officially supported split keyboards) have an internal pairing procedure between the central and each peripheral.
When the central has an open slot for a peripheral, it will advertise for connections (which will not be visible to non-ZMK devices).
Then, any peripheral that has not yet bonded to a central will pair to it.
Similar to how [bluetooth profiles](bluetooth.md) are managed between the keyboard and host devices, the bonding information will be stored with the corresponding hardware addresses of the other keyboard part, on both the central and peripheral.

In practice, this means that your split keyboard parts will automatically pair and work the first time they are all on at the same time.
However, if this process somehow went wrong or you used controllers in a different split keyboard configuration before, you will need to explicitly clear the stored bond information so that the parts can pair properly.
For this, please follow [the specified procedure](../troubleshooting/connection-issues.mdx#split-keyboard-halves-unable-to-pair) in the troubleshooting section.

:::warning
If the central keyboard part is either advertising for a pairing or waiting for disconnected peripherals, it will consume more power and drain batteries faster.
:::

## Behaviors with Locality

Most ZMK [behaviors](../keymaps/behaviors/index.mdx) are processed exclusively on the central of the split keyboard as it handles the keymap state and any communication with the host devices.
However, certain behaviors have "global" or "source" localities, where they can affect the peripherals when invoked.

### Global Locality Behaviors

These are behaviors that affect all keyboard parts, such as changing lighting effects:

- [RGB underglow behaviors](../keymaps/behaviors/underglow.md)
- [Backlight behaviors](../keymaps/behaviors/backlight.md)
- [Power management behaviors](../keymaps/behaviors/power.md)
- [Soft off behavior](../keymaps/behaviors/soft-off.md)

### Source Locality Behaviors

These behaviors only affect the keyboard part that they are invoked from:

- [Reset behaviors](../keymaps/behaviors/reset.md)

:::note[Peripheral invocation]
Peripherals must be paired and connected to the central in order to be able to activate these behaviors, even if it is possible to trigger the behavior using only keys on a particular peripheral.
This is because the key bindings are processed on the central side which would then instruct the peripheral side to run the behavior's effect.
:::

:::note[Combos]
[Combos](../keymaps/combos.md) always invoke behaviors with source locality on the central.
:::
