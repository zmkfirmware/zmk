---
title: Leader Key Behavior
sidebar_label: Leader Key
---

## Summary

The leader key behavior when triggered will capture all following key presses and trigger a [Leader Sequence](../features/leader.md) if pressed.

### Behavior Binding

- Reference: `&leader`

Example:

```
&leader
```

### Configuration

#### `timeout-ms`

Defines the amount of time to wait to trigger a completed leader sequence. Defaults to 200ms.

To change the timeout term, you can update the existing behavior:

```
&leader {
    timeout-ms = <500>;
};

/ {
    keymap {
        ...
    };
};
```

#### `timerless`

By default, the leader key will have a timeout, and will not wait for a sequence to be completed or another key to be pressed. Specify `timerless` if you want a timeout.
