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

:::note Connection Management

When pairing to a host device ZMK saves bond information to the selected profile. It will not replace this automatically when you initiate pairing with another device. To pair with a new device select an unused profile with or clearing the current profile, using the [`&bt` behavior](../behaviors/bluetooth.md) on your keyboard.

A ZMK device may show as "connected" on multiple hosts at the same time. This is working as intended, and only the host associated with the active profile will receive keystrokes.

:::

Failure to manage the profiles can result in unexpected/broken behavior with hosts due to bond key mismatches, so it is an important aspect of ZMK to understand.

## Bluetooth Behavior

Management of the bluetooth in ZMK is accomplished using the [`&bt` behavior](../behaviors/bluetooth.md). Be sure to refer to that documentation to learn how to manage profiles, switch between connected hosts, etc.

## Troubleshooting

### Connectivity Issues

Some users may experience a poor connection between the keyboard and the host. This might be due to poor quality BLE hardware, a metal enclosure on the keyboard or host, or the distance between them. Increasing the transmit power of the keyboard's BLE radio may reduce the severity of this problem. To do this, set the `CONFIG_BT_CTLR_TX_PWR_PLUS_8` configuration value in the `.conf` file of your user config directory as such:

```
CONFIG_BT_CTLR_TX_PWR_PLUS_8=y
```

For the `nRF52840`, the value `PLUS_8` can be set to any multiple of four between `MINUS_20` and `PLUS_8`. The default value for this config is `0`, but if you are having connection issues it is recommended to set it to `PLUS_8` because the power consumption difference is negligible. For more information on changing the transmit power of your BLE device, please refer to [the Zephyr docs.](https://docs.zephyrproject.org/latest/kconfig.html#CONFIG_BT_CTLR_TX_PWR)

:::info
This setting can also improve the connection strength between the keyboard halves for split keyboards.
:::

### Using bluetooth output with USB power

If you want to test bluetooth output on your keyboard and are powering it through the USB connection rather than a battery, you will be able to pair with a host device but may not see keystrokes sent. In this case you need to use the [output selection behavior](../behaviors/outputs.md) to prefer sending keystrokes over bluetooth rather than USB. This might be necessary even if you are not powering from a device capable of receiving USB inputs, such as a USB charger.

## Known Issues

There are a few known issues related to BLE and ZMK:

### Windows Battery Reporting

There is a known issue with Windows failing to update the battery information after connecting to a ZMK keyboard. You can work around this Windows bug by overriding a [Bluetooth config variable](../config/bluetooth.md) to force battery notifications even if a host neglects to subscribe to them:

```
CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION=n
```

### macOS Connected But Not Working

If you attempt to pair a ZMK keyboard from macOS in a way that causes a bonding issue, macOS may report the keyboard as connected, but fail to actually work. If this occurs:

1. Remove the keyboard from macOS using the Bluetooth control panel.
1. Invoke `&bt BT_CLR` on the keyboard while the profile associated with the macOS device is active, by pressing the correct keys for your particular keymap.
1. Try connecting again from macOS.

### Windows Connected But Not Working

Occasionally pairing the keyboard to a Windows device might result in a state where the keyboard is connected but does not send any key strokes.
If this occurs:

1. Remove the keyboard from Windows using the Bluetooth settings.
1. Invoke `&bt BT_CLR` on the keyboard while the profile associated with the Windows device is active, by pressing the correct keys for your particular keymap.
1. Turn off Bluetooth from Windows settings, then turn it back on.
1. Pair the keyboard to the Windows device.

If this doesn't help, try following the procedure above but replace step 3 with one of the following:

- Restart the Windows device
- Open "Device Manager," turn on "Show hidden devices" from the "View" menu, then find and delete the keyboard under the "Bluetooth" item
