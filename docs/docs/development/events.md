---
title: Events
---
## Overview
ZMK uses events to decouple behaviours and to allow new functionality to be added to existing events.

## `activity_state_changed`
Fired by the activity monitor when the state changes. The monitor subscribes to `position_state_changed` and `sensor_event` events to keep track of activity.

* `ZMK_ACTIVITY_ACTIVE` 
* `ZMK_ACTIVITY_IDLE` - When idle time is greater than `CONFIG_ZMK_IDLE_TIMEOUT `
* `ZMK_ACTIVITY_SLEEP` - When `CONFIG_ZMK_SLEEP` is enabled and idle time is greater than `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT`

## `battery_state_changed`
todo

## `ble_active_profile_changed`
todo

## `keycode_state_changed`
todo

## `layer_state_changed`
todo

## `modifiers_state_changed`
todo

## `position_state_changed`
todo

## `sensor_event`
todo

## `usb_conn_state_changed`
todo

