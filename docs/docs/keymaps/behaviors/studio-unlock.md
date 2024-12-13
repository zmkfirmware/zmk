---
title: ZMK Studio Unlock Behavior
sidebar_label: ZMK Studio Unlock
---

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
