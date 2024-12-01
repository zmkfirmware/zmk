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

:::note[Default setting]

While `CONFIG_ZMK_BATTERY_REPORTING` is disabled by default it is implied by `CONFIG_ZMK_BLE`, thus any board with BLE enabled will have this automatically enabled unless explicitly overridden.

:::

:::note[BLE reporting on MacOS]

On macOS the BLE battery reporting packets can cause the computer to wakeup from sleep. To prevent this, the battery _reporting_ service can be disabled by setting `CONFIG_BT_BAS=n`. This setting is independent of battery _monitoring_, for instance the battery level can still be indicated on a display.

:::

### Peripheral Battery Monitoring

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

## Battery Voltage Divider Sensor

Driver for reading the voltage of a battery using an ADC connected to a voltage divider. This driver can also read a GPIO pin to detect whether the battery is charging or not. This requires supported hardware (a battery charging IC with an output to indicate charging status). This functionality is optional, if the hardware doesn't support it the `chg-gpios` devicetree configuration does not have to be set.

### Devicetree

Applies to: `compatible = "zmk,battery-voltage-divider"`

Definition file: [zmk/app/module/dts/bindings/sensor/zmk,battery-voltage-divider.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/module/dts/bindings/sensor/zmk,battery-voltage-divider.yaml)

The ZMK battery voltage divider includes the [Zephyr voltage divider](https://docs.zephyrproject.org/latest/build/dts/api/bindings/adc/voltage-divider.html) and adds on additional functionality.

| Property    | Type       | Description                                      | Default |
| ----------- | ---------- | ------------------------------------------------ | ------- |
| `chg-gpios` | GPIO array | GPIO connected to the charging IC's charging pin |         |

:::note Charging indication

The battery charging status is not consumed by any built-in indicators currently and it cannot be conveyed to the host over BLE.

:::

## nRF VDDH Battery Sensor

Driver for reading the voltage of a battery using a Nordic nRF52's VDDH pin. This driver can also read a GPIO pin to detect whether the battery is charging or not. This requires supported hardware (a battery charging IC with an output to indicate charging status). This functionality is optional, if the hardware doesn't support it the `chg-gpios` devicetree configuration does not have to be set.

### Devicetree

Applies to: `compatible = "zmk,battery-nrf-vddh"`

Definition file: [zmk/app/module/dts/bindings/sensor/zmk,battery-nrf-vddh.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/module/dts/bindings/sensor/zmk,battery-nrf-vddh.yaml)

| Property    | Type       | Description                                      | Default |
| ----------- | ---------- | ------------------------------------------------ | ------- |
| `chg-gpios` | GPIO array | GPIO connected to the charging IC's charging pin |         |

:::note Charging indication

The battery charging status is not consumed by any built-in indicators currently and it cannot be conveyed to the host over BLE.

:::
