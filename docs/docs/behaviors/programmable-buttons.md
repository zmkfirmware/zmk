---
title: Programmable Buttons Behaviors
sidebar_label: Programmable Buttons
---

## Summary

TODO: Programmable Buttons can be used to configure shortcuts that will not overlap with other softwares inputs. As per hid specification they are typically used to  
control software applications or GUI objects.

:::warning[OS Compatibility]

Only Linux is known to have programmable buttons compatibility

:::

## Configuration Option

This feature can be enabled or disabled explicitly via a config option:

```
CONFIG_ZMK_PROGRAMMABLE_BUTTONS=y
```

## Programmable Button Press

This behavior can press/release up to 32 programmable buttons.

### Behavior Binding

- Reference: `&pb`
- Parameter: A `uint8` from 1 up to 32


### Examples

The following will send the programmable button 1

```
&pb 1
```

