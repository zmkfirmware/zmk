---
title: Low Power States
sidebar_label: Low Power States
---

## Idle

In the idle state, peripherals such as displays and lighting are disabled, but the keyboard remains connected to Bluetooth so it can immediately respond when you press a key. Idle state is entered automatically after a timeout period that is [30 seconds by default](../config/power.md#low-power-states).

## Deep Sleep

In the deep sleep state, the keyboard enters a software power-off state. Among others, this:

- Disconnects the keyboard from all Bluetooth connections
- Disables any peripherals such as displays and lighting
- If possible, disables external power output
- Clears the contents of RAM, including any unsaved [Studio](studio.md) changes

This state uses very little power, but it may take a few seconds to reconnect after waking. A [wakeup source](#wakeup-sources) is required to wake from deep sleep.

### Config

Deep sleep must be enabled via its corresponding [config](../config/power.md#low-power-states).

### Wakeup Sources

Using deep sleep requires `kscan` nodes to have the `wakeup-source` property to enable them to wake the keyboard, e.g.:

```dts
/ {
    kscan: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        diode-direction = "col2row";
        wakeup-source;

        ...
    };
};
```

It is recommended to add the `wakeup-source` property to `kscan` devices even if the deep sleep feature is not used -- there is no downside to it.

## Soft Off

The soft off feature is used to turn the keyboard on and off explicitly, rather than through a timeout like the deep sleep feature. Depending on the keyboard, this may be through a dedicated on/off push button defined in hardware, or merely through an additional binding in the keymap to turn the device off and an existing reset button to turn the device back on.

The feature is intended as an alternative to using a hardware switch to physically cut power from the battery to the keyboard. This can be useful for existing PCBs not designed for wireless that don't have a power switch, or for new designs that favor a push button on/off like found on other devices. It yields power savings comparable to the deep sleep state.

:::note

The device enters the same software power-off state as in deep sleep, but is significantly more restrictive in the sources which can wake it. Power is _not_ technically removed from the entire system, unlike a hardware switch.

:::

A device can be put in the soft off state by:

- Triggering a hardware-defined dedicated GPIO pin, if one exists;
- Triggering the [soft off behavior](../keymaps/behaviors/soft-off.md) from the keymap.

Once in the soft off state, the device can only be woken up by:

- Triggering any GPIO pin specified to enable waking from sleep, if one exists;
- Pressing a reset button found on the device.

The GPIO pin used to wake from sleep can be a hardware-defined one, such as for a dedicated on-off push button, or it can be a single specific key switch reused for waking up (which may be accidentally pressed, e.g. while the device is being carried in a bag). To allow the simultaneous pressing of multiple key switches to trigger and exit soft off, some keyboards make use of additional hardware to integrate the dedicated GPIO pin into the keyboard matrix.

### Config

Soft off must be enabled via [its corresponding config](../config/power.md#low-power-states) before it can be used.

### Using Soft Off

If your keyboard has hardware designed to take advantage of soft off, refer to your keyboard's documentation.

For keyboards which do not have such hardware, using soft off is as simple as placing the [soft off behavior](../keymaps/behaviors/soft-off.md) in your keymap and then invoking it.

You can then wake up the keyboard by pressing the reset button once, and repeating this for each side for split keyboards.

### Adding Soft Off to a Keyboard

Please refer to the [corresponding page under hardware integration](../development/hardware-integration/soft-off-setup.mdx) for details.
