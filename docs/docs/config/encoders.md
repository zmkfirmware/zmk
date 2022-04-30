---
title: Encoder Configuration
sidebar_label: Encoders
---

See the [Encoders feature page](../features/encoders.md) for more details, including instructions for adding encoder support to a board.

See [Configuration Overview](index.md) for instructions on how to change these settings.

## EC11 Encoders

### Kconfig

Definition file: [zmk/app/drivers/sensor/ec11/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/drivers/sensor/ec11/Kconfig)

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

Applies to: `compatible = "alps,ec11"`

Definition file: [zmk/app/drivers/zephyr/dts/bindings/sensor/alps,ec11.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/drivers/zephyr/dts/bindings/sensor/alps%2Cec11.yaml)

| Property     | Type       | Description                           | Default |
| ------------ | ---------- | ------------------------------------- | ------- |
| `label`      | string     | Unique label for the node             |         |
| `a-gpios`    | GPIO array | GPIO connected to the encoder's A pin |         |
| `b-gpios`    | GPIO array | GPIO connected to the encoder's B pin |         |
| `resolution` | int        | Number of encoder pulses per tick     | 1       |
