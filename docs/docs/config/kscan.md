---
title: Keyboard Scan Configuration
sidebar_label: Keyboard Scan
---

See [Configuration Overview](index.md) for instructions on how to change these settings.

## Common

### Kconfig

Definition files:

- [zmk/app/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/Kconfig)
- [zmk/app/module/drivers/kscan/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/module/drivers/kscan/Kconfig)

| Config                                 | Type | Description                                          | Default |
| -------------------------------------- | ---- | ---------------------------------------------------- | ------- |
| `CONFIG_ZMK_KSCAN_EVENT_QUEUE_SIZE`    | int  | Size of the event queue for kscan events             | 4       |
| `CONFIG_ZMK_KSCAN_INIT_PRIORITY`       | int  | Keyboard scan device driver initialization priority  | 40      |
| `CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS`   | int  | Global debounce time for key press in milliseconds   | -1      |
| `CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS` | int  | Global debounce time for key release in milliseconds | -1      |

If the debounce press/release values are set to any value other than `-1`, they override the `debounce-press-ms` and `debounce-release-ms` devicetree properties for all keyboard scan drivers which support them. See the [debouncing documentation](../features/debouncing.md) for more details.

### Devicetree

Applies to: [`/chosen` node](https://docs.zephyrproject.org/3.5.0/build/dts/intro-syntax-structure.html#aliases-and-chosen-nodes)

| Property               | Type | Description                                                   |
| ---------------------- | ---- | ------------------------------------------------------------- |
| `zmk,kscan`            | path | The node for the keyboard scan driver to use                  |
| `zmk,matrix-transform` | path | The node for the [matrix transform](#matrix-transform) to use |

## Demux Driver

Keyboard scan driver which works like a regular matrix but uses a demultiplexer to drive the rows or columns. This allows N GPIOs to drive N<sup>2</sup> rows or columns instead of just N like with a regular matrix.

:::note
Currently this driver does not honor the `CONFIG_ZMK_KSCAN_DEBOUNCE_*` settings.
:::

### Devicetree

Applies to: `compatible = "zmk,kscan-gpio-demux"`

Definition file: [zmk/app/module/dts/bindings/kscan/zmk,kscan-gpio-demux.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/module/dts/bindings/kscan/zmk%2Ckscan-gpio-demux.yaml)

| Property                | Type       | Description                      | Default |
| ----------------------- | ---------- | -------------------------------- | ------- |
| `input-gpios`           | GPIO array | Input GPIOs                      |         |
| `output-gpios`          | GPIO array | Demultiplexer address GPIOs      |         |
| `debounce-period`       | int        | Debounce period in milliseconds  | 5       |
| `polling-interval-msec` | int        | Polling interval in milliseconds | 25      |

## Direct GPIO Driver

Keyboard scan driver where each key has a dedicated GPIO.

### Kconfig

Definition file: [zmk/app/module/drivers/kscan/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/module/drivers/kscan/Kconfig)

| Config                            | Type | Description                                      | Default |
| --------------------------------- | ---- | ------------------------------------------------ | ------- |
| `CONFIG_ZMK_KSCAN_DIRECT_POLLING` | bool | Poll for key presses instead of using interrupts | n       |

### Devicetree

Applies to: `compatible = "zmk,kscan-gpio-direct"`

Definition file: [zmk/app/module/dts/bindings/kscan/zmk,kscan-gpio-direct.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/module/dts/bindings/kscan/zmk%2Ckscan-gpio-direct.yaml)

| Property                  | Type       | Description                                                                                                 | Default |
| ------------------------- | ---------- | ----------------------------------------------------------------------------------------------------------- | ------- |
| `input-gpios`             | GPIO array | Input GPIOs (one per key). Can be either direct GPIO pin or `gpio-key` references.                          |         |
| `debounce-press-ms`       | int        | Debounce time for key press in milliseconds. Use 0 for eager debouncing.                                    | 5       |
| `debounce-release-ms`     | int        | Debounce time for key release in milliseconds.                                                              | 5       |
| `debounce-scan-period-ms` | int        | Time between reads in milliseconds when any key is pressed.                                                 | 1       |
| `poll-period-ms`          | int        | Time between reads in milliseconds when no key is pressed and `CONFIG_ZMK_KSCAN_DIRECT_POLLING` is enabled. | 10      |
| `toggle-mode`             | bool       | Use toggle switch mode.                                                                                     | n       |
| `wakeup-source`           | bool       | Mark this kscan instance as able to wake the keyboard from deep sleep                                       | n       |

Assuming the switches connect each GPIO pin to the ground, the [GPIO flags](https://docs.zephyrproject.org/3.5.0/hardware/peripherals/gpio.html#api-reference) for the elements in `input-gpios` should be `(GPIO_ACTIVE_LOW | GPIO_PULL_UP)`:

```dts
    kscan0: kscan {
        compatible = "zmk,kscan-gpio-direct";
        wakeup-source;
        input-gpios
            = <&pro_micro 4 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
            , <&pro_micro 5 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
            ;
    };
```

By default, a switch will drain current through the internal pull up/down resistor whenever it is pressed. This is not ideal for a toggle switch, where the switch may be left in the "pressed" state for a long time. Enabling `toggle-mode` will make the driver enable and disable the internal pull up/down resistor as needed when the switch is toggled to minimise power draw. For `toggle-mode` to work correctly each pole of the switch needs a dedicated GPIO pin.

`toggle-mode` applies to all switches handled by the instance of the driver. To use a toggle switch with other, non-toggle, direct GPIO switches, create two instances of the direct GPIO driver, one with `toggle-mode` and the other without. Then, use a [composite driver](#composite-driver) to combine them. The state of the switch is read on power on, so if the switch is moved whilst the board is off this will get correctly interpreted by the driver.

When using `toggle-mode` the pull resistors get automatically set by the driver and should not be set in the devicetree via GPIO flags. Assuming the common pole of the switch is connected to ground with an SP3T switch:

```dts
    kscan_sp3t_toggle: kscan_sp3t_toggle {
        compatible = "zmk,kscan-gpio-direct";
        toggle-mode;

        input-gpios
        = <&pro_micro 4 GPIO_ACTIVE_LOW>
        , <&pro_micro 3 GPIO_ACTIVE_LOW>
        , <&pro_micro 2 GPIO_ACTIVE_LOW>
        ;
    };
```

## Matrix Driver

Keyboard scan driver where keys are arranged on a matrix with one GPIO per row and column.

Definition file: [zmk/app/module/drivers/kscan/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/module/drivers/kscan/Kconfig)

| Config                                         | Type        | Description                                                               | Default |
| ---------------------------------------------- | ----------- | ------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_KSCAN_MATRIX_POLLING`              | bool        | Poll for key presses instead of using interrupts                          | n       |
| `CONFIG_ZMK_KSCAN_MATRIX_WAIT_BEFORE_INPUTS`   | int (ticks) | How long to wait before reading input pins after setting output active    | 0       |
| `CONFIG_ZMK_KSCAN_MATRIX_WAIT_BETWEEN_OUTPUTS` | int (ticks) | How long to wait between each output to allow previous output to "settle" | 0       |

### Devicetree

Applies to: `compatible = "zmk,kscan-gpio-matrix"`

Definition file: [zmk/app/module/dts/bindings/kscan/zmk,kscan-gpio-matrix.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/module/dts/bindings/kscan/zmk%2Ckscan-gpio-matrix.yaml)

| Property                  | Type       | Description                                                                                                 | Default     |
| ------------------------- | ---------- | ----------------------------------------------------------------------------------------------------------- | ----------- |
| `row-gpios`               | GPIO array | Matrix row GPIOs in order, starting from the top row                                                        |             |
| `col-gpios`               | GPIO array | Matrix column GPIOs in order, starting from the leftmost row                                                |             |
| `debounce-press-ms`       | int        | Debounce time for key press in milliseconds. Use 0 for eager debouncing.                                    | 5           |
| `debounce-release-ms`     | int        | Debounce time for key release in milliseconds.                                                              | 5           |
| `debounce-scan-period-ms` | int        | Time between reads in milliseconds when any key is pressed.                                                 | 1           |
| `diode-direction`         | string     | The direction of the matrix diodes                                                                          | `"row2col"` |
| `poll-period-ms`          | int        | Time between reads in milliseconds when no key is pressed and `CONFIG_ZMK_KSCAN_MATRIX_POLLING` is enabled. | 10          |
| `wakeup-source`           | bool       | Mark this kscan instance as able to wake the keyboard from deep sleep                                       | n           |

The `diode-direction` property must be one of:

| Value       | Description                                                           |
| ----------- | --------------------------------------------------------------------- |
| `"row2col"` | Diodes point from rows to columns (cathodes are connected to columns) |
| `"col2row"` | Diodes point from columns to rows (cathodes are connected to rows)    |

Given the `diode-direction`, the [GPIO flags](https://docs.zephyrproject.org/3.5.0/hardware/peripherals/gpio.html#api-reference) for the elements in `row-` and `col-gpios` should be set appropriately.
The output pins (e.g. columns for `col2row`) should have the flag `GPIO_ACTIVE_HIGH`, and input pins (e.g. rows for `col2row`) should have the flags `(GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)`:

```dts
    kscan0: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        wakeup-source;
        diode-direction = "col2row";
        col-gpios
            = <&pro_micro 4 GPIO_ACTIVE_HIGH>
            , <&pro_micro 5 GPIO_ACTIVE_HIGH>
            ;
        row-gpios
            = <&pro_micro 6 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            , <&pro_micro 7 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
            ;
    };
```

## Charlieplex Driver

Keyboard scan driver where keys are arranged on a matrix with each GPIO used as both input and output.

- With `interrupt-gpios` unset, this allows n pins to drive n\*(n-1) keys.
- With `interrupt-gpios` set, n pins will drive (n-1)\*(n-2) keys, but provide much improved power handling.

Definition file: [zmk/app/module/drivers/kscan/Kconfig](https://github.com/zmkfirmware/zmk/blob/main/app/module/drivers/kscan/Kconfig)

| Config                                              | Type        | Description                                                               | Default |
| --------------------------------------------------- | ----------- | ------------------------------------------------------------------------- | ------- |
| `CONFIG_ZMK_KSCAN_CHARLIEPLEX_WAIT_BEFORE_INPUTS`   | int (ticks) | How long to wait before reading input pins after setting output active    | 0       |
| `CONFIG_ZMK_KSCAN_CHARLIEPLEX_WAIT_BETWEEN_OUTPUTS` | int (ticks) | How long to wait between each output to allow previous output to "settle" | 0       |

### Devicetree

Applies to: `compatible = "zmk,kscan-gpio-charlieplex"`

Definition file: [zmk/app/module/dts/bindings/kscan/zmk,kscan-gpio-charlieplex.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/module/dts/bindings/kscan/zmk%2Ckscan-gpio-charlieplex.yaml)

| Property                  | Type       | Description                                                                                 | Default |
| ------------------------- | ---------- | ------------------------------------------------------------------------------------------- | ------- |
| `gpios`                   | GPIO array | GPIOs used, listed in order.                                                                |         |
| `interrupt-gpios`         | GPIO array | A single GPIO to use for interrupt. Leaving this empty will enable continuous polling.      |         |
| `debounce-press-ms`       | int        | Debounce time for key press in milliseconds. Use 0 for eager debouncing.                    | 5       |
| `debounce-release-ms`     | int        | Debounce time for key release in milliseconds.                                              | 5       |
| `debounce-scan-period-ms` | int        | Time between reads in milliseconds when any key is pressed.                                 | 1       |
| `poll-period-ms`          | int        | Time between reads in milliseconds when no key is pressed and `interrupt-gpois` is not set. | 10      |
| `wakeup-source`           | bool       | Mark this kscan instance as able to wake the keyboard from deep sleep                       | n       |

Define the transform with a [matrix transform](#matrix-transform). The row is always the driven pin, and the column always the receiving pin (input to the controller).
For example, in `RC(5,0)` power flows from the 6th pin in `gpios` to the 1st pin in `gpios`.
Exclude all positions where the row and column are the same as these pairs will never be triggered, since no pin can be both input and output at the same time.

The [GPIO flags](https://docs.zephyrproject.org/3.5.0/hardware/peripherals/gpio.html#api-reference) for the elements in `gpios` should be `GPIO_ACTIVE_HIGH`, and interrupt pins set in `interrupt-gpios` should have the flags `(GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)`.

## Composite Driver

Keyboard scan driver which combines multiple other keyboard scan drivers.

### Devicetree

Applies to : `compatible = "zmk,kscan-composite"`

Definition file: [zmk/app/dts/bindings/zmk,kscan-composite.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk,kscan-composite.yaml)

| Property | Type | Description                                   | Default |
| -------- | ---- | --------------------------------------------- | ------- |
| `rows`   | int  | The number of rows in the composite matrix    |         |
| `cols`   | int  | The number of columns in the composite matrix |         |

The `zmk,kscan-composite` node should have one child node per keyboard scan driver that should be composited. Each child node can have the following properties:

| Property        | Type    | Description                                                                    | Default |
| --------------- | ------- | ------------------------------------------------------------------------------ | ------- |
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

To combine them, we need to create a composite matrix with enough rows and columns to fit both sets of keys without overlapping, then set row and/or columns offsets to shift them so they do not overlap.

One possible way to do this is a 3x4 matrix where the direct GPIO keys are shifted to below the matrix keys...

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

```dts
/ {
    chosen {
        zmk,kscan = &kscan0;
    };

    kscan0: kscan_composite {
        compatible = "zmk,kscan-composite";
        rows = <4>;
        columns = <3>;

        // Include the matrix driver
        matrix {
            kscan = <&kscan1>;
        };

        // Include the direct GPIO driver...
        direct {
            kscan = <&kscan2>;
            row-offset = <3>; // ...and shift it to not overlap
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

### Devicetree

Applies to: `compatible = "zmk,kscan-mock"`

Definition file: [zmk/app/dts/bindings/zmk,kscan-mock.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Ckscan-mock.yaml)

| Property       | Type  | Description                                   | Default |
| -------------- | ----- | --------------------------------------------- | ------- |
| `event-period` | int   | Milliseconds between each generated event     |         |
| `events`       | array | List of key events to simulate                |         |
| `rows`         | int   | The number of rows in the composite matrix    |         |
| `cols`         | int   | The number of columns in the composite matrix |         |
| `exit-after`   | bool  | Exit the program after running all events     | false   |

The `events` array should be defined using the macros from [app/module/include/dt-bindings/zmk/kscan_mock.h](https://github.com/zmkfirmware/zmk/blob/main/app/module/include/dt-bindings/zmk/kscan_mock.h).

## Matrix Transform

Defines a mapping from keymap logical positions to physical matrix positions.

Transforms should be used any time the physical layout of a keyboard's keys does not match the layout of its electrical matrix and/or when not all positions in the matrix are used. This applies to most non-ortholinear boards.

Transforms can also be used for keyboards with multiple layouts. You can define multiple matrix transform nodes, one for each layout, and users can select which one they want from the `/chosen` node in their keymaps.

See the [new shield guide](../development/new-shield.mdx#matrix-transform) for more documentation on how to define a matrix transform.

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

The `map` array should be defined using the `RC()` macro from [dt-bindings/zmk/matrix_transform.h](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/matrix_transform.h). It should have one item per logical position in the keymap. Each item should list the physical row and column that should trigger the key in that position.

### Example: Skipping Unused Positions

Any keyboard which is not a grid of 1 unit keys will likely have some unused positions in the matrix. A matrix transform can be used to skip the unused positions so users don't have to set them to `&none` in keymaps.

```dts
// numpad.overlay
/ {
    chosen {
        zmk,kscan = &kscan0;
        zmk,matrix-transform = &default_transform;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        // define row-gpios with 5 elements and col-gpios with 4...
    };

    default_transform: matrix_transform {
        compatible = "zmk,matrix-transform";
        rows = <5>;
        columns = <4>;
        // ┌───┬───┬───┬───┐
        // │NUM│ / │ * │ - │
        // ├───┼───┼───┼───┤
        // │ 7 │ 8 │ 9 │ + │
        // ├───┼───┼───┤   │
        // │ 4 │ 5 │ 6 │   │
        // ├───┼───┼───┼───┤
        // │ 1 │ 2 │ 3 │RET│
        // ├───┴───┼───┤   │
        // │ 0     │ . │   │
        // └───────┴───┴───┘
        map = <
            RC(0,0) RC(0,1) RC(0,2) RC(0,3)
            RC(1,0) RC(1,1) RC(1,2) RC(1,3)
            RC(2,0) RC(2,1) RC(2,2)
            RC(3,0) RC(3,1) RC(3,2) RC(3,3)
            RC(4,0)         RC(4,1)
        >;
    };
};
```

```dts
// numpad.keymap
/ {
    keymap {
        compatible = "zmk,keymap";
        default {
            bindings = <
                &kp KP_NUM &kp KP_DIV &kp KP_MULT &kp KP_MINUS
                &kp KP_N7  &kp KP_N8  &kp KP_N9   &kp KP_PLUS
                &kp KP_N4  &kp KP_N5  &kp KP_N6
                &kp KP_N1  &kp KP_N2  &kp KP_N3   &kp KP_ENTER
                &kp KP_N0             &kp KP_DOT
            >;
        };
    }
};
```

### Example: Non-standard Matrix

Consider a keyboard with a [duplex matrix](https://wiki.ai03.com/books/pcb-design/page/matrices-and-duplex-matrix), where the matrix has twice as many rows and half as many columns as the keyboard has keys. A matrix transform can be used to correct for this so that keymaps can match the layout of the keys, not the layout of the matrix.

```dts
/ {
    chosen {
        zmk,kscan = &kscan0;
        zmk,matrix-transform = &default_transform;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        // define row-gpios with 12 elements and col-gpios with 8...
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
            RC(0,0) RC(1,0) RC(0,1) RC(1,1)       // ...
            RC(2,0) RC(3,0) RC(2,1) RC(3,1)       // ...
            RC(4,0)   RC(5,0) RC(4,1) RC(5,1)     // ...
            RC(6,0)     RC(7,0) RC(6,1) RC(7,1)   // ...
            RC(8,0)       RC(9,0) RC(8,1) RC(9,1) // ...
            RC(10,0) RC(11,0)                     // ...
        >;
    };
};
```

### Example: Charlieplex

Since a charlieplex driver will never align with a keyboard directly due to the un-addressable positions, a matrix transform should be used to map the pairs to the layout of the keys.
Note that the entire addressable space does not need to be mapped.

```devicetree
/ {
    chosen {
        zmk,kscan = &kscan0;
        zmk,matrix-transform = &default_transform;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-charlieplex";
        wakeup-source;

        interrupt-gpios = <&pro_micro 21 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN) >;
        gpios
          = <&pro_micro 16 GPIO_ACTIVE_HIGH>
          , <&pro_micro 17 GPIO_ACTIVE_HIGH>
          , <&pro_micro 18 GPIO_ACTIVE_HIGH>
          , <&pro_micro 19 GPIO_ACTIVE_HIGH>
          , <&pro_micro 20 GPIO_ACTIVE_HIGH>
          ; // addressable space is 5x5, (minus paired values)
    };

    default_transform: matrix_transform {
        compatible = "zmk,matrix-transform";
        rows = <3>;
        columns = <5>;
        //  Q  W  E  R
        //   A  S  D  F
        //    Z  X  C  V
        map = <
            RC(0,1) RC(0,2) RC(0,3) RC(0,4)
              RC(1,0) RC(1,2) RC(1,3) RC(1,4)
                RC(2,0) RC(2,1) RC(2,3) RC(2,4)
        >;
    };
};
```
