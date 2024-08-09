---
title: Studio Unlock Behavior
sidebar_label: Studio Unlock
---

:::warn
ZMK Studio is still in active development. This behavior is documented in preparation for its general availability.
:::

## Summary

## Studio Unlock

The studio unlock behavior is used to grant ZMK Studio access to make changes to your ZMK device. The device will remain unlocked until a certain amount of time of inactivity in ZMK Studio, or on disconnect. Those trigger events for relocking can be configured with [studio configuration](../config/studio.md).

### Behavior Binding

- Reference: `&studio_unlock`
- Parameters: None

Example:

```dts
&studio_unlock
```
