---
title: Pointing Devices
---

ZMK supports physical pointing devices, as well as [mouse emulation behaviors](../keymaps/behaviors/mouse-emulation.md) for sending HID pointing events to hosts.

## Configuration

To enable the pointer functionality, you must set `CONFIG_ZMK_POINTING=y` in your config. See the [pointer configuration](../config/pointing.md) for the full details.

:::warning

When enabling the feature, changes are made to the HID report descriptor which some hosts may not pick up automatically over BLE. Be sure to [remove and re-pair to your hosts](bluetooth.md#refreshing-the-hid-descriptor) once you enable the feature.

:::

## Mouse Emulation

Mouse emulation allows you to use your keyboard as a pointing device without using dedicated pointer hardware, like an integrated trackpad, trackball, etc. By adding new bindings in your keymap like `&mmv MOVE_UP` you can make key presses send mouse HID events to your connected hosts.

See the [mouse emulation behaviors](../keymaps/behaviors/mouse-emulation.md) for details.

## Physical Pointing Devices

There are a few drivers available for supporting physical pointing devices integrated into a ZMK powered device. When doing so, you can use your device as both a keyboard and a pointing device with any connected hosts. The functionality can be extended further, e.g. slow mode, scroll mode, temporary mouse layers, etc. by configuring [input processors](#input-processors) linked to the physical pointing device.

For more information, refer to the [pointer hardware integration](../development/hardware-integration/pointing.mdx) documentation.

## Input Processors

Input processors are small pieces of functionality that process and optionally modify events generated from emulated and physical pointing devices. Processors can do things like scaling movement values to make them larger or smaller for detailed work, swapping the event types to turn movements into scroll events, or temporarily enabling an extra layer while the pointer is in use.

For more details, see the [input processors](../keymaps/input-processors/index.md) section of the keymap documentation.

## Input Listeners

Listeners are the key piece that integrate the low level input devices to the rest of the ZMK system. In particular, listeners subscribe to input events from the linked device, and when a given event occurs (e.g. X/Y movement), apply any input processors before sending those events to the HID system for notification to the host. The main way to modify the way a pointer behaves is by configuring the input processors for a given listener.

For more details on assigning processors to your listeners, see the [input processor usage](../keymaps/input-processors/usage.md) documentation.
