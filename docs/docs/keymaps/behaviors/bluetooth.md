---
title: Bluetooth Behavior
sidebar_label: Bluetooth
---

## Summary

The bluetooth behavior allows management of various settings and states related to the bluetooth connection(s)
between the keyboard and the host. By default, ZMK supports five "profiles" for selecting which bonded host
computer/laptop/keyboard should receive the keyboard input; many of the commands here operate on those profiles.

:::note[Connection Management]
When pairing to a host device ZMK saves bond information to the selected
profile. It will not replace this when you initiate pairing with another device.
To pair with a new device, select a profile that doesn't have a pairing with `BT_SEL`, `BT_NXT` or
`BT_PRV` bindings, or clear an already paired profile using `BT_CLR` or `BT_CLR_ALL`.

A ZMK device may show as "connected" on multiple hosts at the same time. This is working as intended, and only the host associated with the active profile will receive keystrokes.

An _inactive_ connected profile can be explicitly disconnected using the `BT_DISC` behavior. This can be helpful in
cases when host devices behave differently when a bluetooth keyboard is connected, for example by hiding their on-screen
keyboard. Note that at present the active bluetooth profile will immediately reconnect if disconnected. This is true
even if OUT_USB is selected. To remain disconnected, another bluetooth profile must be first selected using (e.g.)
`BT_SEL`.

:::

## Bluetooth Command Defines

Bluetooth command defines are provided through the [`dt-bindings/zmk/bt.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/bt.h) header,
which is added at the top of the keymap file:

```dts
#include <dt-bindings/zmk/bt.h>
```

This will allow you to reference the actions defined in this header such as `BT_CLR`.

Here is a table describing the command for each define:

| Define       | Action                                                                                                                                                                             |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `BT_CLR`     | Clear bond information between the keyboard and host for the selected profile.                                                                                                     |
| `BT_CLR_ALL` | Clear bond information between the keyboard and host for all profiles.                                                                                                             |
| `BT_NXT`     | Switch to the next profile, cycling through to the first one when the end is reached.                                                                                              |
| `BT_PRV`     | Switch to the previous profile, cycling through to the last one when the beginning is reached.                                                                                     |
| `BT_SEL`     | Select the 0-indexed profile by number; must include a number as an argument in the keymap to work correctly, e.g. `BT_SEL 0`.                                                     |
| `BT_DISC`    | Disconnect from the 0-indexed profile by number, if it's currently connected and inactive; must include a number as an argument in the keymap to work correctly, e.g. `BT_DISC 0`. |

:::note[Selected profile persistence]
The profile that is selected by the `BT_SEL`/`BT_PRV`/`BT_NXT` actions will be saved to flash storage and hence persist across restarts and firmware flashes.
However it will only be saved after [`CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE`](../../config/system.md#general) milliseconds in order to reduce potential wear on the flash memory.
:::

## Bluetooth Behavior

The bluetooth behavior completes an bluetooth action given on press.

### Behavior Binding

- Reference: `&bt`
- Parameter #1: The bluetooth command define, e.g. `BT_CLR`
- Parameter #2: Only applies to `BT_SEL` and `BT_DISC`, and is the 0-indexed profile by number

### Examples

1. Behavior binding to clear the paired host for the selected profile:

   ```dts
   &bt BT_CLR
   ```

1. Behavior binding to select the next profile:

   ```dts
   &bt BT_NXT
   ```

1. Behavior binding to select the previous profile:

   ```dts
   &bt BT_PRV
   ```

1. Behavior binding to select the 2nd profile (passed parameters are [zero based](https://en.wikipedia.org/wiki/Zero-based_numbering)):

   ```dts
   &bt BT_SEL 1
   ```

## Bluetooth Pairing and Profiles

ZMK support bluetooth “profiles” which allows connection to multiple devices (5 by default). Each profile stores the bluetooth MAC address of a peer, which can be empty if a profile has not been paired with a device yet. Upon switching to a profile, ZMK does the following:

- If a profile has not been paired with a peer yet, ZMK automatically advertises itself as connectable. You can discover your keyboard from bluetooth scanning on your laptop / tablet. If you try to connect, it will trigger the _pairing_ procedure. After pairing, the bluetooth MAC address of the peer device will be stored in the current profile. Pairing also negotiates a random key for secure communication between the device and the keyboard.
- If a profile has been paired but the peer is not connected yet, ZMK will also advertise itself as connectable. In the future, the behavior might change to _direct advertising_ which only target the peer with the stored bluetooth MAC address. In this state, if the peer is powered on and moved within the distance of bluetooth signal coverage, it should automatically connect to the keyboard.
- If a profile has been paired and is currently connected, ZMK will not advertise it as connectable.

The bluetooth MAC address and negotiated keys during pairing are stored in the permanent storage on your chip and can be reused even after reflashing the firmware. If for some reason you want to delete the stored information, you can bind the `BT_CLR` behavior described above to a key and use it to clear the _current_ profile.

:::note[Number of Profiles]
Please note there are five available Bluetooth profiles by default. If you need to adjust the number of available profiles, set `CONFIG_BT_MAX_CONN` _and_ `CONFIG_BT_MAX_PAIRED` to the desired number of profiles, `n`, or `n+1` for split keyboards, in your `zmk-config` `.conf` file.
:::

:::note
If you clear bond of a paired profile, make sure you do the same thing on the peer device as well (typically achieved by _removing_ or _forgetting_ the bluetooth connection). Otherwise the peer will try to connect to your keyboard whenever it discovers it. But while the MAC address of both devices could remain the same, the security key no longer match: the peer device still possess the old key negotiated in the previous pairing procedure, but our keyboard firmware has deleted that key. So the connection will fail. If you [enabled USB logging](../../development/usb-logging.mdx), you might see a lot of failed connection attempts due to the reason of “Security failed”.
:::
