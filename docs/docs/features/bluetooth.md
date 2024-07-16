---
title: Bluetooth
sidebar_label: Bluetooth
---

ZMK's bluetooth functionality allows users to connect their keyboards to hosts using Bluetooth Low Energy (BLE) technology. It also is used for split keyboards to connect the two halves wirelessly.

:::note

Bluetooth 4.2 or newer is required in order to connect to a ZMK keyboard. ZMK implements advanced security using BLE's Secure Connection feature, which requires Bluetooth 4.2 at a minimum. To avoid well-known security vulnerabilities, we disallow using Legacy pairing.

:::

## Security

BLE connections between keyboards and hosts are secured by an initial pairing/bonding process that establishes long term keys (LTK) shared between the two sides, using Elliptic Curve Diffie Hellman (ECDH) for key generation. The same security is used to secure the communication between the two sides of split keyboards running ZMK.

The only known vulnerability in the protocol is a risk of an active man-in-the-middle (MITM) attack exactly during the initial pairing, which can be mitigated in the future using the Numeric Comparison association model. Support for that in ZMK is still experimental, so if you have serious concerns about an active attacker with physical proximity to your device, consider only pairing/bonding your keyboards in a controlled environment.

## Profiles

By default, ZMK supports five "profiles" for selecting which bonded host
device should receive the keyboard input.

:::note[Connection Management]

When pairing to a host device ZMK saves bond information to the selected profile. It will not replace this automatically when you initiate pairing with another device. To pair with a new device select an unused profile with or clearing the current profile, using the [`&bt` behavior](../behaviors/bluetooth.md) on your keyboard.

A ZMK device may show as "connected" on multiple hosts at the same time. This is working as intended, and only the host associated with the active profile will receive keystrokes.

:::

Failure to manage the profiles can result in unexpected/broken behavior with hosts due to bond key mismatches, so it is an important aspect of ZMK to understand.

## Bluetooth Behavior

Management of the bluetooth in ZMK is accomplished using the [`&bt` behavior](../behaviors/bluetooth.md). Be sure to refer to that documentation to learn how to manage profiles, switch between connected hosts, etc.

## Refreshing the HID Descriptor

Enabling certain features or behaviors in ZMK changes the data structure that ZMK sends over USB or BLE to host devices.
This in turn requires [HID report descriptors](https://docs.kernel.org/hid/hidintro.html) to be modified for the reports to be parsed correctly.
Firmware changes that would modify the descriptor include the following:

- Changing any of the settings under the [HID category](../config/system.md#hid), including enabling/disabling NKRO or HID indicators
- Enabling mouse features, such as adding [mouse keys](../behaviors/mouse-emulation.md) to your keymap

While the descriptor refresh happens on boot for USB, hosts will frequently cache this descriptor for BLE devices.
In order to refresh this cache, you need to remove the keyboard from the host device, clear the profile associated with the host on the keyboard, then pair again.
For Windows systems you might need to follow the additional instructions in [the section on troubleshooting connection issues](troubleshooting/connection-issues.mdx#windows-connected-but-not-working).
