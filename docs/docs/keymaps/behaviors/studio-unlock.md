---
title: ZMK Studio Unlock Behavior
sidebar_label: ZMK Studio Unlock
---

:::warning[Beta Feature]

ZMK Studio is in beta. Although every effort has been made to provide a stable experience, you may still encounter issues during use. Please report any issues to [GitHub Issues](https://github.com/zmkfirmware/zmk-studio/issues).

:::

## Summary

## ZMK Studio Unlock

The ZMK Studio unlock behavior is used to grant [ZMK Studio](../../features/studio.md) access to make changes to your ZMK device. The device will remain unlocked until a certain amount of time of inactivity in ZMK Studio, or on disconnect. Those trigger events for relocking can be configured with [studio configuration](../../config/studio.md).

### Behavior Binding

- Reference: `&studio_unlock`
- Parameters: None

Example:

```dts
&studio_unlock
```
