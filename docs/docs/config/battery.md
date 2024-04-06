---
title: Battery Level
sidebar_label: Battery Level
---

See the [battery level feature page](../features/battery.md) for more details on configuring a battery sensor.

See [Configuration Overview](index.md) for instructions on how to change these settings.

### Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                               | Type | Description                                            | Default |
| ------------------------------------ | ---- | ------------------------------------------------------ | ------- |
| `CONFIG_ZMK_BATTERY_REPORTING`       | bool | Enables/disables all battery level detection/reporting | n       |
| `CONFIG_ZMK_BATTERY_REPORT_INTERVAL` | int  | Battery level report interval in seconds               | 60      |

If `CONFIG_ZMK_BATTERY_REPORTING` is enabled, exactly zero or one of the following options may be set to `y`. The first is used if none are set. This should be set to match the type of sensor that is selected for the `zmk,battery` chosen node.

| Config                                                    | Description                                                                                                  |
| --------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| `CONFIG_ZMK_BATTERY_REPORTING_FETCH_MODE_LITHIUM_VOLTAGE` | Read a sensor that supports `SENSOR_CHAN_VOLTAGE` and estimate state of charge for a Li-Ion or LiPo battery. |
| `CONFIG_ZMK_BATTERY_REPORTING_FETCH_MODE_STATE_OF_CHARGE` | Read a sensor that supports `SENSOR_CHAN_GAUGE_STATE_OF_CHARGE`.                                             |

ZMK does not yet support Zephyr's recently added fuel gauge API.

:::note[Default setting]

While `CONFIG_ZMK_BATTERY_REPORTING` is disabled by default, it is implied by `CONFIG_ZMK_BLE`, thus any board with BLE enabled will have this automatically enabled unless explicitly overriden.

:::

:::note[BLE reporting on MacOS]

On macOS the BLE battery reporting packets can cause the computer to wakeup from sleep. To prevent this, the battery _reporting_ service can be disabled by setting `CONFIG_BT_BAS=n`. This setting is independent of battery _monitoring_, for instance the battery level can still be indicated on a display.

:::

### Peripheral battery monitoring

You can [configure ZMK to allow support for peripheral battery monitoring over BLE](system.md#split-keyboards) (e.g. when having a split keyboard with two independent and wirelessly connected sides).
If you want to report the battery levels of both sides of a split keyboard, you should have both `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_PROXY` and `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING` set to `y`.

:::note[Displaying both battery levels on your host]

Host support for multiple battery levels is undefined. It appears that in most of the cases only the main battery is being reported. In order to correctly display all the battery values, you probably need a special application or script.

:::

### Devicetree

Applies to: [`/chosen` node](https://docs.zephyrproject.org/3.5.0/build/dts/intro-syntax-structure.html#aliases-and-chosen-nodes)

| Property      | Type | Description                                   |
| ------------- | ---- | --------------------------------------------- |
| `zmk,battery` | path | The node for the battery sensor driver to use |

## Voltage Divider Sensor

Driver for reading the voltage of a battery using an ADC connected to a voltage divider.

Set `CONFIG_ZMK_BATTERY_REPORTING_FETCH_MODE_LITHIUM_VOLTAGE=y` when using this driver.

### Devicetree

Applies to: `compatible = "voltage-divider"`

See [Zephyr's voltage divider documentation](https://docs.zephyrproject.org/3.5.0/build/dts/api/bindings/iio/afe/voltage-divider.html).

ZMK also provides a `compatible = "zmk,battery-voltage-divider"` driver, which is deprecated. Use the improved driver from Zephyr instead.

## nRF VDDH Battery Sensor

Driver for reading the voltage of a battery using a Nordic nRF52's VDDH pin.

Set `CONFIG_ZMK_BATTERY_REPORTING_FETCH_MODE_LITHIUM_VOLTAGE=y` when using this driver.

### Devicetree

Applies to: `compatible = "zmk,battery-nrf-vddh"`

Definition file: [zmk/app/module/dts/bindings/sensor/zmk,battery-nrf-vddh.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/module/dts/bindings/sensor/zmk%2Cbattery-nrf-vddh.yaml)

This driver has no configuration.
