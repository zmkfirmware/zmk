---
title: Battery Level
sidebar_label: Battery Level
---

If your keyboard has a battery sensor, ZMK will report its battery level to the connected bluetooth host and show it on the keyboard's display, if it has one.

For split keyboards, only the battery level of the central (usually left) side is reported over bluetooth.

:::note

Windows may not properly ask the keyboard to notify it of changes in battery level, so the level shown may be out of date.

:::

## Adding a Battery Sensor to a Board

To enable a battery sensor on a new board, add the driver for the sensor to your board's `.dts` file. ZMK provides two drivers for estimating the battery level using its voltage:

- `zmk,battery-voltage-divider`: Reads the voltage on an analog input pin.
- `zmk,battery-nrf-vddh`: Reads the power supply voltage on a Nordic nRF52's VDDH pin.

See the [battery level configuration page](../config/battery.md) for the configuration supported by each driver provided by ZMK.

Zephyr also provides some drivers for fuel gauge ICs such as the TI bq274xx series and Maxim MAX17xxx series. If you use a battery sensor that does not have an existing driver, you will need to write a new driver that supports the `SENSOR_CHAN_GAUGE_STATE_OF_CHARGE` sensor channel and contribute it to Zephyr or ZMK.

Once you have the sensor driver defined, add a `zmk,battery` property to the `chosen` node and set it to reference the sensor node. For example:

```
/ {
    chosen {
      zmk,battery = &vbatt;
    };

    vbatt: vbatt {
        compatible = "zmk,battery-nrf-vddh";
        label = "VBATT";
    };
}
```
