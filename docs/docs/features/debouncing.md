---
title: Debouncing
sidebar_label: Debouncing
---

To prevent contact bounce (also known as chatter) and noise spikes from causing
unwanted key presses, ZMK uses a [cycle-based debounce algorithm](https://www.kennethkuhn.com/electronics/debounce.c),
with each key debounced independently.

By default the debounce algorithm decides that a key is pressed or released after
the input is stable for 5 milliseconds. You can decrease this to improve latency
or increase it to improve reliability.

If you are having problems with a single key press registering multiple inputs,
you can try increasing the debounce press and/or release times to compensate.
You should also check for mechanical issues that might be causing the bouncing,
such as hot swap sockets that are making poor contact. You can try replacing the
socket or using some sharp tweezers to bend the contacts back together.

## Debounce Configuration

:::note
Currently only the `zmk,kscan-gpio-matrix` driver supports these options. The other drivers have not yet been updated to use the new debouncing code.
:::

### Global Options

You can set these options in your `.conf` file to control debouncing globally.
Values must be <= 16383.

- `CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS`: Debounce time for key press in milliseconds. Default = 5.
- `CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS`: Debounce time for key release in milliseconds. Default = 5.

If one of these options is set, it overrides the matching per-driver option described below.

For example, this would shorten the debounce time for both press and release:

```ini
CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS=3
CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS=3
```

### Per-driver Options

You can add these Devicetree properties to a kscan node to control debouncing for
that instance of the driver. Values must be <= 16383.

- `debounce-press-ms`: Debounce time for key press in milliseconds. Default = 5.
- `debounce-release-ms`: Debounce time for key release in milliseconds. Default = 5.
- ~~`debounce-period`~~: Deprecated. Sets both press and release debounce times.
- `debounce-scan-period-ms`: Time between reads in milliseconds when any key is pressed. Default = 1.

If one of the global options described above is set, it overrides the corresponding
per-driver option.

For example, if your board/shield has a kscan driver labeled `kscan0` in its
`.overlay`, `.dts`, or `.dtsi` files,

```devicetree
kscan0: kscan {
    compatible = "zmk,kscan-gpio-matrix";
    ...
};
```

then you could add this to your `.keymap`:

```devicetree
&kscan0 {
    debounce-press-ms = <3>;
    debounce-release-ms = <3>;
};
```

This must be placed outside of any blocks surrounded by curly braces (`{...}`).

`debounce-scan-period-ms` determines how often the keyboard scans while debouncing. It defaults to 1 ms, but it can be increased to reduce power use. Note that the debounce press/release timers are rounded up to the next multiple of the scan period. For example, if the scan period is 2 ms and debounce timer is 5 ms, key presses will take 6 ms to register instead of 5.

## Eager Debouncing

Eager debouncing means reporting a key change immediately and then ignoring
further changes for the debounce time. This eliminates latency but it is not
noise-resistant.

ZMK does not currently support true eager debouncing, but you can get something
very close by setting the time to detect a key press to zero and the time to detect
a key release to a larger number. This will detect a key press immediately, then
debounce the key release.

```ini
CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS=0
CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS=5
```

Also consider setting `CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS=1` instead, which adds
one millisecond of latency but protects against short noise spikes.

## Comparison With QMK

ZMK's default debouncing is similar to QMK's `sym_defer_pk` algorithm.

Setting `CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS=0` for eager debouncing would be similar
to QMK's (unimplemented as of this writing) `asym_eager_defer_pk`.

See [QMK's Debounce API documentation](https://beta.docs.qmk.fm/using-qmk/software-features/feature_debounce_type)
for more information.
