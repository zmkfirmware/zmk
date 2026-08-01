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
| `CONFIG_ZMK_BATTERY_CHARGE_STATUS`   | bool | Report whether the battery is charging                 | y       |

`CONFIG_ZMK_BATTERY_CHARGE_STATUS` requires a [charge status node](#battery-charge-status) in your devicetree, and has no effect without one.

:::note[Default setting]

While `CONFIG_ZMK_BATTERY_REPORTING` is disabled by default it is implied by `CONFIG_ZMK_BLE`, thus any board with BLE enabled will have this automatically enabled unless explicitly overridden.

:::

:::note[BLE reporting on MacOS]

On macOS the BLE battery reporting packets can cause the computer to wakeup from sleep. To prevent this, the battery _reporting_ service can be disabled by setting `CONFIG_BT_BAS=n`. This setting is independent of battery _monitoring_, for instance the battery level can still be indicated on a display.

:::

### Peripheral Battery Monitoring

You can [configure ZMK to allow support for peripheral battery monitoring over BLE](split.md) (e.g. when having a split keyboard with two independent and wirelessly connected sides).
If you want to report the battery levels of both sides of a split keyboard, you should have both `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_PROXY` and `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING` set to `y`.

:::note[Displaying both battery levels on your host]

Host support for multiple battery levels is undefined. It appears that in most of the cases only the main battery is being reported. In order to correctly display all the battery values, you probably need a special application or script.

:::

### Devicetree

Applies to: [`/chosen` node](https://docs.zephyrproject.org/4.1.0/build/dts/intro-syntax-structure.html#aliases-and-chosen-nodes)

| Property      | Type | Description                                   |
| ------------- | ---- | --------------------------------------------- |
| `zmk,battery` | path | The node for the battery sensor driver to use |

## Battery Voltage Divider Sensor

Driver for reading the voltage of a battery using an ADC connected to a voltage divider.

### Devicetree

Applies to: `compatible = "zmk,battery-voltage-divider"`

See [Zephyr's voltage divider documentation](https://docs.zephyrproject.org/4.1.0/build/dts/api/bindings/iio/afe/voltage-divider.html).

## Battery Charge Status

Reports whether the battery is charging, by reading the status output(s) of the board's battery charger IC.

A battery sensor only measures how full the battery is, which cannot tell you whether it is currently being charged. That has to come from the charger, so this requires a board that routes a charger status pin to a GPIO the MCU can read. Many keyboard boards do not, in which case the charge state is reported as unknown.

### Devicetree

Applies to: `compatible = "zmk,battery-charge-status"`

Definition file: [zmk/app/dts/bindings/zmk,battery-charge-status.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Cbattery-charge-status.yaml)

| Property         | Type       | Description                                                 | Default |
| ---------------- | ---------- | ----------------------------------------------------------- | ------- |
| `charging-gpios` | GPIO array | Charger output asserted while the battery is charging       |         |
| `full-gpios`     | GPIO array | Charger output asserted once charging has finished          |         |
| `debounce-ms`    | int        | Time to wait for the status pins to settle before reporting | 100     |

Charger status pins are usually open drain and pulled low to signal a state, so they are normally described with `GPIO_ACTIVE_LOW`, plus `GPIO_PULL_UP` if the board has no external pull up.

A charger with a single status pin, such as the MCP73831, cannot distinguish "charge complete" from "no power applied", so leave `full-gpios` unset and a battery that is not charging is reported as discharging:

```dts
/ {
    charge_status {
        compatible = "zmk,battery-charge-status";
        charging-gpios = <&gpio0 5 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
    };
};
```

A charger with a separate charge complete output, such as the TP4056, can report both:

```dts
/ {
    charge_status {
        compatible = "zmk,battery-charge-status";
        charging-gpios = <&gpio0 5 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
        full-gpios = <&gpio0 6 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
    };
};
```

On a split keyboard each half reads its own charger, and peripherals send their charge state to the central over the Battery Level Status characteristic. A peripheral built without a charge status node simply never reports one.

## nRF VDDH Battery Sensor

Driver for reading the voltage of a battery using a Nordic nRF52's VDDH pin.

### Devicetree

Applies to: `compatible = "zmk,battery-nrf-vddh"`

Definition file: [zmk/app/module/dts/bindings/sensor/zmk,battery-nrf-vddh.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/module/dts/bindings/sensor/zmk%2Cbattery-nrf-vddh.yaml)

This driver has no configuration.
