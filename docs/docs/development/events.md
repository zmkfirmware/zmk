---
title: Events
---
## Overview
ZMK uses events to decouple behaviours and to allow new functionality to be added to existing events.

## Subscribing to events

Subscribing to events uses a combination of the `ZMK_SUBSCRIPTION` and `ZMK_LISTENER` macros.

`ZMK_SUBSCRIPTION` tells the event manager that the module wants to subscribe to a particular type of event.

`ZMK_LISTENER` tells the event manager which method of the module to call when an event is raised on a subscription. When subscribing to multiple event types, you can check which event type was raised using the `is_<event_type>` helper methods (See example below).

:::note
The event manager expects the method to be called `<module>_listener` by convention.
:::

```
int behavior_hold_tap_listener(const struct zmk_event_header *eh) {
    if (is_position_state_changed(eh)) {
        return position_state_changed_listener(eh);
    } else if (is_keycode_state_changed(eh)) {
        return keycode_state_changed_listener(eh);
    }
    return 0;
}

ZMK_LISTENER(behavior_hold_tap, behavior_hold_tap_listener);
ZMK_SUBSCRIPTION(behavior_hold_tap, keycode_state_changed);
```

todo: Describe the return values from the listener methods.

## Events

### `activity_state_changed`

Fired by the activity monitor when the state changes. The monitor subscribes to `position_state_changed` and `sensor_event` events to keep track of activity.

- `ZMK_ACTIVITY_ACTIVE`
- `ZMK_ACTIVITY_IDLE` - When idle time is greater than `CONFIG_ZMK_IDLE_TIMEOUT `
- `ZMK_ACTIVITY_SLEEP` - When `CONFIG_ZMK_SLEEP` is enabled and idle time is greater than `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT`

### `battery_state_changed`

todo

### `ble_active_profile_changed`

todo

### `keycode_state_changed`

todo

### `layer_state_changed`

todo

### `modifiers_state_changed`

todo

### `position_state_changed`

todo

## `sensor_event`
todo

### `usb_conn_state_changed`
todo

