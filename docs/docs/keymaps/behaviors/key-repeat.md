---
title: Key Repeat Behavior
sidebar_label: Key Repeat
---

## Summary

The key repeat behavior when triggered will send whatever keycode was last sent/triggered.

### Behavior Binding

- Reference: `&key_repeat`

Example:

```dts
&key_repeat
```

### Configuration

#### Usage pages

By default, the key repeat will only track the last pressed key from the HID "Key" usage page, and ignore events from other usages, e.g. Consumer page.

If you'd rather have the repeat also capture and send Consumer page usages, you can update the existing behavior:

```dts
&key_repeat {
    usage-pages = <HID_USAGE_KEY HID_USAGE_CONSUMER>;
};

/ {
    keymap {
        ...
    };
};
```
