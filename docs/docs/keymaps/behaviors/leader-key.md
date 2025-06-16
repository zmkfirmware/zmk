---
title: Leader Key Behavior
sidebar_label: Leader Key
---

## Summary

The leader key behavior when triggered will capture all following key presses and trigger a leader sequence's behavior if pressed.

### Configuration

#### `timeout-ms`

Defines the amount of time to wait to trigger a completed leader sequence. Defaults to no timeout and will wait indefinitely.

#### `overlap-timeout-ms`

Defines the amount of time to wait to trigger a completed leader sequence after an overlapping sequence is completed. Defaults to 200ms.

### Leader Sequences

#### Summary

Leader sequences are a way to have a sequence of key presses to output a different key. For example, you can hit the leader key followed by Q, then W to output escape.

#### Configuration

Leader sequences are configured as child nodes of your leader key behavior(s). They are specified like this:

```
leader: leader {
    compatible = "zmk,behavior-leader-key";
    #binding-cells = <0>;

    seq_esc {
        key-positions = <0 1>;
        bindings = <&kp ESC>;
    };
};
```

Each sequence can have the following properties:

- `key-positions` is an array of key positions. See the info section below about how to figure out the positions on your board.
- `bindings` is the behavior that is activated when all the key positions are pressed.
- (advanced) you can specify `immediate-trigger` if you want the sequence to be triggered as soon as all key positions are pressed. The default is to wait for the leader key's timeout to trigger the sequence if it overlaps another.

The `key-positions` array must not be longer than the `CONFIG_ZMK_LEADER_MAX_KEYS_PER_SEQUENCE` setting, which defaults to 4. If you want a leader sequence that triggers when pressing 5 keys, then you must change the setting to 5.

:::info

Key positions are numbered like the keys in your keymap, starting at 0. So, if the first key in your keymap is `Q`, this key is in position `0`. The next key (possibly `W`) will have position 1, etcetera.

:::

#### Locality

Sequence behaviors inherit their locality from the position of the leader key. For example, on split keyboards, a sequence using the `&bootloader` behavior will invoke the bootloader on the side on which the leader key is bound.

::::tip

Add the same leader key to both sides to be able to reset either side.

::::

### Example Usage

```
leader: leader {
    compatible = "zmk,behavior-leader-key";
    binding-cells = <0>;

    seq_b {
        key-positions = <1 1>;
        bindings = <&kp B>;
    };

    seq_c {
        key-positions = <1>;
        bindings = <&kp C>;
    };
};
```

### Advanced usage

See [leader configuration](../../config/leader.md) for advanced configuration options.
