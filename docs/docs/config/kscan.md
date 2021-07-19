---
title: Keyboard Scan Configuration
sidebar_label: Keyboard Scan
---

See [Configuration Overview](/docs/config/index) for instructions on how to
change these settings.

## Common

### Kconfig

Definition files:

- [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)
- [zmk/app/drivers/kscan/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/drivers/kscan/Kconfig)

| Config                              | Type | Description                                            | Default |
| ----------------------------------- | ---- | ------------------------------------------------------ | ------- |
| `CONFIG_ZMK_KSCAN_GPIO_DRIVER`      | bool | Enable GPIO keyboard scan driver to detect key presses | y       |
| `CONFIG_ZMK_KSCAN_EVENT_QUEUE_SIZE` | int  | Size of the event queue for kscan events               | 4       |
| `CONFIG_ZMK_KSCAN_INIT_PRIORITY`    | int  | Keyboard scan device driver initialization priority    | 40      |

### Devicetree

Applies to: [`/chosen` node](https://docs.zephyrproject.org/latest/guides/dts/intro.html#aliases-and-chosen-nodes)

| Property               | Type | Description                                                   |
| ---------------------- | ---- | ------------------------------------------------------------- |
| `zmk,kscan`            | path | The node for the keyboard scan driver to use                  |
| `zmk,matrix_transform` | path | The node for the [matrix transform](#matrix-transform) to use |

## Demux Driver

Keyboard scan driver which works like a regular matrix but uses a demultiplexer
to drive the rows or columns. This allows N GPIOs, to drive N<sup>2</sup> rows
or columns instead of just N like with a regular matrix.

### Devicetree

Applies to: `compatible = "zmk,kscan-gpio-demux"`

Definition file: [zmk/app/drivers/zephyr/dts/bindings/kscan/zmk,kscan-gpio-demux.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/drivers/zephyr/dts/bindings/kscan/zmk%2Ckscan-gpio-demux.yaml)

| Property                | Type       | Description                      | Default |
| ----------------------- | ---------- | -------------------------------- | ------- |
| `label`                 | string     | Unique label for the node        |         |
| `input-gpios`           | GPIO array | Input GPIOs                      |         |
| `output-gpios`          | GPIO array | Demultiplexer address GPIOs      |         |
| `debounce-period`       | int        | Debounce period in milliseconds  | 5       |
| `polling-interval-msec` | int        | Polling interval in milliseconds | 25      |

## Direct GPIO Driver

Keyboard scan driver where each key has a dedicated GPIO.

### Kconfig

Definition file: [zmk/app/drivers/kscan/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/drivers/kscan/Kconfig)

| Config                            | Type | Description                                      | Default |
| --------------------------------- | ---- | ------------------------------------------------ | ------- |
| `CONFIG_ZMK_KSCAN_DIRECT_POLLING` | bool | Poll for key presses instead of using interrupts | n       |

### Devicetree

Applies to: `compatible = "zmk,kscan-gpio-direct"`

Definition file: [zmk/app/drivers/zephyr/dts/bindings/kscan/zmk,kscan-gpio-direct.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/drivers/zephyr/dts/bindings/kscan/zmk%2Ckscan-gpio-direct.yaml)

| Property          | Type       | Description                     | Default |
| ----------------- | ---------- | ------------------------------- | ------- |
| `label`           | string     | Unique label for the node       |         |
| `input-gpios`     | GPIO array | Input GPIOs (one per key)       |         |
| `debounce-period` | int        | Debounce period in milliseconds | 5       |

## Matrix Driver

Keyboard scan driver where keys are arranged on a matrix with one GPIO per row
and column.

Definition file: [zmk/app/drivers/kscan/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/drivers/kscan/Kconfig)

| Config                            | Type | Description                                      | Default |
| --------------------------------- | ---- | ------------------------------------------------ | ------- |
| `CONFIG_ZMK_KSCAN_MATRIX_POLLING` | bool | Poll for key presses instead of using interrupts | n       |

### Devicetree

Applies to: `compatible = "zmk,kscan-gpio-matrix"`

Definition file: [zmk/app/drivers/zephyr/dts/bindings/kscan/zmk,kscan-gpio-matrix.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/drivers/zephyr/dts/bindings/kscan/zmk%2Ckscan-gpio-matrix.yaml)

| Property          | Type       | Description                                                  | Default     |
| ----------------- | ---------- | ------------------------------------------------------------ | ----------- |
| `label`           | string     | Unique label for the node                                    |             |
| `row-gpios`       | GPIO array | Matrix row GPIOs in order, starting from the top row         |             |
| `col-gpios`       | GPIO array | Matrix column GPIOs in order, starting from the leftmost row |             |
| `debounce-period` | int        | Debounce period in milliseconds                              | 5           |
| `diode-direction` | string     | The direction of the matrix diodes                           | `"row2col"` |

The `diode-direction` property must be one of:

| Value       | Description                                                           |
| ----------- | --------------------------------------------------------------------- |
| `"row2col"` | Diodes point from rows to columns (cathodes are connected to columns) |
| `"col2row"` | Diodes point from columns to rows (cathodes are connected to rows)    |

## Composite Driver

Keyboard scan driver which combines multiple other keyboard scan drivers.

### Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                              | Type | Description                   | Default |
| ----------------------------------- | ---- | ----------------------------- | ------- |
| `CONFIG_ZMK_KSCAN_COMPOSITE_DRIVER` | bool | Enable composite kscan driver | n       |

### Devicetree

Applies to : `compatible = "zmk,kscan-composite"`

Definition file: [zmk/app/dts/bindings/zmk,kscan-composite.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk,kscan-composite.yaml)

| Property | Type   | Description                                   | Default |
| -------- | ------ | --------------------------------------------- | ------- |
| `label`  | string | Unique label for the node                     |         |
| `rows`   | int    | The number rows of in the composite matrix    |         |
| `cols`   | int    | The number columns of in the composite matrix |         |

The `zmk,kscan-composite` node should have one child node per keyboard scan driver
that should be composited. Each child node can have the following properties:

| Property        | Type    | Description                                                                    | Default |
| --------------- | ------- | ------------------------------------------------------------------------------ | ------- |
| `label`         | string  | Unique label for the node                                                      |         |
| `kscan`         | phandle | Label of the kscan driver to include                                           |         |
| `row-offset`    | int     | Shifts row 0 of the included driver to a new row in the composite matrix       | 0       |
| `column-offset` | int     | Shifts column 0 of the included driver to a new column in the composite matrix | 0       |

### Example Configuration

For example, consider a macropad with a 3x3 matrix and two direct GPIO keys:

<div style={{columns: 2, marginBottom: '1em'}}>
<div style={{breakInside: 'avoid'}}>
Matrix:

<table>
    <thead>
        <tr><th></th><th>Col 0</th><th>Col 1</th><th>Col 2</th></tr>
    </thead>
    <tbody>
        <tr><th>Row 0</th><td>A0</td><td>A1</td><td>A2</td></tr>
        <tr><th>Row 1</th><td>A3</td><td>A4</td><td>A5</td></tr>
        <tr><th>Row 2</th><td>A6</td><td>A7</td><td>A8</td></tr>
    </tbody>
</table>
</div>

<div style={{breakInside: 'avoid'}}>
Direct GPIO:

<table>
    <thead>
        <tr><th></th><th>Col 0</th><th>Col 1</th></tr>
    </thead>
    <tbody>
        <tr><th>Row 0</th><td>B0</td><td>B1</td></tr>
    </tbody>
</table>
</div>
</div>

To combine them, we need to create a composite matrix with enough rows and columns
to fit both sets of keys without overlapping, then set row and/or columns offsets
to shift them so they do not overlap.

One possible way to do this is a 3x4 matrix where the direct GPIO keys are
shifted to below the matrix keys...

<table>
    <thead>
        <tr><th></th><th>Col 0</th><th>Col 1</th><th>Col 2</th></tr>
    </thead>
    <tbody>
        <tr><th>Row 0</th><td>A0</td><td>A1</td><td>A2</td></tr>
        <tr><th>Row 1</th><td>A3</td><td>A4</td><td>A5</td></tr>
        <tr><th>Row 2</th><td>A6</td><td>A7</td><td>A8</td></tr>
        <tr><th>Row 3</th><td>B0</td><td>B1</td><td>(none)</td></tr>
    </tbody>
</table>

...which can be configured with the following Devicetree code:

```devicetree
/ {
    chosen {
        zmk,kscan = &kscan0;
    };

    kscan0: kscan_composite {
        compatible = "zmk,kscan-composite";
        label = "KSCAN0";
        rows = <3>;
        columns = <4>;

        // Include the matrix driver
        matrix {
            kscan = <&kscan1>;
        };

        // Include the direct GPIO driver...
        direct {
            kscan = <&kscan2>;
            row-offset = <3>; // ..and shift it to not overlap
        };
    };

    kscan1: kscan_matrix {
        compatible = "zmk,kscan-gpio-matrix";
        // define 3x3 matrix here...
    };

    kscan2: kscan_direct {
        compatible = "zmk,kscan-gpio-direct";
        // define 2 direct GPIOs here...
    };
}
```

## Mock Driver

Mock keyboard scan driver that simulates key events.

### Kconfig

Definition file: [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)

| Config                         | Type | Description              | Default |
| ------------------------------ | ---- | ------------------------ | ------- |
| `CONFIG_ZMK_KSCAN_MOCK_DRIVER` | bool | Enable mock kscan driver | n       |

### Devicetree

Applies to: `compatible = "zmk,kscan-mock"`

Definition file: [zmk/app/dts/bindings/zmk,kscan-mock.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Ckscan-mock.yaml)

| Property       | Type   | Description                                   | Default |
| -------------- | ------ | --------------------------------------------- | ------- |
| `label`        | string | Unique label for the node                     |         |
| `event-period` | int    | Milliseconds between each generated event     |         |
| `events`       | array  | List of key events to simulate                |         |
| `rows`         | int    | The number rows of in the composite matrix    |         |
| `cols`         | int    | The number columns of in the composite matrix |         |
| `exit-after`   | bool   | Exit the program after running all events     | false   |

The `events` array should be defined using the macros from [dt-bindings/zmk/kscan_mock.h](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/kscan_mock.h).

## Matrix Transform

Defines a mapping from keymap logical positions to physical matrix positions.

Transforms should be used any time the physical layout of a keyboard's keys does
not match the layout of its electrical matrix and/or when not all positions in
the matrix are used. This applies to most non-ortholinear boards.

Transforms can also be used for keyboards with multiple layouts. You can define
multiple matrix transform nodes, one for each layout, and users can select which
one they want from the `/chosen` node in their keymaps.

See the [new shield guide](/docs/development/new-shield/#optional-matrix-transform)
for more documentation on how to define a matrix transform.

### Devicetree

Applies to: `compatible = "zmk,matrix-transform"`

Definition file: [zmk/app/dts/bindings/zmk,matrix-transform.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Cmatrix-transform.yaml)

| Property     | Type  | Description                                                           | Default |
| ------------ | ----- | --------------------------------------------------------------------- | ------- |
| `rows`       | int   | Number of rows in the transformed matrix                              |         |
| `columns`    | int   | Number of columns in the transformed matrix                           |         |
| `row-offset` | int   | Adds an offset to all rows before looking them up in the transform    | 0       |
| `col-offset` | int   | Adds an offset to all columns before looking them up in the transform | 0       |
| `map`        | array | A list of position transforms                                         |         |

The `map` array should be defined using the `RC()` macro from [dt-bindings/zmk/matrix_transform.h](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/matrix_transform.h).
It should have one item per logical position in the keymap. Each item should list
the physical row and column that should trigger the key in that position.

### Example Configuration

Consider a keyboard with a [duplex matrix](https://wiki.ai03.com/books/pcb-design/page/matrices-and-duplex-matrix),
where the matrix has twice as many rows and half as many columns as the keyboard
has keys. A matrix transform can be used to correct for this so that keymaps can
match the layout of the keys, not the layout of the matrix.

```devicetree
/ {
    chosen {
        zmk,kscan = &kscan0;
        zmk,matrix_transform = &default_transform;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        rows = <12>;
        columns = <8>;
        // define the matrix...
    };

    default_transform: matrix_transform {
        compatible = "zmk,matrix-transform";
        rows = <6>;
        columns = <16>;
        // ESC F1 F2 F3   ...
        // `   1  2  3    ...
        // Tab  Q  W  E   ...
        // Caps  A  S  D  ...
        // Shift  Z  X  C ...
        // Ctrl Alt       ...
        map = <
            RC(0,0) RC(1,0) RC(0,1) RC(1,1)      // ...
            RC(2,0) RC(3,0) RC(2,1) RC(3,1)      // ...
            RC(4,0)   RC(5,0) RC(4,1) RC(5,1)    // ...
            RC(6,0)      RC(7,0) RC(6,1) RC(7,1) // ...
            RC(8,0)         RC(8,1) RC(9,1)      // ...
            RC(10,0) RC(11,0)                    // ...
        >;
    };
};
```
