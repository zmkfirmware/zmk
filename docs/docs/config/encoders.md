---
title: Encoder Configuration
sidebar_label: Encoders
---

See the [Encoders feature page](../features/encoders.md) for more details, including instructions for adding encoder support to a board.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## EC11 Encoders

### Kconfig

Definition file: [zmk/app/module/drivers/sensor/ec11/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/module/drivers/sensor/ec11/Kconfig)

| Config                          | Type | Description                      | Default |
| ------------------------------- | ---- | -------------------------------- | ------- |
| `CONFIG_EC11`                   | bool | Enable EC11 encoders             | n       |
| `CONFIG_EC11_THREAD_PRIORITY`   | int  | Priority of the encoder thread   | 10      |
| `CONFIG_EC11_THREAD_STACK_SIZE` | int  | Stack size of the encoder thread | 1024    |

If `CONFIG_EC11` is enabled, exactly one of the following options must be set to `y`:

| Config                              | Type | Description                                     |
| ----------------------------------- | ---- | ----------------------------------------------- |
| `CONFIG_EC11_TRIGGER_NONE`          | bool | No trigger (encoders are disabled)              |
| `CONFIG_EC11_TRIGGER_GLOBAL_THREAD` | bool | Process encoder interrupts on the global thread |
| `CONFIG_EC11_TRIGGER_OWN_THREAD`    | bool | Process encoder interrupts on their own thread  |

### Devicetree

#### Keymap Sensor Config

For shields/boards that export a `sensors` node configuration label, both global and per-sensor settings can be set by overriding the properties there.

To override the general settings, update them on the exported `sensors` node, e.g.:

```
&sensors {
    triggers-per-rotation = <18>;
};
```

Per sensor overrides can be added with ordered nested nodes with the correct overrides, e.g.:

```
&sensors {
    left_config {
        triggers-per-rotation = <18>;
    };

    right_config {
        triggers-per-rotation = <24>;
    };
};
```

:::note

The names of the child nodes are not important, and are applied in order to the sensors listed in the `sensors` property of the sensors node.

:::

Applies to the node and child nodes of: `compatible = "zmk,keymap-sensors"`

Definition file: [zmk/app/drivers/zephyr/dts/bindings/zmk,keymap-sensors.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/drivers/zephyr/dts/bindings/zmk%2Ckeymap-sensors.yaml)

| Property                | Type | Description                                                     | Default |
| ----------------------- | ---- | --------------------------------------------------------------- | ------- |
| `triggers-per-rotation` | int  | Number of times to trigger the bound behavior per full rotation |         |

#### EC11 Nodes

Applies to: `compatible = "alps,ec11"`

Definition file: [zmk/app/module/dts/bindings/sensor/alps,ec11.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/module/dts/bindings/sensor/alps%2Cec11.yaml)

| Property  | Type       | Description                                    | Default |
| --------- | ---------- | ---------------------------------------------- | ------- |
| `label`   | string     | Unique label for the node                      |         |
| `a-gpios` | GPIO array | GPIO connected to the encoder's A pin          |         |
| `b-gpios` | GPIO array | GPIO connected to the encoder's B pin          |         |
| `steps`   | int        | Number of encoder pulses per complete rotation |         |
