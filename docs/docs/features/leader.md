---
title: Leader Sequences
---

## Summary

Leader sequences are a way to have a sequence of key presses to output a different key. For example, you can hit the leader key followed by Q, then W to output escape.

### Configuration

Leader sequences configured in your `.keymap` file, but are separate from the `keymap` node found there, since they are processed before the normal keymap. They are specified like this:

```
/ {
    leader_sequences {
        compatible = "zmk,leader-sequences";
        seq_esc {
            key-positions = <0 1>;
            bindings = <&kp ESC>;
        };
    };
};
```

- The `compatible` property should always be `"zmk,leader-sequences"` for leader sequences.
- `key-positions` is an array of key positions. See the info section below about how to figure out the positions on your board.
- `layers = <0 1...>` will allow limiting a sequence to specific layers. This is an _optional_ parameter, when omitted it defaults to global scope.
- `bindings` is the behavior that is activated when all the key positions are pressed.
- (advanced) you can specify `immediate-trigger` if you want the sequence to be triggered as soon as all key positions are pressed. The default is to wait for the leader key's timeout to trigger the sequence if it overlaps another.

:::info

Key positions are numbered like the keys in your keymap, starting at 0. So, if the first key in your keymap is `Q`, this key is in position `0`. The next key (possibly `W`) will have position 1, etcetera.

:::

### Advanced usage

See [leader configuration](/docs/config/leader) for advanced configuration options.
